#include "fmsequencer.h"
#include "fmwrap.h"
#include <algorithm>

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
    // LFO深度からF-numberへの変換係数
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
    // DT2ごとのF-number
    const int F_NUMBER_TABLE[4][12] = {
        // C,  C#,   D,  D#,   E,   F,  F#,   G,  G#,   A,  A#,   B
        {617, 655, 694, 735, 779, 825, 874, 926, 981,1040,1102,1167}, // DT2=0
        {431, 458, 485, 514, 545, 577, 611, 648, 686, 728, 771, 816}, // DT2=1
        {493, 524, 555, 588, 623, 660, 699, 740, 784, 832, 881, 933}, // DT2=2
        {530, 563, 596, 632, 669, 709, 751, 796, 843, 894, 947,1003}  // DT2=3
    };

    // N/IからF-number補正値を求める係数
    const int F_NUMBER_ADJUST_COEFFICIENT[13] = {
        // C-, C, C#, D, D#, E, F, F#, G, G#, A, A#, B
           34,38, 39,41, 44,46,49, 52,55, 59,62, 65,67
    };
    // clang-format on

    FmSequencer::FmSequencer(const RegisterWriter& register_writer, FMWrap& fmwrap, const MusicData& music, int channel, int rate)
        : PartSequencerBase(register_writer, music, music.GetChannelTail(channel), rate),
          channel_(channel),
          fmwrap_(fmwrap),
          sound_no_(0),
          lfo_accumulator_(0),
          lfo_phase_(0),
          lfo_value_(0),
          lfo_note_(0),
          last_note_(0),
          current_octave_(0),
          current_tone_(0),
          current_operator_fnumber_{0, 0, 0, 0},
          operator_portamento_active_(false),
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
        last_note_ = 0;
        current_octave_ = 0;
        current_tone_ = 0;
        std::fill_n(current_operator_fnumber_, 4, 0);
        operator_portamento_active_ = false;
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
        last_note_ = part_data.LastTone < 0 ? base_tone : lfo_note_;
        InitializeSoundLFO(base_tone);
        part_data.Tone = CalculateTone(base_tone, part_data.Detune);
        SetTone(part_data.Octave, part_data.Tone);
    }

    void FmSequencer::ApplyVibratoEffect(int octave, int tone, int depth)
    {
        SetTone(octave, tone + CalculateFNumberOffset(lfo_note_, depth));
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
        int portamento_tone = initial_tone + delta * tick;

        if (channel_ != 2)
        {
            SetTone(block, portamento_tone);
            return;
        }

        const FMSound& sound = GetSound(sound_no_);
        int operator_fnumber[4];
        for (int op = 0; op < 4; op++)
        {
            int dt2 = sound.Op[op].Dt2;
            int initial_operator_tone =
                (F_NUMBER_TABLE[dt2][last_note_] + last_tone - F_NUMBER_TABLE[0][last_note_]) >> (block - last_octave);
            int target_operator_tone =
                (F_NUMBER_TABLE[dt2][lfo_note_] + tone - F_NUMBER_TABLE[0][lfo_note_]) >> (block - octave);
            int operator_delta = (target_operator_tone - initial_operator_tone) / (length + 1);
            operator_fnumber[op] = initial_operator_tone + operator_delta * tick;
            current_operator_fnumber_[op] = operator_fnumber[op];
        }
        current_octave_ = block;
        current_tone_ = portamento_tone;
        operator_portamento_active_ = true;
        fmwrap_.SetOperatorTones(channel_, block, operator_fnumber, GetSoundLFOOffset());
    }

    void FmSequencer::SetTone(int octave, int tone)
    {
        current_octave_ = octave;
        current_tone_ = tone;
        operator_portamento_active_ = false;
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
        if (channel_ == 2)
        {
            if (operator_portamento_active_)
            {
                fmwrap_.SetOperatorTones(channel_, octave, current_operator_fnumber_, GetSoundLFOOffset());
                return;
            }

            int operator_fnumber[4];
            for (int op = 0; op < 4; op++)
            {
                operator_fnumber[op] = GetOperatorFNumber(op, lfo_note_, tone);
            }
            fmwrap_.SetOperatorTones(channel_, octave, operator_fnumber, GetSoundLFOOffset());
        }
        else
        {
            fmwrap_.SetTone(channel_, octave, tone, GetSoundLFOOffset());
        }
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
        return F_NUMBER_TABLE[0][base_tone] + CalculateFNumberOffset(base_tone, detune);
    }

    int FmSequencer::CalculateFNumberOffset(int note, int depth) const
    {
        if (depth == 0)
        {
            return 0;
        }

        int magnitude = depth < 0 ? -depth : depth;
        magnitude &= 0xff;
        int coefficient = F_NUMBER_ADJUST_COEFFICIENT[note + (depth > 0 ? 1 : 0)];
        int offset = magnitude * coefficient / 0x100;
        return depth < 0 ? -offset : offset;
    }

    int FmSequencer::GetOperatorFNumber(int op, int note, int tone) const
    {
        const FMSound& sound = GetSound(sound_no_);
        int dt2 = sound.Op[op].Dt2;
        return F_NUMBER_TABLE[dt2][note] + tone - F_NUMBER_TABLE[0][note];
    }

} // namespace MusicCom
