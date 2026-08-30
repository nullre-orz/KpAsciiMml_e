#pragma once

#include "partsequencerbase.h"

namespace MusicCom
{
    class ToneSequencerBase : public PartSequencerBase
    {
    protected:
        // PartSequencerBaseのコンストラクタを継承する
        using PartSequencerBase::PartSequencerBase;

        CommandIterator ProcessCommandImpl(CommandIterator ptr, int current_frame, PartData& part_data) override;
        virtual void ProcessEffect(int current_frame);

    private:
        void ProcessCommandFrame(int current_frame) override;
        void ProcessVolumeEffect(int current_frame);
        virtual bool NeedsVolumeEffect(const PartData& part_data) const = 0;
        virtual int AdjustVolumeEffect(int volume, int keyon_length, const PartData& part_data);
        virtual void ApplyVibratoEffect(int octave, int tone, int depth);
        virtual void UpdateTone(int base_tone, PartData& part_data) = 0;
        virtual void ApplyPortamentoEffect(int octave, int tone, int last_octave, int last_tone, int tick, int length) = 0;
        virtual void SetTone(int octave, int tone) = 0;
        virtual void SetVolume(int volume) = 0;
    };
} // namespace MusicCom
