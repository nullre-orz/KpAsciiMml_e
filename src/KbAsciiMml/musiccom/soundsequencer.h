#pragma once

#include "partsequencerbase.h"
#include "sounddata.h"
#include <optional>

namespace MusicCom
{
    class SSGWrap;
    class MusicData;
    struct FMSound;
    class SoundSequencer : public PartSequencerBase
    {
    public:
        SoundSequencer(const RegisterWriter& register_writer, SSGWrap& ssgwrap, const MusicData& music, const SoundData& sound, int soundtempo, int rate);
        ~SoundSequencer() override;

    protected: // for PartSequencerBase
        int GetRemainFrameSizeImpl() override;
        void IncreaseFrameImpl(int frame_size) override;
        CommandIterator ProcessCommandImpl(CommandIterator ptr, int current_frame, PartData& part_data) override;

    private:
        void NextSoundFrame();

    private: // for PartSequencerBase
        void InitializeImpl(PartData& part_data) override;
        void PreProcess(int current_frame) override;
        void ProcessEffect(int current_frame) override;
        void KeyOn() override;
        void KeyOff() override;
        void UpdateTone(int base_tone, PartData& part_data) override;
        void ApplyPortamentoEffect(int octave, int tone, int last_octave, int last_tone, int tick, int length) override;
        void SetTone(int octave, int tone) override;
        void SetVolume(int volume) override;
        const CommandIterator GetHead() const override;

        void KeyOnOffImpl(bool on);

    private:
        SSGWrap& ssgwrap_;
        const SoundData& sound_;

        struct CurrentSoundData
        {
            RhythmData::const_iterator ptr;
            RhythmData::const_iterator end_ptr;
        };
        std::optional<CurrentSoundData> current_sound_data_;

        // 効果音フレーム
        bool sound_interrupt_enabled_;
        int sound_interrupt_per_frame_;
        int sound_interrupt_left_;
    };
} // namespace MusicCom
