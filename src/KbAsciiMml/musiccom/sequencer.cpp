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
    namespace
    {
        enum YM2203Register : int
        {
            YM2203_MODE_REGISTER = 0x27,
        };

        enum YM2203Control : int
        {
            YM2203_CH3_SPECIAL_MODE = 0x40,
        };

        constexpr unsigned int OPN_CLOCKFREQ = 3993600; // OPNのクロック周波数
        constexpr int SSG_REGISTER_LIMIT = 0x10;
    }

    Sequencer::Sequencer(FM::OPN& o, MusicData& md, SoundData& sd, int stempo)
        : opn(o),
          fmwrap(o),
          ssgwrap(o),
          musicdata(md),
          sounddata(sd),
          soundtempo(stempo)
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
        opn.SetReg(YM2203_MODE_REGISTER, YM2203_CH3_SPECIAL_MODE);

        return true;
    }

    void Sequencer::WriteRegister(int address, int value)
    {
        if (address < SSG_REGISTER_LIMIT)
        {
            ssgwrap.WriteMusicRegister(address, value);
        }
        else
        {
            opn.SetReg(address, value);
        }
    }

    void Sequencer::InitializeSequencer(int rate)
    {
        // YコマンドによるYM2203レジスタ書込みを各パートから受け取る
        RegisterWriter register_writer = [this](int address, int value)
        {
            WriteRegister(address, value);
        };

        for (int ch = 0; ch < 6; ch++)
        {
            if (musicdata.IsChannelPresent(ch))
            {
                std::unique_ptr<PartSequencerBase> ptr;
                if (ch < 3)
                {
                    ptr = std::make_unique<FmSequencer>(register_writer, fmwrap, musicdata, ch, rate);
                }
                else
                {
                    ptr = std::make_unique<PsgSequencer>(register_writer, ssgwrap, musicdata, ch, rate);
                }
                ptr->Initialize();
                partSequencer.emplace_back(std::move(ptr));
            }
        }
        if (musicdata.IsRhythmPartPresent())
        {
            auto ptr = std::make_unique<SoundSequencer>(register_writer, ssgwrap, musicdata, sounddata, soundtempo, rate);
            ptr->Initialize();
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
