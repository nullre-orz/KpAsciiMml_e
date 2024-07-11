#pragma once

#include "fmwrap.h"
#include "partdata.h"
#include "partsequencerbase.h"
#include <memory>
#include <vector>

namespace FM
{
    class OPN;
}

namespace MusicCom
{
    class MusicData;
    class SoundData;

    // 適当すぎ
    class Sequencer
    {
    public:
        Sequencer(FM::OPN& o, MusicData* pmd, SoundData* psd);
        bool Init(int rate);
        void Mix(__int16* dest, int nsamples);
        int GetSamplesPerFrame()
        {
            return samplesPerFrame;
        }

    private:
        void NextFrame();

        FM::OPN& opn;
        FMWrap fmwrap;
        SSGWrap ssgwrap;
        MusicData& musicdata;
        SoundData& sounddata;

        std::vector<std::unique_ptr<PartSequencerBase>> partSequencer;

        int rate;
        int samplesPerFrame;
        int samplesLeft; // このフレーム(64分音符)でmixすべき残りのサンプル数
        int currentFrame;
    };

} // namespace MusicCom
