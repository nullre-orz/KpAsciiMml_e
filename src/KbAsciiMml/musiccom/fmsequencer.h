#pragma once

#include "musdata.h"
#include "tonesequencerbase.h"
#include <array>

namespace MusicCom
{
    class FMWrap;
    class FmSequencer : public ToneSequencerBase
    {
    public:
        FmSequencer(const RegisterWriter& register_writer, FMWrap& fmwrap, const MusicData& music, int channel, int rate);
        ~FmSequencer() override;

    protected: // for PartSequencerBase
        CommandIterator ProcessCommandImpl(CommandIterator ptr, int current_frame, PartData& part_data) override;
        void ProcessEffect(int current_frame) override;

    private: // for PartSequencerBase
        void InitializeImpl(PartData& part_data) override;
        void KeyOn() override;
        void KeyOff() override;
        bool NeedsVolumeEffect(const PartData& part_data) const override;
        void UpdateTone(int base_tone, PartData& part_data) override;
        void ApplyVibratoEffect(int octave, int tone, int depth) override;
        void ApplyPortamentoEffect(int octave, int tone, int last_octave, int last_tone, int tick, int length) override;
        void SetTone(int octave, int tone) override;
        void SetVolume(int volume) override;
        CommandIterator GetHead() const override;

    private:
        int CalculateTone(int base_tone, int detune) const;
        int CalculateFNumberOffset(int note, int depth) const;
        int GetOperatorFNumber(int op, int note, int tone) const;
        void InitializeSoundLFO(int note);
        bool UpdateSoundLFO();
        int GetSoundLFOAmplitude() const;
        int GetSoundLFOOffset() const;
        void WriteTone(int octave, int tone);

        int channel_;
        FMWrap& fmwrap_;
        int sound_no_;
        int lfo_accumulator_;
        int lfo_phase_;
        int lfo_value_;
        int lfo_note_;
        int last_note_;
        int current_octave_;
        int current_tone_;
        std::array<int, FMSound::OPERATOR_COUNT> current_operator_fnumber_;
        bool operator_portamento_active_;
        bool note_active_;
    };
} // namespace MusicCom
