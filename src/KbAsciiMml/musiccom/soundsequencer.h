#pragma once

#include "partsequencerbase.h"
#include "sounddata.h"
#include <functional>
#include <optional>

namespace FM
{
    class OPN;
}

namespace MusicCom
{
    class SSGWrap;
    class MusicData;
    struct FMSound;
    class SoundSequencer : public PartSequencerBase
    {
    public:
        SoundSequencer(FM::OPN& opn, SSGWrap& ssgwrap, const MusicData& music, const SoundData& sound);
        ~SoundSequencer();

    protected: // for PartSequencerBase
        virtual CommandIterator ProcessCommandImpl(CommandIterator ptr, int current_frame, PartData& part_data);

    private: // for PartSequencerBase
        virtual int InitializeTone();
        virtual void PreProcess(int current_frame);
        virtual void ProcessEffect(int current_frame);
        virtual void KeyOn();
        virtual void KeyOff();
        virtual void UpdateTone(int base_tone, PartData& part_data);
        virtual void ApplyPortamentoEffect(int octave, int tone, int last_octave, int last_tone, double coefficient);
        virtual void SetTone(int octave, int tone);
        virtual void SetVolume(int volume);
        virtual const CommandIterator GetHead() const;

    private:
        SSGWrap& ssgwrap_;
        const SoundData& sound_;

        struct CurrentSoundData
        {
            RhythmData::const_iterator ptr;
            RhythmData::const_iterator end_ptr;
        };
        std::optional<CurrentSoundData> current_sound_data_;

        std::function<CommandIterator()> GetHeadImpl;
    };
} // namespace MusicCom
