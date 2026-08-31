#include "psgsequencer.h"
#include "musdata.h"
#include "ssgwrap.h"
#include <algorithm>
#include <array>

namespace MusicCom
{
    // clang-format off
    // O1におけるトーン周期
    const std::array<int, 12> SSG_TONE_PERIOD = {
        // C,  C#,   D,  D#,   E,   F,  F#,   G,  G#,   A,  A#,   B
        3816,3602,3400,3209,3029,2859,2698,2547,2404,2269,2142,2022
    };

    // N/Iからトーン周期補正値を求める係数
    const std::array<int, 13> SSG_TONE_ADJUST_COEFFICIENT = {
        // C-,  C, C#,  D, D#,  E,  F, F#,  G, G#,  A, A#,  B
          228,214,202,191,180,170,161,151,143,135,127,120,114
    };
    // clang-format on

    PsgSequencer::PsgSequencer(const RegisterWriter& register_writer, SSGWrap& ssgwrap, const MusicData& music, int channel, int rate)
        : ToneSequencerBase(register_writer, music, music.GetChannelTail(channel), rate),
          channel_(channel - MUSIC_CHANNEL_SSG_A),
          ssgwrap_(ssgwrap),
          current_note_(0),
          last_period_(SSG_TONE_PERIOD[0]),
          current_period_(SSG_TONE_PERIOD[0])
    {
    }

    PsgSequencer::~PsgSequencer() = default;

    void PsgSequencer::InitializeImpl(PartData& part_data)
    {
        // 初手ポルタメント対応
        part_data.Note.LastOctave = 0;
        part_data.Note.LastTone = CalculateTone(0, 0, 0);
        current_note_ = 0;
        last_period_ = SSG_TONE_PERIOD[0];
        current_period_ = SSG_TONE_PERIOD[0];
    }

    CommandIterator PsgSequencer::ProcessCommandImpl(CommandIterator ptr, int current_frame, PartData& part_data)
    {
        auto return_ptr = CommandIterator(ptr);
        const Command& command = *return_ptr++;
        switch (command.GetType())
        {
        case CommandType::TYPE_TONE:
            part_data.Note.SoundNo = command.GetArg(0);
            part_data.Note.SSGEnvOn = part_data.Note.SoundNo != 0;
            if (part_data.Note.SSGEnvOn)
            {
                ssgwrap_.SetEnv(channel_, false);
            }
            break;
        case CommandType::TYPE_ENV_FORM:
            part_data.Note.SSGEnvOn = false;
            ssgwrap_.SetEnv(channel_, true);
            ssgwrap_.SetEnvForm(channel_, command.GetArg(0));
            break;
        case CommandType::TYPE_ENV_PERIOD:
            part_data.Note.SSGEnvOn = false;
            ssgwrap_.SetEnv(channel_, true);
            ssgwrap_.SetEnvPeriod(command.GetArg(0));
            break;
        default:
            return_ptr = ToneSequencerBase::ProcessCommandImpl(ptr, current_frame, part_data);
            break;
        }
        return return_ptr;
    }

    void PsgSequencer::KeyOn()
    {
        ssgwrap_.PrepareKeyOn(channel_);
        ssgwrap_.KeyOnOff(channel_, true);
    }

    void PsgSequencer::KeyOff()
    {
        ssgwrap_.KeyOnOff(channel_, false);
    }

    void PsgSequencer::UpdateTone(int base_tone, PartData& part_data)
    {
        last_period_ = current_period_;
        current_note_ = base_tone;
        current_period_ = CalculateTonePeriod(base_tone, part_data.Note.Detune);
        part_data.Note.Tone = ApplyOctave(current_period_, part_data.Note.Octave);
        SetTone(part_data.Note.Octave, part_data.Note.Tone);
    }

    void PsgSequencer::ApplyVibratoEffect(int octave, int tone, int depth)
    {
        int period = current_period_ + CalculateTonePeriodOffset(current_note_, depth);
        SetTone(octave, ApplyOctave(period, octave));
    }

    bool PsgSequencer::NeedsVolumeEffect(const PartData& part_data) const
    {
        return part_data.Effect.TremoloDepth != 0 || part_data.Note.SSGEnvOn;
    }

    int PsgSequencer::AdjustVolumeEffect(int volume, int keyon_length, const PartData& part_data)
    {
        int adjusted_volume = volume;
        if (part_data.Note.SSGEnvOn)
        {
            const auto& env = GetMusicData().GetSSGEnv(part_data.Note.SoundNo);
            if (env.Env.empty())
            {
                return adjusted_volume;
            }

            size_t pos = keyon_length / env.Unit;
            if (pos >= env.Env.size())
            {
                pos = env.Env.size() - 1;
            }
            adjusted_volume = std::max(((volume + env.Env[pos]) & 0xff) - 15, 0);
        }
        return adjusted_volume;
    }

    void PsgSequencer::ApplyPortamentoEffect(int octave, int tone, int last_octave, int last_tone, int tick, int length)
    {
        // octaveは使用しない(SetToneの第1引数はダミー)
        if (tick == length + 1)
        {
            SetTone(octave, tone);
            return;
        }

        int base_octave = std::min(octave, last_octave);
        int initial_period = last_period_ >> (last_octave - base_octave);
        int target_period = current_period_ >> (octave - base_octave);
        int delta = (target_period - initial_period) / (length + 1);
        int portamento_period = initial_period + delta * tick;
        SetTone(base_octave, ApplyOctave(portamento_period, base_octave));
    }

    void PsgSequencer::SetTone(int octave, int tone)
    {
        // octaveは使用しない
        ssgwrap_.SetTonePeriod(channel_, tone);
    }

    void PsgSequencer::SetVolume(int volume)
    {
        ssgwrap_.SetVolume(channel_, volume);
    }

    CommandIterator PsgSequencer::GetHead() const
    {
        return GetMusicData().GetChannelHead(channel_ + MUSIC_CHANNEL_SSG_A);
    }

    int PsgSequencer::CalculateTone(int base_octave, int base_tone, int detune) const
    {
        return ApplyOctave(CalculateTonePeriod(base_tone, detune), base_octave);
    }

    int PsgSequencer::CalculateTonePeriod(int note, int depth) const
    {
        return SSG_TONE_PERIOD[note] + CalculateTonePeriodOffset(note, depth);
    }

    int PsgSequencer::CalculateTonePeriodOffset(int note, int depth) const
    {
        if (depth == 0)
        {
            return 0;
        }

        int magnitude = depth < 0 ? -depth : depth;
        magnitude &= 0xff;
        int coefficient = SSG_TONE_ADJUST_COEFFICIENT[note + (depth > 0 ? 1 : 0)];
        int offset = magnitude * coefficient / 0x100;
        return depth > 0 ? -offset : offset;
    }

    int PsgSequencer::ApplyOctave(int period, int octave) const
    {
        return (period * 2) >> octave;
    }

} // namespace MusicCom
