#pragma once

#include "partdata.h"

namespace FM
{
    class OPN;
}

namespace MusicCom
{
    class MusicData;
    class PartSequencerBase
    {
    public:
        PartSequencerBase(FM::OPN& opn, const MusicData& music);
        virtual ~PartSequencerBase();

        void Initialize();
        bool IsPlaying() const;
        void NextFrame(int current_frame);
        void ReturnToHead();

    protected:
        const MusicData& GetMusicData() const;
        virtual CommandIterator ProcessCommandImpl(CommandIterator ptr, int current_frame, PartData& part_data);

    private:
        std::optional<CommandIterator> ProcessLoop(CommandIterator ptr);
        std::optional<CommandType> FindLinkedItem(CommandIterator ptr) const;
        void ProcessCommand(int current_frame);

        virtual int InitializeTone() = 0;
        virtual void ProcessEffect(int current_frame);
        virtual void KeyOn() = 0;
        virtual void KeyOff() = 0;
        virtual void UpdateTone(int base_tone, PartData& part_data) = 0;
        virtual int AdjustVolume(int volume, int length, const PartData& part_data);
        virtual void ApplyPortamentoEffect(int octave, int tone, int last_octave, int last_tone, double coefficient) = 0;
        virtual void SetTone(int octave, int tone) = 0;
        virtual void SetVolume(int volume) = 0;
        virtual const CommandIterator GetHead() const = 0;

        FM::OPN& opn_;
        PartData part_data_;
        const MusicData& music_data_;
    };
} // namespace MusicCom
