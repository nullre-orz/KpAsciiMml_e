#include "partsequencerbase.h"
#include "musdata.h"
#include <algorithm>
#include <cmath>
#include <fmgen/opna.h>

namespace MusicCom
{
    const int TONE_KEY_OFF = -1;
    const int MAX_MACRO_COUNT = 100;

    PartSequencerBase::PartSequencerBase(FM::OPN& opn, const MusicData& music, CommandIterator command_tail)
        : opn_(opn),
          part_data_(),
          music_data_(music),
          command_tail_(command_tail)
    {
    }

    PartSequencerBase::~PartSequencerBase()
    {
    }

    void PartSequencerBase::Initialize()
    {
        part_data_.Playing = true;
        ReturnToHead();
        // 初手ポルタメント対応
        // 初期値をセットしておく
        part_data_.Tone = InitializeTone();
    }

    bool PartSequencerBase::IsPlaying() const
    {
        return part_data_.Playing;
    }

    void PartSequencerBase::Resume()
    {
        // 再生中は何もしない
        if (IsPlaying())
        {
            return;
        }

        part_data_.CommandPtr++;
        part_data_.Playing = true;

        // パート終端の場合は先頭に戻して再開
        if (part_data_.CommandPtr == command_tail_)
        {
            ReturnToHead();
        }
    }

    void PartSequencerBase::NextFrame(int current_frame)
    {
        if (!part_data_.Playing)
        {
            return;
        }

        PreProcess(current_frame);
        ProcessCommand(current_frame);
        ProcessEffect(current_frame);
    }

    const MusicData& PartSequencerBase::GetMusicData() const
    {
        return music_data_;
    }

    void PartSequencerBase::ReturnToHead()
    {
        part_data_.CommandPtr = GetHead();
        part_data_.CallStack = {};
        part_data_.LoopStack = {};
    }

    void PartSequencerBase::PreProcess(int current_frame)
    {
        if (part_data_.NoteEndFrame <= current_frame)
        {
            part_data_.LastOctave = part_data_.Octave;
            part_data_.LastTone = part_data_.Tone;
            // 前回のコマンド先読みで & や W が検出された場合はキーオフせず継続
            if (!part_data_.LinkedItem)
            {
                KeyOff();
            }
        }
    }

    std::optional<CommandIterator> PartSequencerBase::ProcessLoop(CommandIterator ptr)
    {
        while (1)
        {
            const Command& command = *ptr;

            switch (command.GetType())
            {
            case CommandType::TYPE_PAUSE:
                part_data_.Playing = false;
                part_data_.CommandPtr = ptr;
                return std::nullopt;
            case CommandType::TYPE_MACRO:
                if (!music_data_.IsMacroPresent(command.GetStrArg()))
                {
                    ptr++;
                    continue;
                }
                part_data_.CallStack.push(++CommandIterator(ptr));
                // 自己参照等で入れ子の数がMAX_MACRO_COUNTを超えたら強制終了
                if (part_data_.CallStack.size() > MAX_MACRO_COUNT)
                {
                    part_data_.Playing = false;
                    return std::nullopt;
                }
                ptr = music_data_.GetMacroHead(command.GetStrArg());
                continue;
            case CommandType::TYPE_RETURN:
                if (part_data_.CallStack.empty())
                {
                    part_data_.Playing = false;
                    return std::nullopt;
                }
                ptr = part_data_.CallStack.top();
                part_data_.CallStack.pop();
                continue;
            case CommandType::TYPE_LOOP:
                part_data_.LoopStack.push(std::pair<CommandIterator, int>(++CommandIterator(ptr), command.GetArg(0)));
                ptr++;
                continue;
            case CommandType::TYPE_EXIT:
            {
                if (part_data_.LoopStack.empty())
                {
                    part_data_.Playing = false;
                    return std::nullopt;
                }
                std::pair<CommandIterator, int>& p = part_data_.LoopStack.top();
                if (p.second != 0)
                {
                    p.second--;
                    // ループ脱出
                    if (p.second == 0)
                    {
                        part_data_.LoopStack.pop();
                        ptr++;
                    }
                    else
                    {
                        ptr = p.first;
                    }
                }
                // 無限ループ
                else
                {
                    // 音符の入っていない無限ループ検出
                    if (part_data_.InfiniteLooping)
                    {
                        part_data_.Playing = false;
                        return std::nullopt;
                    }
                    part_data_.InfiniteLooping = true;
                    ptr = p.first;
                }
            }
                continue;
            default:
                return ptr;
            }
        }
    }

    std::optional<CommandType> PartSequencerBase::FindLinkedItem(CommandIterator ptr) const
    {
        // 後方にタイ(&)やキーオフなし休符(W)が存在するかどうかを先読みして確認
        // ProcessLoop同様にマクロ/ループは展開するが、本体に影響しないようコピーで処理する
        bool infinite_looping = false;
        auto macro_call_stack(part_data_.CallStack);
        auto loop_stack(part_data_.LoopStack);

        while (1)
        {
            const Command& command = *ptr++;
            auto type = command.GetType();
            switch (type)
            {
            case CommandType::TYPE_PAUSE:
                return std::nullopt;
            case CommandType::TYPE_MACRO:
                if (!music_data_.IsMacroPresent(command.GetStrArg()))
                {
                    continue;
                }
                macro_call_stack.push(CommandIterator(ptr));
                // 自己参照等で入れ子の数がMAX_MACRO_COUNTを超えたら強制終了
                if (macro_call_stack.size() > MAX_MACRO_COUNT)
                {
                    return std::nullopt;
                }
                ptr = music_data_.GetMacroHead(command.GetStrArg());
                break;
            case CommandType::TYPE_RETURN:
                if (macro_call_stack.empty())
                {
                    return std::nullopt;
                }
                ptr = macro_call_stack.top();
                macro_call_stack.pop();
                break;
            case CommandType::TYPE_LOOP:
                loop_stack.push(std::pair<CommandIterator, int>(CommandIterator(ptr), command.GetArg(0)));
                break;
            case CommandType::TYPE_EXIT:
            {
                if (loop_stack.empty())
                {
                    return std::nullopt;
                }
                std::pair<CommandIterator, int>& p = loop_stack.top();
                if (p.second != 0)
                {
                    p.second--;
                    // ループ脱出
                    if (p.second == 0)
                    {
                        loop_stack.pop();
                    }
                    else
                    {
                        ptr = p.first;
                    }
                }
                // 無限ループ
                else
                {
                    // 音符の入っていない無限ループ検出
                    if (infinite_looping)
                    {
                        return std::nullopt;
                    }
                    infinite_looping = true;
                    ptr = p.first;
                }
            }
                continue;
            case CommandType::TYPE_TIE:
            case CommandType::TYPE_WAIT:
                // & および W はそのまま返す
                return type;
            case CommandType::TYPE_NOTE:
            case CommandType::TYPE_REST:
                return std::nullopt;
            }
        }

        return std::nullopt;
    }

    void PartSequencerBase::ProcessCommand(int current_frame)
    {
        part_data_.InfiniteLooping = false;
        auto ptr = part_data_.CommandPtr;

        while (part_data_.NoteEndFrame <= current_frame)
        {
            // マクロ・ループ・終端は別処理
            {
                auto result = ProcessLoop(ptr);
                if (!result)
                {
                    return;
                }
                ptr = *result;
            }

            ptr = ProcessCommandImpl(ptr, current_frame, part_data_);
        }
        part_data_.CommandPtr = ptr;
    }

    CommandIterator PartSequencerBase::ProcessCommandImpl(CommandIterator ptr, int current_frame, PartData& part_data)
    {
        static auto get_length = [](const PartData& part_data, int base_length)
        {
            return (base_length == 0) ? part_data.DefaultNoteLength : base_length;
        };

        static auto update_note_frames = [](PartData& part_data, int current_frame, int length)
        {
            part_data.NoteBeginFrame = current_frame;
            part_data.NoteEndFrame = current_frame + length;
        };

        static auto update_keyoff_frames = [](const PartSequencerBase& sequencer, PartData& part_data, CommandIterator ptr, int current_frame, int length)
        {
            if ((part_data.LinkedItem = sequencer.FindLinkedItem(CommandIterator(ptr))) == CommandType::TYPE_TIE)
            {
                part_data.KeyOffFrame = part_data.NoteEndFrame;
            }
            else
            {
                part_data.KeyOffFrame = current_frame + std::max(length * part_data.GateTime / 8, 1);
            }
        };

        const Command& command = *ptr++;
        switch (command.GetType())
        {
        case CommandType::TYPE_NOTE:
        {
            if (!part_data.LinkedItem)
            {
                part_data.KeyOnFrame = current_frame;
            }

            UpdateTone(command.GetArg(0), part_data);
            KeyOn();

            if (part_data.LastTone == TONE_KEY_OFF)
            {
                part_data.LastOctave = part_data.Octave;
                part_data.LastTone = part_data.Tone;
            }

            int length = get_length(part_data, command.GetArg(1));
            update_note_frames(part_data, current_frame, length);
            update_keyoff_frames(*this, part_data, ptr, current_frame, length);

            break;
        }
        case CommandType::TYPE_REST:
        {
            int length = get_length(part_data, command.GetArg(0));
            update_note_frames(part_data, current_frame, length);

            if (part_data.LinkedItem != CommandType::TYPE_TIE)
            {
                // &R でなければキーオフ
                KeyOff();
                part_data.LastTone = TONE_KEY_OFF;
                part_data.Tone = TONE_KEY_OFF;
            }
            else
            {
                update_keyoff_frames(*this, part_data, ptr, current_frame, length);
            }

            break;
        }
        case CommandType::TYPE_WAIT:
        {
            int length = get_length(part_data, command.GetArg(0));
            update_note_frames(part_data, current_frame, length);

            if (part_data.LinkedItem == CommandType::TYPE_TIE)
            {
                // &W は後方にも & があるとみなす (music.comのバグ?)
                // LinkedItemは更新不要
                part_data.KeyOffFrame = part_data.NoteEndFrame;
            }
            else
            {
                update_keyoff_frames(*this, part_data, ptr, current_frame, length);
            }

            break;
        }
        case CommandType::TYPE_LENGTH:
            part_data.DefaultNoteLength = command.GetArg(0);
            break;
        case CommandType::TYPE_OCTAVE:
            part_data.Octave = std::min(std::max(command.GetArg(0), 1), 8);
            break;
        case CommandType::TYPE_OCTAVE_DOWN:
            part_data.Octave = std::max(part_data.Octave - 1, 0);
            break;
        case CommandType::TYPE_OCTAVE_UP:
            part_data.Octave = std::min(part_data.Octave + 1, 8);
            break;
        case CommandType::TYPE_VOLUME:
            part_data.Volume = std::min(std::max(command.GetArg(0), 0), 15);
            SetVolume(part_data.Volume);
            break;
        //case CommandType::TYPE_TONE:
        case CommandType::TYPE_GATE_TIME:
            part_data.GateTime = command.GetArg(0);
            break;
        case CommandType::TYPE_DETUNE:
            part_data.Detune = command.GetArg(0);
            break;
        case CommandType::TYPE_PORTAMENTO:
            part_data.PLength = std::max(command.GetArg(0), 0);
            // 排他
            part_data.ILength = 0;
            part_data.ULength = 0;
            break;
        case CommandType::TYPE_TREMOLO:
            part_data.UDepth = command.GetArg(0);
            part_data.ULength = std::max(command.GetArg(1), 0);
            part_data.UDelay = std::max(command.GetArg(2), 0);
            // 排他
            part_data.ILength = 0;
            part_data.PLength = 0;
            break;
        case CommandType::TYPE_VIBRATO:
            part_data.IDepth = command.GetArg(0);
            part_data.ILength = std::max(command.GetArg(1), 0);
            part_data.IDelay = std::max(command.GetArg(2), 0);
            // 排他
            part_data.PLength = 0;
            part_data.ULength = 0;
            break;
        //case CommandType::TYPE_ENV_FORM:
        //case CommandType::TYPE_ENV_PERIOD:
        case CommandType::TYPE_DIRECT:
            opn_.SetReg(command.GetArg(0), command.GetArg(1));
            break;
        }

        return ptr;
    }

    void PartSequencerBase::ProcessEffect(int current_frame)
    {
        // エフェクト等の処理
        // ディレイはKeyOnFrameを基準とする

        // 一時停止中の場合は何もしない
        if (!part_data_.Playing)
        {
            return;
        }

        // ゲートタイム(Q)でのキーオフ
        if (part_data_.KeyOffFrame <= current_frame)
        {
            KeyOff();
        }

        // Volume
        int keyon_length = current_frame - part_data_.KeyOnFrame;
        int final_volume = part_data_.Volume;

        if (part_data_.UDepth != 0 && keyon_length >= part_data_.UDelay)
        {
            if (((keyon_length - part_data_.UDelay) / part_data_.ULength) & 1)
                final_volume -= part_data_.UDepth;
        }

        final_volume = AdjustVolume(final_volume, keyon_length, part_data_);

        final_volume = std::min(std::max(final_volume, 0), 15);
        SetVolume(final_volume);

        // Tone
        if (part_data_.PLength != 0 && part_data_.Tone != TONE_KEY_OFF)
        {
            // ポルタメントは音程が変わった時点で適用する
            // KeyOnFrameではなくNoteBeginFrameを基準とする
            int diff = current_frame - part_data_.NoteBeginFrame;
            if (diff <= part_data_.PLength)
            {
                double coefficient = diff / static_cast<double>(part_data_.PLength);
                ApplyPortamentoEffect(part_data_.Octave, part_data_.Tone, part_data_.LastOctave, part_data_.LastTone, coefficient);
            }
        }
        else if (part_data_.ILength != 0 && keyon_length >= part_data_.IDelay)
        {
            int depth;
            if (((keyon_length - part_data_.IDelay) / part_data_.ILength) & 1)
                depth = -part_data_.IDepth;
            else
                depth = part_data_.IDepth;

            int tone = static_cast<int>(part_data_.Tone * pow(2.0, depth / (255.0 * 12.0)) + 0.5);
            SetTone(part_data_.Octave, tone);
        }
    }

    int PartSequencerBase::AdjustVolume(int volume, int length, const PartData& part_data)
    {
        // デフォルト実装は何もしない
        return volume;
    }

} // namespace MusicCom
