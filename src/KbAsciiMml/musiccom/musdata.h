#pragma once

#include "command.h"
#include <array>
#include <algorithm>
#include <cassert>
#include <list>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace MusicCom
{
    enum MusicChannel : int
    {
        MUSIC_CHANNEL_FM_1 = 0,
        MUSIC_CHANNEL_FM_2,
        MUSIC_CHANNEL_FM_3,
        MUSIC_CHANNEL_FM_COUNT,
        MUSIC_CHANNEL_SSG_A = MUSIC_CHANNEL_FM_COUNT,
        MUSIC_CHANNEL_SSG_B,
        MUSIC_CHANNEL_SSG_C,
        MUSIC_CHANNEL_COUNT,
    };

    struct FMSound
    {
        int LFOForm;
        int LFOSpeed;
        int LFODepth;

        unsigned char AlgFb;
        struct Operator
        {
            unsigned char DtMl;
            unsigned char Tl;
            unsigned char KsAr;
            unsigned char Dr;
            unsigned char Sr;
            unsigned char SlRr;
            unsigned char Dt2;
        };
        enum : int
        {
            OPERATOR_COUNT = 4,
        };
        std::array<Operator, OPERATOR_COUNT> Op;

        FMSound()
            : LFOForm(0),
              LFOSpeed(0),
              LFODepth(0),
              AlgFb(0),
              Op{}
        {
        }
        void Clear()
        {
            LFOForm = 0;
            LFOSpeed = 0;
            LFODepth = 0;
            AlgFb = 0;
            Op.fill({});
        }
        void SetLFO(int form, int speed, int depth)
        {
            // 各LFO値は下位8bitだけを保存
            LFOForm = form & 0xff;
            LFOSpeed = speed & 0xff;
            LFODepth = depth & 0xff;
        }
        void SetAlgFb(int alg, int fb)
        {
            AlgFb = ((fb & 0x7) << 3) | (alg & 0x7);
        }
        void SetDtMl(int op, int dt, int ml)
        {
            assert(0 <= op && op < OPERATOR_COUNT);
            Op[op].DtMl = ((dt & 0x7) << 4) | (ml & 0xf);
        }
        void SetTl(int op, int tl)
        {
            assert(0 <= op && op < OPERATOR_COUNT);
            Op[op].Tl = tl & 0x7f;
        }
        void SetKsAr(int op, int ks, int ar)
        {
            assert(0 <= op && op < OPERATOR_COUNT);
            Op[op].KsAr = ((ks & 0x3) << 6) | (ar & 0x1f);
        }
        void SetDr(int op, int dr)
        {
            assert(0 <= op && op < OPERATOR_COUNT);
            Op[op].Dr = dr & 0x1f;
        }
        void SetSr(int op, int sr)
        {
            assert(0 <= op && op < OPERATOR_COUNT);
            Op[op].Sr = sr & 0x1f;
        }
        void SetSlRr(int op, int sl, int rr)
        {
            assert(0 <= op && op < OPERATOR_COUNT);
            Op[op].SlRr = ((sl & 0xf) << 4) | (rr & 0xf);
        }
        void SetDt2(int op, int dt2)
        {
            assert(0 <= op && op < OPERATOR_COUNT);
            Op[op].Dt2 = std::clamp(dt2 & 0xff, 0, 3);
        }

        int GetAlg() const
        {
            return AlgFb & 0x7;
        }
    };

    struct SSGEnv
    {
        SSGEnv()
        {
            Unit = 64;
            Env.push_back(15);
        }
        SSGEnv(unsigned char initial_value) : Unit(64), Env(1, initial_value)
        {
        }
        int Unit;
        std::vector<unsigned char> Env;
    };

    class MusicData
    {
    public:
        MusicData();
        void SetFMSound(int no, const FMSound& sound)
        {
            fmsounds[no] = sound;
        }
        void SetSSGEnv(int no, const SSGEnv& env)
        {
            ssgenvs[no] = env;
        }
        const FMSound& GetFMSound(int no) const;
        const SSGEnv& GetSSGEnv(int no) const;
        void SetTempo(int t) { tempo = t; }
        int GetTempo() const { return tempo; }
        bool IsChannelPresent(int channel) const
        {
            assert(0 <= channel && channel < MUSIC_CHANNEL_COUNT);
            return channel_present[channel];
        }
        bool IsRhythmPartPresent() const
        {
            return rhythm_part_present;
        }
        bool IsMacroPresent(const std::string& name) const
        {
            return macros.find(name) != macros.end();
        }

        void DefineMacro(const std::string& name);
        void AddCommandToChannel(int channel, const Command& command);
        void AddCommandToRhythmPart(const Command& command);
        void AddCommandToMacro(const std::string& name, const Command& command);

        CommandIterator GetChannelHead(int channel) const;
        CommandIterator GetChannelTail(int channel) const;
        CommandIterator GetRhythmPartHead() const;
        CommandIterator GetRhythmPartTail() const;
        CommandIterator GetMacroHead(const std::string& name) const;

    private:
        static const int channel_count = 6;
        std::map<int, FMSound> fmsounds;
        std::map<int, SSGEnv> ssgenvs;
        std::map<std::string, CommandList> macros;
        std::array<CommandList, MUSIC_CHANNEL_COUNT> channels;
        std::array<bool, MUSIC_CHANNEL_COUNT> channel_present;
        CommandList rhythm_part;
        bool rhythm_part_present;
        int tempo;
    };

} // namespace MusicCom
