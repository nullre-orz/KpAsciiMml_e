#pragma once

#include "partsequencerbase.h"
#include "sounddata.h"
#include <functional>
#include <optional>
#include <vector>

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
        SoundSequencer(FM::OPN& opn, SSGWrap& ssgwrap, const MusicData& music, const SoundData& sound, int soundtempo, int rate);
        ~SoundSequencer();

        enum class PlayStatus : int
        {
            STOP,
            PLAYING
        };
        using PlayStatusObserver = std::function<void(PlayStatus)>;
        void AppendPlayStatusObserver(PlayStatusObserver observer);

    protected: // for PartSequencerBase
        virtual int GetRemainFrameSizeImpl();
        virtual void IncreaseFrameImpl(int frame_size);
        virtual CommandIterator ProcessCommandImpl(CommandIterator ptr, int current_frame, PartData& part_data);

    private:
        void NextSoundFrame();

    private: // for PartSequencerBase
        virtual void InitializeImpl(PartData& part_data);
        virtual void PreProcess(int current_frame);
        virtual void ProcessEffect(int current_frame);
        virtual void KeyOn();
        virtual void KeyOff();
        virtual void UpdateTone(int base_tone, PartData& part_data);
        virtual void ApplyPortamentoEffect(int octave, int tone, int last_octave, int last_tone, double coefficient);
        virtual void SetTone(int octave, int tone);
        virtual void SetVolume(int volume);
        virtual const CommandIterator GetHead() const;

        void KeyOnOffImpl(bool on);

    private:
        SSGWrap& ssgwrap_;
        const SoundData& sound_;

        std::vector<PlayStatusObserver> observer_list_;

        struct CurrentSoundData
        {
            RhythmData::const_iterator ptr;
            RhythmData::const_iterator end_ptr;
        };
        std::optional<CurrentSoundData> current_sound_data_;

        std::function<CommandIterator()> GetHeadImpl;

        // 効果音フレーム
        bool sound_interrupt_enabled_;
        int sound_interrupt_per_frame_;
        int sound_interrupt_left_;
    };
} // namespace MusicCom
