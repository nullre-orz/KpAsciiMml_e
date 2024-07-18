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
    class SoundSequencer;

    // 適当すぎ
    class Sequencer
    {
    public:
        Sequencer(FM::OPN& o, MusicData* pmd, SoundData* psd, int stempo);
        bool Init(int rate);
        void Mix(__int16* dest, int nsamples);

    private:
        void InitializeSequencer(int rate);

        FM::OPN& opn;
        FMWrap fmwrap;
        SSGWrap ssgwrap;
        MusicData& musicdata;
        SoundData& sounddata;
        int soundtempo;

        std::vector<std::unique_ptr<PartSequencerBase>> partSequencer;
    };

} // namespace MusicCom
