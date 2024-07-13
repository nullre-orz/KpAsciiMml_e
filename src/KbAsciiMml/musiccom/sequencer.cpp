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
    const unsigned int OPN_CLOCKFREQ = 3993600; // OPNのクロック周波数

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
        rate = r;
        samplesPerFrame = (int)(rate * (60.0 / (musicdata.GetTempo() * 16.0)) / 1.1 + 0.5); // 1.1: music.comの演奏は速いので補正
        samplesLeft = 0;
        currentFrame = 0;

        if (!opn.Init(OPN_CLOCKFREQ, rate))
            return false;

        InitializeSequencer();

        // 効果音モード on
        opn.SetReg(0x27, 0x40);

        return true;
    }

    void Sequencer::InitializeSequencer()
    {
        std::vector<PsgSequencer*> observer_list;
        for (int ch = 0; ch < 6; ch++)
        {
            if (musicdata.IsChannelPresent(ch))
            {
                std::unique_ptr<PartSequencerBase> ptr;
                if (ch < 3)
                {
                    ptr = std::make_unique<FmSequencer>(opn, fmwrap, musicdata, ch);
                }
                else
                {
                    auto psg = std::make_unique<PsgSequencer>(opn, ssgwrap, musicdata, ch);
                    // チャンネル4,5(プログラム上は3,4)は効果音再生状態通知を受け取る
                    if (ch == 3 || ch == 4)
                    {
                        observer_list.push_back(psg.get());
                    }
                    ptr = std::move(psg);
                }
                ptr->Initialize();
                partSequencer.emplace_back(std::move(ptr));
            }
        }
        if (musicdata.IsRhythmPartPresent())
        {
            auto ptr = std::make_unique<SoundSequencer>(opn, ssgwrap, musicdata, sounddata);
            ptr->Initialize();
            // 効果音再生状態通知(チャンネル4,5を抑止するため)
            for (auto item : observer_list)
            {
                ptr->AppendPlayStatusObserver(
                    [item](SoundSequencer::PlayStatus status)
                    {
                        item->UpdateDeterrence(status);
                    });
            }
            partSequencer.emplace_back(std::move(ptr));
        }
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
