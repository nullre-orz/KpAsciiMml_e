#include "fmsequencer.h"
#include "fmwrap.h"
#include <algorithm>
#include <cmath>

namespace MusicCom
{
    enum SoundLFOForm
    {
        SOUND_LFO_SQUARE = 0,
        SOUND_LFO_SAWTOOTH = 1,
        SOUND_LFO_TRIANGLE = 2,
        SOUND_LFO_ONE_SHOT = 3
    };

    const int SOUND_LFO_SAWTOOTH_RESET_PHASE = 5;
    const int SOUND_LFO_TRIANGLE_FALL_PHASE = 5;
    const int SOUND_LFO_TRIANGLE_RESET_PHASE = 8;
    const int SOUND_LFO_ONE_SHOT_END_PHASE = 3;
    const int SOUND_LFO_ONE_SHOT_STOP_PHASE = 6;
    const int SOUND_LFO_ACCUMULATOR_SIZE = 0x100;

    // clang-format off
    // 音名ごとのLFO深度からF-numberへの変換係数
    const int SOUND_LFO_FNUMBER_COEFFICIENT[12] = {
        //C,   C#,    D,   D#,    E,    F,   F#,   G,    G#,    A,   A#,    B
        123, -125, -118, -109, -101,  -91,  -82,  -71,  -60,  -48,  -36,  -23
    };
    // clang-format on

    int ToSignedByte(int value)
    {
        // byte値として扱うため上位bitを捨て、符号付き8bitへ変換する
        value &= 0xff;
        return value < 0x80 ? value : value - 0x100;
    }

    int WrapSignedWord(int value)
    {
        // 16bit演算を再現するため、上位bitを捨てて符号付き16bitへ変換する
        value &= 0xffff;
        return value < 0x8000 ? value : value - 0x10000;
    }

    // clang-format off
    const int F_NUMBER_BASE[14] = {
        //C-,  C,  C#,   D,  D#,   E,   F,  F#,   G,  G#,   A,  A#,   B,  B#
        584, 618, 655, 694, 735, 779, 825, 874, 926, 981,1040,1101,1167,1236
    };
    // clang-format on
    const int* const F_NUMBER = &F_NUMBER_BASE[1];

    FmSequencer::FmSequencer(FM::OPN& opn, FMWrap& fmwrap, const MusicData& music, int channel, int rate)
        : PartSequencerBase(opn, music, music.GetChannelTail(channel), rate),
          channel_(channel),
          fmwrap_(fmwrap),
          sound_no_(0),
          lfo_accumulator_(0),
          lfo_phase_(0),
          lfo_value_(0),
          lfo_note_(0),
          current_octave_(0),
          current_tone_(0),
          note_active_(false),
          GetSound([this](int no) -> const FMSound&
                   { return GetMusicData().GetFMSound(no); }),
          GetHeadImpl([this, channel]()
                      { return GetMusicData().GetChannelHead(channel); })
    {
    }

    FmSequencer::~FmSequencer()
    {
    }

    void FmSequencer::InitializeImpl(PartData& part_data)
    {
        // 初手ポルタメント対応
        part_data.LastOctave = 0;
        part_data.LastTone = CalculateTone(0, 0);
        sound_no_ = 0;
        lfo_accumulator_ = 0;
        lfo_phase_ = 0;
        lfo_value_ = 0;
        lfo_note_ = 0;
        current_octave_ = 0;
        current_tone_ = 0;
        note_active_ = false;
    }

    CommandIterator FmSequencer::ProcessCommandImpl(CommandIterator ptr, int current_frame, PartData& part_data)
    {
        auto return_ptr = CommandIterator(ptr);
        const Command& command = *return_ptr++;
        switch (command.GetType())
        {
        case CommandType::TYPE_TONE:
            part_data.SoundNo = command.GetArg(0);
            sound_no_ = part_data.SoundNo;
            fmwrap_.SetSound(channel_, GetSound(part_data.SoundNo));
            lfo_value_ = 0;
            break;
        case CommandType::TYPE_REST:
            note_active_ = false;
            return_ptr = PartSequencerBase::ProcessCommandImpl(ptr, current_frame, part_data);
            break;
        default:
            return_ptr = PartSequencerBase::ProcessCommandImpl(ptr, current_frame, part_data);
            break;
        }
        return return_ptr;
    }

    void FmSequencer::KeyOn()
    {
        fmwrap_.KeyOnOff(channel_, true);
    }

    void FmSequencer::KeyOff()
    {
        fmwrap_.KeyOnOff(channel_, false);
    }

    void FmSequencer::UpdateTone(int base_tone, PartData& part_data)
    {
        InitializeSoundLFO(base_tone);
        part_data.Tone = CalculateTone(base_tone, part_data.Detune);
        SetTone(part_data.Octave, part_data.Tone);
    }

    void FmSequencer::ProcessEffect(int current_frame)
    {
        PartSequencerBase::ProcessEffect(current_frame);

        if (UpdateSoundLFO())
        {
            WriteTone(current_octave_, current_tone_);
        }
    }

    void FmSequencer::ApplyPortamentoEffect(int octave, int tone, int last_octave, int last_tone, int tick, int length)
    {
        if (tick == length + 1)
        {
            SetTone(octave, tone);
            return;
        }

        int block = std::max(octave, last_octave);
        int initial_tone = last_tone >> (block - last_octave);
        int target_tone = tone >> (block - octave);
        int delta = (target_tone - initial_tone) / (length + 1);
        SetTone(block, initial_tone + delta * tick);
    }

    void FmSequencer::SetTone(int octave, int tone)
    {
        current_octave_ = octave;
        current_tone_ = tone;
        WriteTone(octave, tone);
    }

    void FmSequencer::InitializeSoundLFO(int note)
    {
        lfo_accumulator_ = 0;
        lfo_phase_ = 0;
        lfo_note_ = note;
        note_active_ = true;

        const FMSound& sound = GetSound(sound_no_);
        int amplitude = GetSoundLFOAmplitude();
        int scale = sound.LFOForm == SOUND_LFO_SQUARE ? 1 : 2;
        lfo_value_ = WrapSignedWord(-amplitude * scale);
    }

    bool FmSequencer::UpdateSoundLFO()
    {
        const FMSound& sound = GetSound(sound_no_);
        if (!note_active_ || sound.LFODepth == 0)
        {
            return false;
        }

        int accumulator = lfo_accumulator_ + sound.LFOSpeed;
        // SPEED累積値は1 byteのため、下位8bitだけを保持する。
        lfo_accumulator_ = accumulator & 0xff;
        if (accumulator < SOUND_LFO_ACCUMULATOR_SIZE)
        {
            return false;
        }

        // 位相は1 byteのため、255の次は0へ戻る。
        lfo_phase_ = (lfo_phase_ + 1) & 0xff;
        int amplitude = GetSoundLFOAmplitude();

        switch (sound.LFOForm)
        {
        case SOUND_LFO_SQUARE:
            lfo_value_ = (lfo_phase_ & 1) ? -amplitude : amplitude;
            break;
        case SOUND_LFO_SAWTOOTH:
            if (lfo_phase_ == SOUND_LFO_SAWTOOTH_RESET_PHASE)
            {
                lfo_value_ = WrapSignedWord(-2 * amplitude);
                lfo_phase_ = 0;
            }
            else
            {
                lfo_value_ = WrapSignedWord(lfo_value_ + amplitude);
            }
            break;
        case SOUND_LFO_TRIANGLE:
            if (lfo_phase_ < SOUND_LFO_TRIANGLE_FALL_PHASE)
            {
                lfo_value_ = WrapSignedWord(lfo_value_ + amplitude);
            }
            else if (lfo_phase_ == SOUND_LFO_TRIANGLE_RESET_PHASE)
            {
                lfo_value_ = WrapSignedWord(-2 * amplitude);
                lfo_phase_ = 0;
            }
            else
            {
                lfo_value_ = WrapSignedWord(lfo_value_ - amplitude);
            }
            break;
        case SOUND_LFO_ONE_SHOT:
            if (lfo_phase_ < SOUND_LFO_ONE_SHOT_END_PHASE)
            {
                lfo_value_ = WrapSignedWord(lfo_value_ + amplitude);
            }
            else
            {
                lfo_value_ = 0;
                lfo_phase_ = SOUND_LFO_ONE_SHOT_STOP_PHASE;
            }
            break;
        }

        return true;
    }

    int FmSequencer::GetSoundLFOAmplitude() const
    {
        const FMSound& sound = GetSound(sound_no_);
        return ToSignedByte(sound.LFODepth) * SOUND_LFO_FNUMBER_COEFFICIENT[lfo_note_];
    }

    int FmSequencer::GetSoundLFOOffset() const
    {
        if (lfo_value_ >= 0)
        {
            return lfo_value_ / SOUND_LFO_ACCUMULATOR_SIZE;
        }
        return -((-lfo_value_ + SOUND_LFO_ACCUMULATOR_SIZE - 1) / SOUND_LFO_ACCUMULATOR_SIZE);
    }

    void FmSequencer::WriteTone(int octave, int tone)
    {
        fmwrap_.SetTone(channel_, octave, tone, GetSoundLFOOffset());
    }

    void FmSequencer::SetVolume(int volume)
    {
        fmwrap_.SetVolume(channel_, volume);
    }

    const CommandIterator FmSequencer::GetHead() const
    {
        return GetHeadImpl();
    }

    int FmSequencer::CalculateTone(int base_tone, int detune) const
    {
        int tone = F_NUMBER[base_tone];
        if (detune != 0)
        {
            tone = static_cast<int>(tone * std::pow(2.0, detune / (255.0 * 12.0)) + 0.5);
        }
        return tone;
    }

} // namespace MusicCom
