#include "musiccom.h"
#include "mmlparser.h"
#include "musdata.h"
#include "sequencer.h"
#include "sounddata.h"
#include "soundparser.h"
#include "soundsequencer.h"
#include <algorithm>

namespace MusicCom
{
    const int MusicCom::SOUND_EFFECT_DEFAULT_TEMPO = 195;

    MusicCom::MusicCom()
        : pseq(nullptr),
          pmusicdata(nullptr),
          psounddata(nullptr),
          fmVolume(0),
          psgVolume(0),
          soundTempo(SOUND_EFFECT_DEFAULT_TEMPO)
    {
    }

    MusicCom::~MusicCom() = default;

    bool MusicCom::Load(const char* filename)
    {
        auto new_musicdata = ParseMML(filename);
        if (!new_musicdata)
            return false;

        // SOUND.datを解析
        auto new_sounddata = ParseSound(filename);
        if (!new_sounddata)
            return false;

        pseq.reset();
        pmusicdata = std::move(new_musicdata);
        psounddata = std::move(new_sounddata);

        return true;
    }

    bool MusicCom::PrepareMix(uint rate)
    {
        pseq = std::make_unique<Sequencer>(opn, *pmusicdata, *psounddata, soundTempo);
        if (!pseq->Init(rate))
        {
            return false;
        }
        opn.SetVolumeFM(fmVolume);
        opn.SetVolumePSG(psgVolume);

        return true;
    }

    void MusicCom::Mix(FM_SAMPLETYPE* dest, int nsamples)
    {
        pseq->Mix(dest, nsamples);
    }

    void MusicCom::SetFMVolume(int vol)
    {
        fmVolume = std::clamp(vol, -192, 20);
    }

    void MusicCom::SetPSGVolume(int vol)
    {
        psgVolume = std::clamp(vol, -192, 20);
    }

    void MusicCom::SetSoundTempo(int tempo)
    {
        soundTempo = std::clamp(tempo, 128, 255);
    }

} // namespace MusicCom
