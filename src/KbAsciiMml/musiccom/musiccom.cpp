#include "musiccom.h"
#include "mmlparser.h"
#include "musdata.h"
#include "sequencer.h"
#include "sounddata.h"
#include "soundparser.h"

namespace MusicCom
{
    MusicCom::MusicCom()
        : pseq(nullptr),
          pmusicdata(nullptr),
          psounddata(nullptr),
          adjustment(false)
    {
    }

    MusicCom::~MusicCom()
    {
    }

    bool MusicCom::Load(const char* filename)
    {
        pmusicdata.reset(ParseMML(filename));
        if (!pmusicdata)
            return false;

        // SOUND.datを解析
        psounddata.reset(ParseSound(filename));
        if (!psounddata)
            return false;

        return true;
    }

    bool MusicCom::PrepareMix(uint rate)
    {
        pseq = std::make_unique<Sequencer>(opn, pmusicdata.get(), psounddata.get());
        if (!pseq->Init(rate))
        {
            return false;
        }
        opn.SetVolumeFM(fmVolume);
        opn.SetVolumePSG(psgVolume);
        opn.SetNoiseAdjustment(adjustment);

        return true;
    }

    void MusicCom::Mix(FM_SAMPLETYPE* dest, int nsamples)
    {
        pseq->Mix(dest, nsamples);
    }

    void MusicCom::SetFMVolume(int vol)
    {
        fmVolume = vol;
    }

    void MusicCom::SetPSGVolume(int vol)
    {
        psgVolume = vol;
    }

    void MusicCom::SetNoiseAdjustment(bool on)
    {
        adjustment = on;
    }

} // namespace MusicCom
