#include "partsequencerbase.h"
#include "musdata.h"
#include <algorithm>
#include <cmath>
#include <fmgen/opna.h>
#include <limits>

namespace MusicCom
{
    const int TONE_KEY_OFF = -1;
    const int MAX_MACRO_COUNT = 24;

    PartSequencerBase::PartSequencerBase(FM::OPN& opn, const MusicData& music, CommandIterator command_tail, int rate)
        : opn_(opn),
          part_data_(),
          music_data_(music),
          command_tail_(command_tail),
          rate_(rate),
          samples_per_frame_(0),
          samples_left_(0),
          current_frame_(0)
    {
    }

    PartSequencerBase::~PartSequencerBase()
    {
    }

    void PartSequencerBase::Initialize()
    {
        part_data_.Playing = true;
        part_data_.HasPreviousNote = false;
        ReturnToHead();

        // フレーム初期化
        samples_per_frame_ = CalculatePerFrame(GetMusicData().GetTempo());
        samples_left_ = 0;
        current_frame_ = 0;

        // サブクラス固有の設定
        InitializeImpl(part_data_);
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

    int PartSequencerBase::GetRemainFrameSize()
    {
        // NVI pattern
        return GetRemainFrameSizeImpl();
    }
    int PartSequencerBase::GetRemainFrameSizeImpl()
    {
        return samples_left_;
    }

    void PartSequencerBase::IncreaseFrame(int frame_size)
    {
        if (!part_data_.Playing)
        {
            return;
        }

        // NVI pattern
        IncreaseFrameImpl(frame_size);
    }
    void PartSequencerBase::IncreaseFrameImpl(int frame_size)
    {
        samples_left_ -= frame_size;
        if (samples_left_ <= 0)
        {
            samples_left_ = samples_per_frame_;
            NextCommandFrame();
        }
    }

    void PartSequencerBase::NextCommandFrame()
    {
        PreProcess(current_frame_);
        ProcessCommand(current_frame_);
        ProcessEffect(current_frame_);

        current_frame_++;
    }

    const MusicData& PartSequencerBase::GetMusicData() const
    {
        return music_data_;
    }

    int PartSequencerBase::CalculatePerFrame(int tempo)
    {
        // music.comの挙動に合わせ、テンポを補正する(10%強の高速化)
        // テンポが30未満の場合は30として扱う
        return static_cast<int>(rate_ * 3.0 * (5880 / std::max(tempo, 30)) / 5200.0);
    }

    void PartSequencerBase::ReturnToHead()
    {
        part_data_.CommandPtr = GetHead();
        part_data_.CallStack = {};
        part_data_.LoopStack = {};
    }

    void PartSequencerBase::PreProcess(int current_frame)
    {
        if (part_data_.HasPreviousNote && part_data_.NoteEndFrame <= current_frame)
        {
            part_data_.LastOctave = part_data_.Octave;
            part_data_.LastTone = part_data_.Tone;
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
                    if (part_data_.HasPreviousNote && !part_data_.LinkedItem)
                    {
                        KeyOff();
                    }
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
            part_data.LinkedItem = sequencer.FindLinkedItem(CommandIterator(ptr));
            if (ptr->GetType() == CommandType::TYPE_TIE)
            {
                part_data.KeyOffFrame = part_data.NoteEndFrame;
            }
            else
            {
                if (part_data.GateTime >= 8)
                {
                    // Q8以上では途中キーオフを無効化する
                    part_data.KeyOffFrame = std::numeric_limits<int>::max();
                }
                else
                {
                    // 64分音符単位でゲート時間を計算する
                    part_data.KeyOffFrame = current_frame + (length - 1) * part_data.GateTime / 8 + 1;
                }
            }
        };

        const Command& command = *ptr++;
        switch (command.GetType())
        {
        case CommandType::TYPE_NOTE:
        {
            if (!part_data.LinkedItem)
            {
                if (part_data.HasPreviousNote)
                {
                    KeyOff();
                }
                part_data.KeyOnFrame = current_frame;
            }

            // オクターブの変更を反映
            part_data.Octave = part_data.ReservedOctave;

            UpdateTone(command.GetArg(0), part_data);
            if (part_data.LinkedItem != CommandType::TYPE_TIE)
            {
                KeyOn();
            }
            part_data.HasPreviousNote = true;

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
            part_data.HasPreviousNote = true;
            int length = get_length(part_data, command.GetArg(0));
            update_note_frames(part_data, current_frame, length);

            if (part_data.LinkedItem != CommandType::TYPE_TIE)
            {
                // &R でなければキーオフ
                KeyOff();

                // 休符後のトーンエフェクトを無効にするためキーオフ状態を設定
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
            auto linked_item = part_data.LinkedItem;
            int length = get_length(part_data, command.GetArg(0));
            update_note_frames(part_data, current_frame, length);
            update_keyoff_frames(*this, part_data, ptr, current_frame, length);

            // W開始前のタイはWを越えて維持する
            if (linked_item == CommandType::TYPE_TIE)
            {
                part_data.LinkedItem = linked_item;
            }

            break;
        }
        case CommandType::TYPE_LENGTH:
            part_data.DefaultNoteLength = command.GetArg(0);
            break;
        case CommandType::TYPE_OCTAVE:
            part_data.ReservedOctave = std::clamp(command.GetArg(0), 0, 8);
            break;
        case CommandType::TYPE_OCTAVE_DOWN:
            part_data.ReservedOctave = std::max(part_data.ReservedOctave - 1, 0);
            break;
        case CommandType::TYPE_OCTAVE_UP:
            part_data.ReservedOctave = std::min(part_data.ReservedOctave + 1, 8);
            break;
        case CommandType::TYPE_VOLUME:
            part_data.Volume = command.GetArg(0);
            SetVolume(std::clamp(part_data.Volume, 0, 15));
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
            // Iのみ排他
            part_data.IDepth = 0;
            break;
        case CommandType::TYPE_TREMOLO:
            part_data.UDepth = command.GetArg(0);
            part_data.ULength = std::max(command.GetArg(1), 1);
            part_data.UDelay = std::max(command.GetArg(2), 0);
            break;
        case CommandType::TYPE_VIBRATO:
            part_data.IDepth = command.GetArg(0);
            part_data.ILength = std::max(command.GetArg(1), 1);
            part_data.IDelay = std::max(command.GetArg(2), 0);
            // Pのみ排他
            part_data.PLength = 0;
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

        // ビブラート用サブ関数
        static auto set_vibrato = [](PartSequencerBase& sequencer, int keyon_length, int base_tone)
        {
            auto part_data = sequencer.part_data_;
            int depth = (((keyon_length - part_data.IDelay) / part_data.ILength) & 1) ? -part_data.IDepth : part_data.IDepth;
            sequencer.ApplyVibratoEffect(part_data.Octave, base_tone, depth);
        };

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
        if (part_data_.UDepth != 0 || part_data_.SSGEnvOn)
        {
            int final_volume = part_data_.Volume;

            if (part_data_.UDepth != 0 && keyon_length >= part_data_.UDelay)
            {
                if (((keyon_length - part_data_.UDelay) / part_data_.ULength) & 1)
                    final_volume -= part_data_.UDepth;
            }

            final_volume = AdjustVolume(final_volume, keyon_length, part_data_);

            final_volume = std::min(std::max(final_volume, 0), 15);
            SetVolume(final_volume);
        }

        // Tone
        if (part_data_.PLength != 0 && part_data_.Tone != TONE_KEY_OFF)
        {
            // ポルタメントは音程が変わった時点で適用する。
            // P+1を分母として整数差分を求め、P+1 tick目に目標音程へ切り替える。
            int tick = current_frame - part_data_.NoteBeginFrame;
            if (tick <= part_data_.PLength + 1)
            {
                ApplyPortamentoEffect(part_data_.Octave, part_data_.Tone, part_data_.LastOctave, part_data_.LastTone, tick, part_data_.PLength);
            }
        }
        else if (part_data_.IDepth != 0 && keyon_length >= part_data_.IDelay)
        {
            if (part_data_.Tone != TONE_KEY_OFF)
            {
                set_vibrato(*this, keyon_length, part_data_.Tone);
            }
            else if (part_data_.LastTone != TONE_KEY_OFF)
            {
                set_vibrato(*this, keyon_length, part_data_.LastTone);
                part_data_.LastTone = TONE_KEY_OFF;
            }
        }
    }

    int PartSequencerBase::AdjustVolume(int volume, int length, const PartData& part_data)
    {
        // デフォルト実装は何もしない
        return volume;
    }

    void PartSequencerBase::ApplyVibratoEffect(int octave, int tone, int depth)
    {
        int adjusted_tone = static_cast<int>(tone * pow(2.0, depth / (255.0 * 12.0)) + 0.5);
        SetTone(octave, adjusted_tone);
    }

} // namespace MusicCom
