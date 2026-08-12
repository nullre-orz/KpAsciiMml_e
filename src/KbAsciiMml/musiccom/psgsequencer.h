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
        PsgSequencer(const RegisterWriter& register_writer, SSGWrap& ssgwrap, const MusicData& music, int channel, int rate);
        ~PsgSequencer();

        void UpdateDeterrence(SoundSequencer::PlayStatus status);

    protected: // for PartSequencerBase
        virtual CommandIterator ProcessCommandImpl(CommandIterator ptr, int current_frame, PartData& part_data);
        virtual void ProcessEffect(int current_frame);

    private: // for PartSequencerBase
        virtual void InitializeImpl(PartData& part_data);
        virtual void KeyOn();
        virtual void KeyOff();
        virtual void UpdateTone(int base_tone, PartData& part_data);
        virtual int AdjustVolume(int volume, int length, const PartData& part_data);
        virtual void ApplyVibratoEffect(int octave, int tone, int depth);
        virtual void ApplyPortamentoEffect(int octave, int tone, int last_octave, int last_tone, int tick, int length);
        virtual void SetTone(int octave, int tone);
        virtual void SetVolume(int volume);
        virtual const CommandIterator GetHead() const;

    private:
        int CalculateTone(int base_octave, int base_tone, int detune) const;
        int CalculateTonePeriod(int note, int depth) const;
        int CalculateTonePeriodOffset(int note, int depth) const;
        int ApplyOctave(int period, int octave) const;

        int channel_;
        SSGWrap& ssgwrap_;
        bool ring_deterrence_;
        int current_note_;
        int last_period_;
        int current_period_;

        std::function<const SSGEnv&(int)> GetSSGEnv;
        std::function<CommandIterator()> GetHeadImpl;
    };
} // namespace MusicCom
