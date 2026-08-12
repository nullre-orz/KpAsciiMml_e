#pragma once

#include "partsequencerbase.h"

namespace MusicCom
{
    class FMWrap;
    class MusicData;
    class FmSequencer : public PartSequencerBase
    {
    public:
        FmSequencer(const RegisterWriter& register_writer, FMWrap& fmwrap, const MusicData& music, int channel, int rate);
        ~FmSequencer();

    protected: // for PartSequencerBase
        virtual CommandIterator ProcessCommandImpl(CommandIterator ptr, int current_frame, PartData& part_data);
        virtual void ProcessEffect(int current_frame);

    private: // for PartSequencerBase
        virtual void InitializeImpl(PartData& part_data);
        virtual void KeyOn();
        virtual void KeyOff();
        virtual void UpdateTone(int base_tone, PartData& part_data);
        virtual void ApplyVibratoEffect(int octave, int tone, int depth);
        virtual void ApplyPortamentoEffect(int octave, int tone, int last_octave, int last_tone, int tick, int length);
        virtual void SetTone(int octave, int tone);
        virtual void SetVolume(int volume);
        virtual const CommandIterator GetHead() const;

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
        int current_operator_fnumber_[4];
        bool operator_portamento_active_;
        bool note_active_;
    };
} // namespace MusicCom
