#include "fmsequencer.h"
#include "fmwrap.h"
#include <algorithm>
#include <cmath>

namespace MusicCom
{
    // clang-format off
    const int F_NUMBER_BASE[14] = {
	    //C-,  C,  C#,   D,  D#,   E,   F,  F#,   G,  G#,   A,  A#,   B,   B#
	    584, 618, 655, 694, 735, 779, 825, 874, 926, 981,1040,1101,1167, 1236
    };
    // clang-format on
    const int* const F_NUMBER = &F_NUMBER_BASE[1];

    FmSequencer::FmSequencer(FM::OPN& opn, FMWrap& fmwrap, const MusicData& music, int channel, int rate)
        : PartSequencerBase(opn, music, music.GetChannelTail(channel), rate),
          channel_(channel),
          fmwrap_(fmwrap),
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
    }

    CommandIterator FmSequencer::ProcessCommandImpl(CommandIterator ptr, int current_frame, PartData& part_data)
    {
        auto return_ptr = CommandIterator(ptr);
        const Command& command = *return_ptr++;
        switch (command.GetType())
        {
        case CommandType::TYPE_TONE:
            part_data.SoundNo = command.GetArg(0);
            fmwrap_.SetSound(channel_, GetSound(part_data.SoundNo));
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
        part_data.Tone = CalculateTone(base_tone, part_data.Detune);
        SetTone(part_data.Octave, part_data.Tone);
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
        fmwrap_.SetTone(channel_, octave, tone);
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
