#include "partsequencerbase.h"
#include "musdata.h"
#include <algorithm>

namespace MusicCom
{
    const int MAX_MACRO_COUNT = 24;
    const int MAX_COMMANDS_PER_FRAME = 10000;

    PartSequencerBase::PartSequencerBase(const RegisterWriter& register_writer, const MusicData& music, CommandIterator command_tail, int rate)
        : register_writer_(register_writer),
          music_data_(music),
          command_tail_(command_tail),
          rate_(rate)
    {
    }

    PartSequencerBase::~PartSequencerBase() = default;

    void PartSequencerBase::Initialize()
    {
        permanently_stopped_ = false;
        command_limit_exceeded_ = false;
        part_data_.Playback.Playing = true;
        part_data_.Playback.HasPreviousNote = false;
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
        return part_data_.Playback.Playing;
    }

    void PartSequencerBase::Resume()
    {
        // 再生中は何もしない
        if (IsPlaying() || permanently_stopped_)
        {
            return;
        }

        part_data_.Commands.CommandPtr++;
        part_data_.Playback.Playing = true;

        // パート終端の場合は先頭に戻して再開
        if (part_data_.Commands.CommandPtr == command_tail_)
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
        // NVI pattern
        IncreaseFrameImpl(frame_size);
    }
    void PartSequencerBase::IncreaseFrameImpl(int frame_size)
    {
        if (!part_data_.Playback.Playing)
        {
            return;
        }

        samples_left_ -= frame_size;
        if (samples_left_ <= 0)
        {
            samples_left_ = samples_per_frame_;
            NextCommandFrame();
        }
    }

    void PartSequencerBase::NextCommandFrame()
    {
        ProcessCommandFrame(current_frame_);

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
        const int timer_period = 5880 / std::max(tempo, 30);

        // Timer Bのレジスタ値は(-timer_period) & 0xffで設定される
        // 商が0の場合はレジスタ値0、すなわち周期256として動作する
        const int effective_period = timer_period == 0 ? 256 : timer_period;
        return static_cast<int>(rate_ * 3.0 * effective_period / 5200.0);
    }

    int PartSequencerBase::ResolveNoteLength(const PartData& part_data, int base_length)
    {
        return base_length == 0 ? part_data.Note.DefaultLength : base_length;
    }

    void PartSequencerBase::UpdateNoteFrames(PartData& part_data, int current_frame, int length)
    {
        part_data.Note.BeginFrame = current_frame;
        part_data.Note.EndFrame = current_frame + length;
    }

    void PartSequencerBase::ReturnToHead()
    {
        part_data_.Commands.CommandPtr = GetHead();
        part_data_.Commands.CallStack = {};
        part_data_.Commands.LoopStack = {};
    }

    void PartSequencerBase::ProcessCommandFrame(int current_frame)
    {
        PreProcess(current_frame);
        ProcessCommand(current_frame);
    }

    void PartSequencerBase::PreProcess(int current_frame)
    {
        if (part_data_.Playback.HasPreviousNote && part_data_.Note.EndFrame <= current_frame)
        {
            part_data_.Note.LastOctave = part_data_.Note.Octave;
            part_data_.Note.LastTone = part_data_.Note.Tone;
        }
    }

    std::optional<CommandIterator> PartSequencerBase::ProcessLoop(CommandIterator ptr)
    {
        while (1)
        {
            if (!ConsumeCommand())
                return std::nullopt;
            const Command& command = *ptr;

            switch (command.GetType())
            {
            case CommandType::TYPE_PAUSE:
                part_data_.Playback.Playing = false;
                part_data_.Commands.CommandPtr = ptr;
                return std::nullopt;
            case CommandType::TYPE_MACRO:
                if (!music_data_.IsMacroPresent(command.GetStrArg()))
                {
                    ptr++;
                    continue;
                }
                part_data_.Commands.CallStack.push(++CommandIterator(ptr));
                // 自己参照等で入れ子の数がMAX_MACRO_COUNTを超えたら強制終了
                if (part_data_.Commands.CallStack.size() > MAX_MACRO_COUNT)
                {
                    part_data_.Playback.Playing = false;
                    return std::nullopt;
                }
                ptr = music_data_.GetMacroHead(command.GetStrArg());
                continue;
            case CommandType::TYPE_RETURN:
                if (part_data_.Commands.CallStack.empty())
                {
                    part_data_.Playback.Playing = false;
                    return std::nullopt;
                }
                ptr = part_data_.Commands.CallStack.top();
                part_data_.Commands.CallStack.pop();
                continue;
            case CommandType::TYPE_LOOP:
                part_data_.Commands.LoopStack.push(std::pair<CommandIterator, int>(++CommandIterator(ptr), command.GetArg(0)));
                ptr++;
                continue;
            case CommandType::TYPE_EXIT:
            {
                if (part_data_.Commands.LoopStack.empty())
                {
                    part_data_.Playback.Playing = false;
                    return std::nullopt;
                }
                std::pair<CommandIterator, int>& p = part_data_.Commands.LoopStack.top();
                if (p.second != 0)
                {
                    p.second--;
                    // ループ脱出
                    if (p.second == 0)
                    {
                        part_data_.Commands.LoopStack.pop();
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
                    if (part_data_.Playback.InfiniteLooping)
                    {
                        part_data_.Playback.Playing = false;
                        return std::nullopt;
                    }
                    part_data_.Playback.InfiniteLooping = true;
                    ptr = p.first;
                }
            }
                continue;
            default:
                return ptr;
            }
        }
    }

    std::optional<CommandType> PartSequencerBase::FindLinkedItem(CommandIterator ptr)
    {
        // 後方にタイ(&)やキーオフなし休符(W)が存在するかどうかを先読みして確認
        // ProcessLoop同様にマクロ/ループは展開するが、本体に影響しないようコピーで処理する
        bool infinite_looping = false;
        auto macro_call_stack(part_data_.Commands.CallStack);
        auto loop_stack(part_data_.Commands.LoopStack);

        while (1)
        {
            if (!ConsumeCommand())
                return std::nullopt;
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

    bool PartSequencerBase::ConsumeCommand()
    {
        if (remaining_commands_ == 0)
        {
            command_limit_exceeded_ = true;
            return false;
        }
        --remaining_commands_;
        return true;
    }

    bool PartSequencerBase::IsCommandLimitExceeded() const
    {
        return command_limit_exceeded_;
    }

    void PartSequencerBase::StopForCommandLimit()
    {
        permanently_stopped_ = true;
        part_data_.Playback.Playing = false;
        KeyOff();
    }

    void PartSequencerBase::ProcessCommand(int current_frame)
    {
        remaining_commands_ = MAX_COMMANDS_PER_FRAME;
        command_limit_exceeded_ = false;
        part_data_.Playback.InfiniteLooping = false;
        auto ptr = part_data_.Commands.CommandPtr;

        while (part_data_.Note.EndFrame <= current_frame)
        {
            // マクロ・ループ・終端は別処理
            {
                auto result = ProcessLoop(ptr);
                if (command_limit_exceeded_)
                {
                    StopForCommandLimit();
                    return;
                }
                if (!result)
                {
                    if (part_data_.Playback.HasPreviousNote && !part_data_.Commands.LinkedItem)
                    {
                        KeyOff();
                    }
                    return;
                }
                ptr = *result;
            }

            ptr = ProcessCommandImpl(ptr, current_frame, part_data_);
            if (command_limit_exceeded_)
            {
                StopForCommandLimit();
                return;
            }
        }
        part_data_.Commands.CommandPtr = ptr;
    }

} // namespace MusicCom
