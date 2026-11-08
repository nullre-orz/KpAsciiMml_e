#pragma once

#include "partdata.h"
#include <functional>

namespace MusicCom
{
    class MusicData;
    using RegisterWriter = std::function<void(int, int)>;

    class PartSequencerBase
    {
    public:
        PartSequencerBase(const RegisterWriter& register_writer, const MusicData& music, CommandIterator command_tail, int rate);
        virtual ~PartSequencerBase();

        void Initialize();
        bool IsPlaying() const;
        void Resume();

        int GetRemainFrameSize();
        void IncreaseFrame(int frame_size);

    protected:
        const MusicData& GetMusicData() const;
        int CalculatePerFrame(int tempo);
        static int ResolveNoteLength(const PartData& part_data, int base_length);
        static void UpdateNoteFrames(PartData& part_data, int current_frame, int length);

        virtual int GetRemainFrameSizeImpl();
        virtual void IncreaseFrameImpl(int frame_size);
        virtual void ProcessCommandFrame(int current_frame);
        std::optional<CommandType> FindLinkedItem(CommandIterator ptr);
        bool IsCommandLimitExceeded() const;
        virtual void KeyOn() = 0;
        virtual void KeyOff() = 0;

    private:
        void ReturnToHead();
        void NextCommandFrame();
        std::optional<CommandIterator> ProcessLoop(CommandIterator ptr);
        void ProcessCommand(int current_frame);
        bool ConsumeCommand();
        void StopForCommandLimit();

        virtual void InitializeImpl(PartData& part_data) = 0;
        virtual void PreProcess(int current_frame);
        virtual CommandIterator ProcessCommandImpl(CommandIterator ptr, int current_frame, PartData& part_data) = 0;
        virtual CommandIterator GetHead() const = 0;

    protected:
        RegisterWriter register_writer_;
        PartData part_data_{};
        const MusicData& music_data_;
        const CommandIterator command_tail_;

    private:
        const int rate_;
        // コマンドフレーム
        int samples_per_frame_ = 0;
        int samples_left_ = 0; // このフレーム(64分音符)でmixすべき残りのサンプル数
        int current_frame_ = 0;
        int remaining_commands_ = 0;
        bool command_limit_exceeded_ = false;
        bool permanently_stopped_ = false;
    };
} // namespace MusicCom
