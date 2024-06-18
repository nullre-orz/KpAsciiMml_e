#include "sounddata.h"

namespace MusicCom
{
    SoundData::SoundData()
    {
    }

    void SoundData::SetRhythm(int no, const RhythmData& rhythm)
    {
        rhythms[no] = rhythm;
    }

    const RhythmData& SoundData::GetRhythm(int no)
    {
        return rhythms[no];
    }
} // namespace MusicCom
