#pragma once

#include "partsequencerbase.h"
#include <functional>

namespace FM
{
    class OPN;
}

namespace MusicCom
{
    class FMWrap;
    class MusicData;
    struct FMSound;
    class FmSequencer : public PartSequencerBase
    {
    public:
        FmSequencer(FM::OPN& opn, FMWrap& fmwrap, const MusicData& music, int channel, int rate);
        ~FmSequencer();

    protected: // for PartSequencerBase
        virtual CommandIterator ProcessCommandImpl(CommandIterator ptr, int current_frame, PartData& part_data);

    private: // for PartSequencerBase
        virtual void InitializeImpl(PartData& part_data);
        virtual void KeyOn();
        virtual void KeyOff();
        virtual void UpdateTone(int base_tone, PartData& part_data);
        virtual void ApplyPortamentoEffect(int octave, int tone, int last_octave, int last_tone, double coefficient);
        virtual void SetTone(int octave, int tone);
        virtual void SetVolume(int volume);
        virtual const CommandIterator GetHead() const;

    private:
        int CalculateTone(int base_tone, int detune) const;

        int channel_;
        FMWrap& fmwrap_;

        std::function<const FMSound&(int)> GetSound;
        std::function<CommandIterator()> GetHeadImpl;
    };
} // namespace MusicCom
