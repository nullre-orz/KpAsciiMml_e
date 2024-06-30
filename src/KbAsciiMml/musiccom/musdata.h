#pragma once

#include "command.h"
#include <cassert>
#include <list>
#include <map>
#include <string>
#include <vector>

namespace MusicCom
{
    struct FMSound
    {
        int LFOForm;
        int LFOSpeed;
        int LFODepth;

        unsigned char AlgFb;
        struct
        {
            unsigned char DtMl;
            unsigned char Tl;
            unsigned char KsAr;
            unsigned char Dr;
            unsigned char Sr;
            unsigned char SlRr;
            unsigned char Dt2;
        } Op[4];

        struct FMSound()
        {
            Clear();
        }
        void Clear()
        {
            std::memset(this, 0, sizeof(*this));
        }
        void SetLFO(int form, int speed, int depth)
        {
            LFOForm = form;
            LFOSpeed = speed;
            LFODepth = depth;
        }
        void SetAlgFb(int alg, int fb)
        {
            AlgFb = ((fb & 0x7) << 3) | (alg & 0x7);
        }
        void SetDtMl(int op, int dt, int ml)
        {
            assert(0 <= op && op < 4);
            Op[op].DtMl = ((dt & 0x7) << 4) | (ml & 0xf);
        }
        void SetTl(int op, int tl)
        {
            assert(0 <= op && op < 4);
            Op[op].Tl = tl & 0x7f;
        }
        void SetKsAr(int op, int ks, int ar)
        {
            assert(0 <= op && op < 4);
            Op[op].KsAr = ((ks & 0x3) << 6) | (ar & 0x1f);
        }
        void SetDr(int op, int dr)
        {
            assert(0 <= op && op < 4);
            Op[op].Dr = dr & 0x1f;
        }
        void SetSr(int op, int sr)
        {
            assert(0 <= op && op < 4);
            Op[op].Sr = sr & 0x1f;
        }
        void SetSlRr(int op, int sl, int rr)
        {
            assert(0 <= op && op < 4);
            Op[op].SlRr = ((sl & 0xf) << 4) | (rr & 0xf);
        }
        void SetDt2(int op, int dt2)
        {
            assert(0 <= op && op < 4);
            Op[op].Dt2 = dt2 & 0x3;
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
        const FMSound& GetFMSound(int no) const
        {
            return fmsounds[no];
        }
        const SSGEnv& GetSSGEnv(int no) const
        {
            // 存在しないnoが指定された場合は無音とする
            if (ssgenvs.find(no) == ssgenvs.end())
            {
                SSGEnv ssgenv(0);
                ssgenvs[no] = ssgenv;
            }
            return ssgenvs[no];
        }
        void SetTempo(int t) { tempo = t; }
        int GetTempo() const { return tempo; }
        bool IsChannelPresent(int index) const
        {
            assert(0 <= index && index < channel_count);
            return channel_present[index];
        }
        bool IsRhythmPartPresent() const
        {
            return rhythm_part_present;
        }
        bool IsMacroPresent(const std::string& name) const
        {
            return macros.find(name) != macros.end();
        }

        void AddCommandToChannel(int index, const Command& command);
        void AddCommandToRhythmPart(const Command& command);
        void AddCommandToMacro(std::string name, const Command& command);

        CommandIterator GetChannelHead(int channel) const;
        CommandIterator GetRhythmPartHead() const;
        CommandIterator GetMacroHead(const std::string& name) const;

    private:
        static const int channel_count = 6;

        mutable std::map<int, FMSound> fmsounds;
        mutable std::map<int, SSGEnv> ssgenvs;
        std::map<std::string, CommandList> macros;
        CommandList channels[channel_count];
        bool channel_present[channel_count];
        CommandList rhythm_part;
        bool rhythm_part_present;
        int tempo;
    };

} // namespace MusicCom
