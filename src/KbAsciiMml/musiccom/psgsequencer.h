#pragma once

#include "tonesequencerbase.h"

namespace MusicCom
{
    class SSGWrap;
    class MusicData;
    class PsgSequencer : public ToneSequencerBase
    {
    public:
        PsgSequencer(const RegisterWriter& register_writer, SSGWrap& ssgwrap, const MusicData& music, int channel, int rate);
        ~PsgSequencer() override;

    protected: // for PartSequencerBase
        CommandIterator ProcessCommandImpl(CommandIterator ptr, int current_frame, PartData& part_data) override;

    private: // for PartSequencerBase
        void InitializeImpl(PartData& part_data) override;
        void KeyOn() override;
        void KeyOff() override;
        void UpdateTone(int base_tone, PartData& part_data) override;
        bool NeedsVolumeEffect(const PartData& part_data) const override;
        int AdjustVolumeEffect(int volume, int keyon_length, const PartData& part_data) override;
        void ApplyVibratoEffect(int octave, int tone, int depth) override;
        void ApplyPortamentoEffect(int octave, int tone, int last_octave, int last_tone, int tick, int length) override;
        void SetTone(int octave, int tone) override;
        void SetVolume(int volume) override;
        CommandIterator GetHead() const override;

    private:
        int CalculateTone(int base_octave, int base_tone, int detune) const;
        int CalculateTonePeriod(int note, int depth) const;
        int CalculateTonePeriodOffset(int note, int depth) const;
        int ApplyOctave(int period, int octave) const;

        int channel_;
        SSGWrap& ssgwrap_;
        int current_note_ = 0;
        int last_period_;
        int current_period_;
    };
} // namespace MusicCom
