#pragma once

#include "fmwrap.h"
#include "musdata.h"
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
    // 適当すぎ
    class Sequencer
    {
    public:
        Sequencer(FM::OPN& o, MusicData* pmd);
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

        std::vector<std::unique_ptr<PartSequencerBase>> partSequencer;

        int rate;
        int samplesPerFrame;
        int samplesLeft; // このフレーム(64分音符)でmixすべき残りのサンプル数
        int currentFrame;
    };

} // namespace MusicCom
