#include "sequencer.h"
#include "fmsequencer.h"
#include "musdata.h"
#include "partsequencerbase.h"
#include "psgsequencer.h"
#include "sounddata.h"
#include "soundsequencer.h"
#include <algorithm>
#include <fmgen/opna.h>
#include <numeric>

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

    bool Sequencer::Init(int rate)
    {
        if (!opn.Init(OPN_CLOCKFREQ, rate))
        {
            return false;
        }

        InitializeSequencer(rate);

        // 効果音モード on
        opn.SetReg(0x27, 0x40);

        return true;
    }

    void Sequencer::InitializeSequencer(int rate)
    {
        std::vector<PsgSequencer*> observer_list;
        for (int ch = 0; ch < 6; ch++)
        {
            if (musicdata.IsChannelPresent(ch))
            {
                std::unique_ptr<PartSequencerBase> ptr;
                if (ch < 3)
                {
                    ptr = std::make_unique<FmSequencer>(opn, fmwrap, musicdata, ch, rate);
                }
                else
                {
                    auto psg = std::make_unique<PsgSequencer>(opn, ssgwrap, musicdata, ch, rate);
                    // チャンネル4,5(プログラム上は3,4)は効果音再生状態通知を受け取る
                    // D:パートの初期化時に設定するためここではポインタのみ保持
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
            auto ptr = std::make_unique<SoundSequencer>(opn, ssgwrap, musicdata, sounddata, rate);
            ptr->Initialize();
            // 効果音再生状態通知(効果音フレームの更新およびチャンネル4,5の抑止のため)
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
            // 各パートから次フレームまでの残時間が最小のものを抽出
            auto frame_size = std::reduce(
                partSequencer.begin(),
                partSequencer.end(),
                nsamples,
                [](auto acc, const auto& sequencer)
                {
                    return std::min(acc, sequencer->GetRemainFrameSize());
                });

            opn.Mix(dest, frame_size);

            dest += frame_size * 2;
            nsamples -= frame_size;

            for (const auto& part : partSequencer)
            {
                part->IncreaseFrame(frame_size);
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
                    part->IncreaseFrame(frame_size);
                }
            }
        }
    }
} // namespace MusicCom
