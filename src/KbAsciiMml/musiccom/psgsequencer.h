#pragma once

#include "partsequencerbase.h"
#include "soundsequencer.h"
#include <functional>

namespace MusicCom
{
    class SSGWrap;
    class MusicData;
    struct SSGEnv;
    class PsgSequencer : public PartSequencerBase
    {
    public:
        PsgSequencer(FM::OPN& opn, SSGWrap& ssgwrap, const MusicData& music, int channel);
        ~PsgSequencer();

        void UpdateDeterrence(SoundSequencer::PlayStatus status);

    protected: // for PartSequencerBase
        virtual CommandIterator ProcessCommandImpl(CommandIterator ptr, int current_frame, PartData& part_data);

    private: // for PartSequencerBase
        virtual int InitializeTone();
        virtual void KeyOn();
        virtual void KeyOff();
        virtual void UpdateTone(int base_tone, PartData& part_data);
        virtual int AdjustVolume(int volume, int length, const PartData& part_data);
        virtual void ApplyPortamentoEffect(int octave, int tone, int last_octave, int last_tone, double coefficient);
        virtual void SetTone(int octave, int tone);
        virtual void SetVolume(int volume);
        virtual const CommandIterator GetHead() const;

    private:
        int CalculateTone(int base_octave, int base_tone, int detune) const;

        int channel_;
        SSGWrap& ssgwrap_;
        bool ring_deterrence_;

        std::function<const SSGEnv&(int)> GetSSGEnv;
        std::function<CommandIterator()> GetHeadImpl;
    };
} // namespace MusicCom
