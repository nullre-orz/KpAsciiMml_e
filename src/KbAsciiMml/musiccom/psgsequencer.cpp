#include "psgsequencer.h"
#include "fmwrap.h"

namespace MusicCom
{
    // clang-format off
    const int SSG_TONE_NUM[10][12] = {
        // 124800 / Hz
        //0  C,1 C#,2  D,3 D#,4  E,5  F,6 F#,7  G,8 G#,9  A,10A#,11 B
	    { 3816,3602,3400,3209,3029,2859,2698,2547,2404,2269,2142,4043},	// O0
	    { 3816,3602,3400,3209,3029,2859,2698,2547,2404,2269,2142,2022}, // O1
	    { 1908,1801,1700,1604,1514,1429,1349,1273,1202,1135,1071,1011},
	    {  954, 900, 850, 802, 757, 715, 675, 637, 601, 567, 535, 505},
	    {  477, 450, 425, 401, 379, 357, 337, 318, 301, 284, 268, 253},	// O4
	    {  239, 225, 212, 201, 189, 179, 169, 159, 150, 142, 134, 126},
	    {  119, 113, 106, 100,  95,  89,  84,  80,  75,  71,  67,  63},
	    {   60,  56,  53,  50,  47,  45,  42,  40,  38,  35,  33,  32},
	    {   30,  28,  27,  25,  24,  22,  21,  20,  19,  18,  17,  16},	// O8
	    {   15,  14,  13,  13,  12,  11,  11,  10,   9,   9,   8,   8},
    };
    // clang-format on

    PsgSequencer::PsgSequencer(FM::OPN& opn, SSGWrap& ssgwrap, const MusicData& music, int channel)
        : PartSequencerBase(opn, music, music.GetChannelTail(channel)),
          channel_(channel - 3),
          ssgwrap_(ssgwrap),
          ring_deterrence_(false),
          GetSSGEnv([this](int no) -> const SSGEnv&
                    { return GetMusicData().GetSSGEnv(no); }),
          GetHeadImpl([this, channel]()
                      { return GetMusicData().GetChannelHead(channel); })
    {
    }

    PsgSequencer::~PsgSequencer()
    {
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
            part_data.SSGEnvOn = true;
            break;
        case CommandType::TYPE_ENV_FORM:
            part_data.SSGEnvOn = false;
            ssgwrap_.SetEnv(channel_, true);
            ssgwrap_.SetEnvForm(command.GetArg(0));
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

    int PsgSequencer::InitializeTone()
    {
        return CalculateTone(1, 1, 0);
    }

    void PsgSequencer::KeyOn()
    {
        if (!ring_deterrence_)
        {
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
        part_data.Tone = CalculateTone(part_data.Octave, base_tone, part_data.Detune);
        SetTone(part_data.Octave, part_data.Tone);
    }

    int PsgSequencer::AdjustVolume(int volume, int length, const PartData& part_data)
    {
        int adjust_volume = volume;
        if (part_data.SSGEnvOn)
        {
            auto env = GetSSGEnv(part_data.SoundNo);
            size_t pos = length / env.Unit;
            if (pos >= env.Env.size())
            {
                pos = env.Env.size() - 1;
            }
            adjust_volume = volume + (env.Env[pos] - 15);
        }
        return adjust_volume;
    }

    void PsgSequencer::ApplyPortamentoEffect(int octave, int tone, int last_octave, int last_tone, double coefficient)
    {
        // octaveは使用しない(SetToneの第1引数はダミー)
        double new_tone = last_tone + (tone - last_tone) * coefficient;
        SetTone(octave, static_cast<int>(new_tone + 0.5));
    }

    void PsgSequencer::SetTone(int octave, int tone)
    {
        // octaveは使用しない
        ssgwrap_.SetTone(channel_, tone);
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
        int tone = SSG_TONE_NUM[base_octave][base_tone];
        if (detune != 0)
        {
            int tone2 = static_cast<int>(tone * pow(0.5, detune / (255.0 * 12.0)) + 0.5);
            if (tone == tone2)
            {
                tone = (detune < 0) ? tone + 1 : tone - 1;
            }
            else
            {
                tone = tone2;
            }
        }
        return tone;
    }

} // namespace MusicCom
