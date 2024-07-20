#include "soundsequencer.h"
#include "fmwrap.h"
#include <algorithm>
#include <cmath>

namespace MusicCom
{
    // clang-format off
    const int NOTE_SOUND_MAP[] = {
         2, // C
        -1, // C#
         3, // D
        -1, // D#
         4, // E
         5, // F
        -1, // F#
         6, // G
        -1, // G#
         0, // A
        -1, // A#
         1  // B
    };
    // clang-format on

    SoundSequencer::SoundSequencer(FM::OPN& opn, SSGWrap& ssgwrap, const MusicData& music, const SoundData& sound, int soundtempo, int rate)
        : PartSequencerBase(opn, music, music.GetRhythmPartTail(), rate),
          ssgwrap_(ssgwrap),
          sound_(sound),
          current_sound_data_(std::nullopt),
          GetHeadImpl([this]()
                      { return GetMusicData().GetRhythmPartHead(); }),
          sound_interrupt_enabled_(false),
          sound_interrupt_per_frame_(CalculatePerFrame(soundtempo)),
          sound_interrupt_left_(0)
    {
    }

    SoundSequencer::~SoundSequencer()
    {
        observer_list_.clear();
    }

    void SoundSequencer::InitializeImpl(PartData& part_data)
    {
        // 効果音フレーム初期化
        sound_interrupt_enabled_ = false;
        sound_interrupt_left_ = 0;
    }

    int SoundSequencer::GetRemainFrameSizeImpl()
    {
        // SoundSequencerでは効果音フレームとコマンドフレームを同時に扱う
        // 効果音再生中は効果音フレームの残時間とコマンドフレームの残時間のうち小さい方を返す

        // コマンドフレームの残時間は基底クラスから取得
        int command_frame_size = PartSequencerBase::GetRemainFrameSizeImpl();
        if (sound_interrupt_enabled_)
        {
            return std::min(sound_interrupt_left_, command_frame_size);
        }
        return command_frame_size;
    }

    void SoundSequencer::IncreaseFrameImpl(int frame_size)
    {
        // コマンドフレームの処理
        PartSequencerBase::IncreaseFrameImpl(frame_size);

        // 効果音フレームの処理
        if (sound_interrupt_enabled_)
        {
            sound_interrupt_left_ -= frame_size;
            if (sound_interrupt_left_ <= 0)
            {
                sound_interrupt_left_ = sound_interrupt_per_frame_;
                NextSoundFrame();
            }
        }
    }

    void SoundSequencer::NextSoundFrame()
    {
        if (!current_sound_data_)
        {
            return;
        }

        // optinal外し
        auto& current_sound = *current_sound_data_;

        // 終端の場合はクリア
        if (current_sound.ptr == current_sound.end_ptr)
        {
            current_sound_data_ = std::nullopt;
            KeyOff();
            return;
        }

        // 効果音の設定
        const auto& data = *current_sound.ptr;
        ssgwrap_.SetNoisePeriod(data.noise_period);
        for (int ch = 0; ch < 2; ch++)
        {
            ssgwrap_.SetTonePeriod(ch, data.tone[ch]);
            ssgwrap_.SetVolume(ch, data.volume[ch]);
            ssgwrap_.SetToneEnabled(ch, data.tone_enabled[ch]);
            ssgwrap_.SetNoiseEnabled(ch, data.noise_enabled[ch]);
        }
        ssgwrap_.SetNoiseToneEnable();

        ++current_sound.ptr;
    }

    void SoundSequencer::AppendPlayStatusObserver(PlayStatusObserver observer)
    {
        observer_list_.push_back(observer);
    }

    void SoundSequencer::PreProcess(int current_frame)
    {
        // nothing todo.
    }

    CommandIterator SoundSequencer::ProcessCommandImpl(CommandIterator ptr, int current_frame, PartData& part_data)
    {
        // 原則基底クラスではなく本クラスで処理する
        static auto get_length = [](const PartData& part_data, int base_length)
        {
            return (base_length == 0) ? part_data.DefaultNoteLength : base_length;
        };

        static auto update_note_frames = [](PartData& part_data, int current_frame, int length)
        {
            part_data.NoteBeginFrame = current_frame;
            part_data.NoteEndFrame = current_frame + length;
        };

        auto return_ptr = CommandIterator(ptr);
        const Command& command = *return_ptr++;
        switch (command.GetType())
        {
        case CommandType::TYPE_NOTE:
        {
            part_data.KeyOnFrame = current_frame;

            // 効果音の設定
            auto sound_no = NOTE_SOUND_MAP[command.GetArg(0)];
            auto& sound_data = sound_.GetRhythm(sound_no);
            if (sound_data.length() > 0)
            {
                current_sound_data_ = {sound_data.begin(), sound_data.end()};
                NextSoundFrame();
                KeyOn();
            }

            int length = get_length(part_data, command.GetArg(1));
            update_note_frames(part_data, current_frame, length);

            part_data.KeyOffFrame = sound_data.length();
            break;
        }
        case CommandType::TYPE_REST:
        case CommandType::TYPE_WAIT:
        {
            part_data.KeyOnFrame = current_frame;

            int length = get_length(part_data, command.GetArg(0));
            update_note_frames(part_data, current_frame, length);

            part_data.KeyOffFrame = part_data.NoteEndFrame;
            break;
        }
        case CommandType::TYPE_LENGTH:
            part_data.DefaultNoteLength = command.GetArg(0);
            break;
        }
        return return_ptr;
    }

    void SoundSequencer::ProcessEffect(int current_frame)
    {
        // nothing todo.
    }

    void SoundSequencer::KeyOn()
    {
        KeyOnOffImpl(true);

        // コマンドフレームの残時間を設定
        // (有効/無効はImpl内で設定するため省略)
        sound_interrupt_left_ = sound_interrupt_per_frame_;
    }

    void SoundSequencer::KeyOff()
    {
        // トーンを有効化, ノイズを無効化しておく
        ssgwrap_.SetToneEnabled(0, true);
        ssgwrap_.SetToneEnabled(1, true);
        ssgwrap_.SetNoiseEnabled(0, false);
        ssgwrap_.SetNoiseEnabled(1, false);

        KeyOnOffImpl(false);
    }

    void SoundSequencer::KeyOnOffImpl(bool on)
    {
        // チャンネル4,5(SSGチャンネルA,B)を流用
        ssgwrap_.KeyOnOff(0, on);
        ssgwrap_.KeyOnOff(1, on);

        sound_interrupt_enabled_ = on;

        // 通知
        for (auto item : observer_list_)
        {
            item((on) ? PlayStatus::PLAYING : PlayStatus::STOP);
        }
    }

    void SoundSequencer::UpdateTone(int base_tone, PartData& part_data)
    {
        // nothing todo.
    }

    void SoundSequencer::ApplyPortamentoEffect(int octave, int tone, int last_octave, int last_tone, double coefficient)
    {
        // nothing todo.
    }

    void SoundSequencer::SetTone(int octave, int tone)
    {
        // nothing todo.
    }

    void SoundSequencer::SetVolume(int volume)
    {
        // nothing todo.
    }

    const CommandIterator SoundSequencer::GetHead() const
    {
        return GetHeadImpl();
    }

} // namespace MusicCom
