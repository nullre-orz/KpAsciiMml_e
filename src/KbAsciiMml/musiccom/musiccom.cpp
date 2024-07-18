#include "musiccom.h"
#include "mmlparser.h"
#include "musdata.h"
#include "sequencer.h"
#include "sounddata.h"
#include "soundparser.h"
#include "soundsequencer.h"

namespace MusicCom
{
    const int MusicCom::SOUND_EFFECT_DEFAULT_TEMPO = 195;

    MusicCom::MusicCom()
        : pseq(nullptr),
          pmusicdata(nullptr),
          psounddata(nullptr),
          adjustment(false),
          soundTempo(SOUND_EFFECT_DEFAULT_TEMPO)
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
        pseq = std::make_unique<Sequencer>(opn, pmusicdata.get(), psounddata.get(), soundTempo);
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
        fmVolume = std::min(std::max(vol, -192), 20);
    }

    void MusicCom::SetPSGVolume(int vol)
    {
        psgVolume = std::min(std::max(vol, -192), 20);
    }

    void MusicCom::SetNoiseAdjustment(bool on)
    {
        adjustment = on;
    }

    void MusicCom::SetSoundTempo(int tempo)
    {
        soundTempo = std::min(std::max(tempo, 128), 255);
    }

} // namespace MusicCom
