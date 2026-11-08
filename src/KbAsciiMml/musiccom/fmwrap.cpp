#include "fmwrap.h"
#include "musdata.h"
#include <fmgen/opna.h>

using namespace std;

namespace MusicCom
{
    namespace
    {
        enum FMRegister : int
        {
            FM_KEY_ON_OFF_REGISTER = 0x28,
            FM_DETUNE_MULTIPLE_REGISTER_BASE = 0x30,
            FM_TOTAL_LEVEL_REGISTER_BASE = 0x40,
            FM_KEY_SCALE_ATTACK_RATE_REGISTER_BASE = 0x50,
            FM_DECAY_RATE_REGISTER_BASE = 0x60,
            FM_SUSTAIN_RATE_REGISTER_BASE = 0x70,
            FM_SUSTAIN_LEVEL_RELEASE_RATE_REGISTER_BASE = 0x80,
            FM_TONE_LOW_REGISTER_BASE = 0xa0,
            FM_CH3_OPERATOR4_TONE_LOW_REGISTER = 0xa2,
            FM_TONE_HIGH_REGISTER_BASE = 0xa4,
            FM_CH3_OPERATOR4_TONE_HIGH_REGISTER = 0xa6,
            FM_CH3_OPERATOR3_TONE_LOW_REGISTER = 0xa8,
            FM_CH3_OPERATOR1_TONE_LOW_REGISTER = 0xa9,
            FM_CH3_OPERATOR2_TONE_LOW_REGISTER = 0xaa,
            FM_CH3_OPERATOR3_TONE_HIGH_REGISTER = 0xac,
            FM_CH3_OPERATOR1_TONE_HIGH_REGISTER = 0xad,
            FM_CH3_OPERATOR2_TONE_HIGH_REGISTER = 0xae,
            FM_ALGORITHM_FEEDBACK_REGISTER_BASE = 0xb0,
        };
        enum FMControl : int
        {
            FM_KEY_OFF_ALL_OPERATORS = 0x00,
            FM_KEY_ON_ALL_OPERATORS = 0xf0,
        };
        enum FMMask : int
        {
            FM_FNUMBER_HIGH_MASK = 0x07,
            FM_BLOCK_MASK = 0x07,
        };
        constexpr int BYTE_MASK = 0xff;
        constexpr int BYTE_SHIFT = 8;
        constexpr int FM_TOTAL_LEVEL_MAX = 0x7f;
        constexpr int FM_FNUMBER_LIMIT = 0x800;
        constexpr int FM_BLOCK_SHIFT = 3;
        constexpr int FM_BLOCK_COUNT = 8;
        constexpr int FM_BLOCK_MAX = 7;
        constexpr std::array<std::array<FMRegister, 2>, FMSound::OPERATOR_COUNT> FM_CH3_OPERATOR_TONE_REGISTERS = {{
            {FM_CH3_OPERATOR1_TONE_HIGH_REGISTER, FM_CH3_OPERATOR1_TONE_LOW_REGISTER},
            {FM_CH3_OPERATOR2_TONE_HIGH_REGISTER, FM_CH3_OPERATOR2_TONE_LOW_REGISTER},
            {FM_CH3_OPERATOR3_TONE_HIGH_REGISTER, FM_CH3_OPERATOR3_TONE_LOW_REGISTER},
            {FM_CH3_OPERATOR4_TONE_HIGH_REGISTER, FM_CH3_OPERATOR4_TONE_LOW_REGISTER},
        }};
    }

    // TLをvolume変化に使用するかどうか
    // [alg][op]
    const std::array<std::array<bool, FMSound::OPERATOR_COUNT>, 8> FMWrap::vol_tl_flag = {{
        {0, 0, 0, 1},
        {0, 0, 0, 1},
        {0, 0, 0, 1},
        {0, 0, 0, 1},
        {0, 0, 1, 1},
        {0, 1, 1, 1},
        {0, 1, 1, 1},
        {1, 1, 1, 1}}};

    const std::array<int, FMSound::OPERATOR_COUNT> FMWrap::op_table = {0, 2, 1, 3};

    FMWrap::FMWrap(FM::OPN& o) : opn(o)
    {
    }

    void FMWrap::SetSound(int ch, const FMSound& s)
    {
        assert(0 <= ch && ch < MUSIC_CHANNEL_FM_COUNT);

        sound[ch] = s;
        opn.SetReg(FM_ALGORITHM_FEEDBACK_REGISTER_BASE + ch, s.AlgFb);
        for (int op = 0; op < FMSound::OPERATOR_COUNT; op++)
        {
            int d = op_table[op] * FMSound::OPERATOR_COUNT + ch;
            opn.SetReg(FM_DETUNE_MULTIPLE_REGISTER_BASE + d, s.Op[op].DtMl);
            opn.SetReg(FM_TOTAL_LEVEL_REGISTER_BASE + d, s.Op[op].Tl);
            opn.SetReg(FM_KEY_SCALE_ATTACK_RATE_REGISTER_BASE + d, s.Op[op].KsAr);
            opn.SetReg(FM_DECAY_RATE_REGISTER_BASE + d, s.Op[op].Dr);
            opn.SetReg(FM_SUSTAIN_RATE_REGISTER_BASE + d, s.Op[op].Sr);
            opn.SetReg(FM_SUSTAIN_LEVEL_RELEASE_RATE_REGISTER_BASE + d, s.Op[op].SlRr);
        }
        // TL 設定
        SetVolume(ch, vol[ch]);
    }

    void FMWrap::SetVolume(int ch, int v)
    {
        assert(0 <= ch && ch < MUSIC_CHANNEL_FM_COUNT);

        vol[ch] = v;
        const FMSound& s = sound[ch];
        for (int op = 0; op < FMSound::OPERATOR_COUNT; op++)
        {
            int d = op_table[op] * FMSound::OPERATOR_COUNT + ch;
            if (vol_tl_flag[s.GetAlg()][op_table[op]])
                opn.SetReg(FM_TOTAL_LEVEL_REGISTER_BASE + d, std::min(s.Op[op].Tl + 4 * (16 - v), FM_TOTAL_LEVEL_MAX));
        }
    }

    void FMWrap::KeyOnOff(int ch, bool on)
    {
        assert(0 <= ch && ch < MUSIC_CHANNEL_FM_COUNT);

        int val;
        if (on)
            val = FM_KEY_ON_ALL_OPERATORS | ch;
        else
            val = FM_KEY_OFF_ALL_OPERATORS | ch;
        opn.SetReg(FM_KEY_ON_OFF_REGISTER, val);
    }

    void FMWrap::SetTone(int ch, int block, int fnumber, int pitch_offset)
    {
        assert(0 <= ch && ch < MUSIC_CHANNEL_FM_COUNT);

        if (ch == MUSIC_CHANNEL_FM_3)
        {
            std::array<int, FMSound::OPERATOR_COUNT> operator_fnumber = {fnumber, fnumber, fnumber, fnumber};
            SetOperatorTones(ch, block, operator_fnumber, pitch_offset);
        }
        else
        {
            SetToneReg(FM_TONE_HIGH_REGISTER_BASE + ch, FM_TONE_LOW_REGISTER_BASE + ch, block, fnumber + pitch_offset);
        }
    }

    void FMWrap::SetOperatorTones(int ch, int block, std::span<const int, FMSound::OPERATOR_COUNT> fnumber, int pitch_offset)
    {
        assert(ch == MUSIC_CHANNEL_FM_3);

        const FMSound& s = sound[ch];
        for (int op = 0; op < FMSound::OPERATOR_COUNT; op++)
        {
            int operator_block = block + (s.Op[op].Dt2 != 0 ? 1 : 0);
            SetToneReg(FM_CH3_OPERATOR_TONE_REGISTERS[op][0], FM_CH3_OPERATOR_TONE_REGISTERS[op][1], operator_block, fnumber[op] + pitch_offset);
        }
    }

    void FMWrap::SetToneReg(int highaddr, int lowaddr, int block, int fnumber)
    {
        while (fnumber >= FM_FNUMBER_LIMIT)
        {
            fnumber /= 2;
            block++;
        }

        if (block >= FM_BLOCK_COUNT)
        {
            block = FM_BLOCK_MAX;
        }

        opn.SetReg(highaddr, ((block & FM_BLOCK_MASK) << FM_BLOCK_SHIFT) | ((fnumber >> BYTE_SHIFT) & FM_FNUMBER_HIGH_MASK));
        opn.SetReg(lowaddr, fnumber & BYTE_MASK);
    }
} // namespace MusicCom
