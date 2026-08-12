#include "psgsequencer.h"
#include "fmwrap.h"
#include <algorithm>

namespace MusicCom
{
    // clang-format off
    // O1におけるトーン周期
    const int SSG_TONE_PERIOD[12] = {
        // C,  C#,   D,  D#,   E,   F,  F#,   G,  G#,   A,  A#,   B
        3816,3602,3400,3209,3029,2859,2698,2547,2404,2269,2142,2022
    };

    // N/Iからトーン周期補正値を求める係数
    const int SSG_TONE_ADJUST_COEFFICIENT[13] = {
        // C-,  C, C#,  D, D#,  E,  F, F#,  G, G#,  A, A#,  B
          228,214,202,191,180,170,161,151,143,135,127,120,114
    };
    // clang-format on

    PsgSequencer::PsgSequencer(FM::OPN& opn, SSGWrap& ssgwrap, const MusicData& music, int channel, int rate)
        : PartSequencerBase(opn, music, music.GetChannelTail(channel), rate),
          channel_(channel - 3),
          ssgwrap_(ssgwrap),
          ring_deterrence_(false),
          current_note_(0),
          last_period_(SSG_TONE_PERIOD[0]),
          current_period_(SSG_TONE_PERIOD[0]),
          GetSSGEnv([this](int no) -> const SSGEnv&
                    { return GetMusicData().GetSSGEnv(no); }),
          GetHeadImpl([this, channel]()
                      { return GetMusicData().GetChannelHead(channel); })
    {
    }

    PsgSequencer::~PsgSequencer()
    {
    }

    void PsgSequencer::InitializeImpl(PartData& part_data)
    {
        // 初手ポルタメント対応
        part_data.LastOctave = 0;
        part_data.LastTone = CalculateTone(0, 0, 0);
        current_note_ = 0;
        last_period_ = SSG_TONE_PERIOD[0];
        current_period_ = SSG_TONE_PERIOD[0];
    }

    void PsgSequencer::UpdateDeterrence(SoundSequencer::PlayStatus status)
    {
        ring_deterrence_ = (status == SoundSequencer::PlayStatus::PLAYING);
    }

    CommandIterator PsgSequencer::ProcessCommandImpl(CommandIterator ptr, int current_frame, PartData& part_data)
    {
        auto return_ptr = CommandIterator(ptr);
        const Command& command = *return_ptr++;
        switch (command.GetType())
        {
        case CommandType::TYPE_TONE:
            part_data.SoundNo = command.GetArg(0);
            part_data.SSGEnvOn = part_data.SoundNo != 0;
            if (part_data.SSGEnvOn)
            {
                ssgwrap_.SetEnv(channel_, false);
            }
            break;
        case CommandType::TYPE_ENV_FORM:
            part_data.SSGEnvOn = false;
            ssgwrap_.SetEnv(channel_, true);
            ssgwrap_.SetEnvForm(channel_, command.GetArg(0));
            break;
        case CommandType::TYPE_ENV_PERIOD:
            part_data.SSGEnvOn = false;
            ssgwrap_.SetEnv(channel_, true);
            ssgwrap_.SetEnvPeriod(command.GetArg(0));
            break;
        default:
            return_ptr = PartSequencerBase::ProcessCommandImpl(ptr, current_frame, part_data);
            break;
        }
        return return_ptr;
    }

    void PsgSequencer::ProcessEffect(int current_frame)
    {
        if (!ring_deterrence_)
        {
            PartSequencerBase::ProcessEffect(current_frame);
        }
    }

    void PsgSequencer::KeyOn()
    {
        if (!ring_deterrence_)
        {
            ssgwrap_.PrepareKeyOn(channel_);
            ssgwrap_.KeyOnOff(channel_, true);
        }
    }

    void PsgSequencer::KeyOff()
    {
        if (!ring_deterrence_)
        {
            ssgwrap_.KeyOnOff(channel_, false);
        }
    }

    void PsgSequencer::UpdateTone(int base_tone, PartData& part_data)
    {
        last_period_ = current_period_;
        current_note_ = base_tone;
        current_period_ = CalculateTonePeriod(base_tone, part_data.Detune);
        part_data.Tone = ApplyOctave(current_period_, part_data.Octave);
        SetTone(part_data.Octave, part_data.Tone);
    }

    void PsgSequencer::ApplyVibratoEffect(int octave, int tone, int depth)
    {
        int period = current_period_ + CalculateTonePeriodOffset(current_note_, depth);
        SetTone(octave, ApplyOctave(period, octave));
    }

    int PsgSequencer::AdjustVolume(int volume, int length, const PartData& part_data)
    {
        int adjust_volume = volume;
        if (part_data.SSGEnvOn)
        {
            auto env = GetSSGEnv(part_data.SoundNo);
            if (env.Env.empty())
            {
                return adjust_volume;
            }
            size_t pos = length / env.Unit;
            if (pos >= env.Env.size())
            {
                pos = env.Env.size() - 1;
            }
            adjust_volume = std::max(((volume + env.Env[pos]) & 0xff) - 15, 0);
        }
        return adjust_volume;
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

    const CommandIterator PsgSequencer::GetHead() const
    {
        return GetHeadImpl();
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
