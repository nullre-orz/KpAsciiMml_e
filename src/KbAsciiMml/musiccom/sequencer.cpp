#include "sequencer.h"
#include "fmsequencer.h"
#include "musdata.h"
#include "partsequencerbase.h"
#include "psgsequencer.h"
#include "sounddata.h"
#include "soundsequencer.h"
#include <algorithm>
#include <fmgen/opna.h>

// KeyOff したあと、音量レベルが低下するまでMixしてからOnしないとタイになってしまう
// fmgen の問題？

using namespace std;

namespace MusicCom
{
    Sequencer::Sequencer(FM::OPN& o, MusicData* pmd, SoundData* psd)
        : opn(o),
          fmwrap(o),
          ssgwrap(o),
          musicdata(*pmd),
          sounddata(*psd)
    {
    }

    bool Sequencer::Init(int r)
    {
        const unsigned int OPN_CLOCKFREQ = 3993600; // OPNのクロック周波数

        rate = r;
        samplesPerFrame = (int)(rate * (60.0 / (musicdata.GetTempo() * 16.0)) / 1.1 + 0.5); // 1.1: music.comの演奏は速いので補正
        samplesLeft = 0;
        currentFrame = 0;

        if (!opn.Init(OPN_CLOCKFREQ, rate))
            return false;

        for (int ch = 0; ch < 6; ch++)
        {
            if (musicdata.IsChannelPresent(ch))
            {
                if (ch < 3)
                {
                    partSequencer.emplace_back(std::make_unique<FmSequencer>(opn, fmwrap, musicdata, ch));
                }
                else
                {
                    partSequencer.emplace_back(std::make_unique<PsgSequencer>(opn, ssgwrap, musicdata, ch));
                }
                partSequencer.back()->Initialize();
            }
        }
        if (musicdata.IsRhythmPartPresent())
        {
            partSequencer.emplace_back(std::make_unique<SoundSequencer>(opn, ssgwrap, musicdata, sounddata));
            partSequencer.back()->Initialize();
        }

        // 効果音モード on
        opn.SetReg(0x27, 0x40);

        return true;
    }

    void Sequencer::Mix(__int16* dest, int nsamples)
    {
        memset(dest, 0, nsamples * sizeof(__int16) * 2);
        while (nsamples > 0)
        {
            if (samplesLeft < nsamples)
            {
                opn.Mix(dest, samplesLeft);
                dest += samplesLeft * 2;
                nsamples -= samplesLeft;
                NextFrame();
                samplesLeft = samplesPerFrame;
            }
            else
            {
                opn.Mix(dest, nsamples);
                samplesLeft -= nsamples;
                nsamples = 0;
            }
        }
    }

    void Sequencer::NextFrame()
    {
        for (const auto& part : partSequencer)
        {
            part->NextFrame(currentFrame);
        }

        // 全パートが一時停止していた場合、再開させる
        if (std::none_of(
                partSequencer.begin(),
                partSequencer.end(),
                [](const auto& part)
                {
                    return part->IsPlaying();
                }))
        {
            for (const auto& part : partSequencer)
            {
                part->Resume();
                part->NextFrame(currentFrame);
            }
        }

        currentFrame++;
    }
} // namespace MusicCom
