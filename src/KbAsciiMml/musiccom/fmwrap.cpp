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

        enum SSGRegister : int
        {
            SSG_TONE_PERIOD_LOW_REGISTER_BASE = 0x00,
            SSG_TONE_PERIOD_HIGH_REGISTER_BASE = 0x01,
            SSG_NOISE_PERIOD_REGISTER = 0x06,
            SSG_MIXER_REGISTER = 0x07,
            SSG_VOLUME_REGISTER_BASE = 0x08,
            SSG_ENV_PERIOD_LOW_REGISTER = 0x0b,
            SSG_ENV_PERIOD_HIGH_REGISTER = 0x0c,
            SSG_ENV_FORM_REGISTER = 0x0d,
        };

        enum FMControl : int
        {
            FM_KEY_OFF_ALL_OPERATORS = 0x00,
            FM_KEY_ON_ALL_OPERATORS = 0xf0,
        };

        enum SSGControl : int
        {
            SSG_ENV_ENABLE = 0x10,
            SSG_MIXER_IO_INITIAL_VALUE = 0x80,
        };

        enum FMMask : int
        {
            FM_FNUMBER_HIGH_MASK = 0x07,
            FM_BLOCK_MASK = 0x07,
        };

        enum SSGMask : int
        {
            SSG_TONE_PERIOD_HIGH_MASK = 0x0f,
            SSG_NOISE_PERIOD_MASK = 0x1f,
            SSG_VOLUME_MASK = 0x0f,
            SSG_MIXER_IO_MASK = 0xc0,
        };

        constexpr int BYTE_MASK = 0xff;
        constexpr int BYTE_SHIFT = 8;
        constexpr int FM_TOTAL_LEVEL_MAX = 0x7f;
        constexpr int FM_FNUMBER_LIMIT = 0x800;
        constexpr int FM_BLOCK_SHIFT = 3;
        constexpr int FM_BLOCK_COUNT = 8;
        constexpr int FM_BLOCK_MAX = 7;
        constexpr int SSG_NOISE_DISABLE_SHIFT = 3;

        constexpr FMRegister FM_CH3_OPERATOR_TONE_REGISTERS[4][2] = {
            {FM_CH3_OPERATOR1_TONE_HIGH_REGISTER, FM_CH3_OPERATOR1_TONE_LOW_REGISTER},
            {FM_CH3_OPERATOR2_TONE_HIGH_REGISTER, FM_CH3_OPERATOR2_TONE_LOW_REGISTER},
            {FM_CH3_OPERATOR3_TONE_HIGH_REGISTER, FM_CH3_OPERATOR3_TONE_LOW_REGISTER},
            {FM_CH3_OPERATOR4_TONE_HIGH_REGISTER, FM_CH3_OPERATOR4_TONE_LOW_REGISTER},
        };
    }

    // TLをvolume変化に使用するかどうか
    // [alg][op]
    const bool FMWrap::vol_tl_flag[8][4] = {
        {0, 0, 0, 1},
        {0, 0, 0, 1},
        {0, 0, 0, 1},
        {0, 0, 0, 1},
        {0, 0, 1, 1},
        {0, 1, 1, 1},
        {0, 1, 1, 1},
        {1, 1, 1, 1}};

    const int FMWrap::op_table[4] = {0, 2, 1, 3};

    FMWrap::FMWrap(FM::OPN& o) : opn(o)
    {
        fill_n(vol, 3, 15);
    }

    void FMWrap::SetSound(int ch, const FMSound& s)
    {
        assert(0 <= ch && ch < 3);

        sound[ch] = s;
        opn.SetReg(FM_ALGORITHM_FEEDBACK_REGISTER_BASE + ch, s.AlgFb);
        for (int op = 0; op < 4; op++)
        {
            int d = op_table[op] * 4 + ch;
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
        assert(0 <= ch && ch < 3);

        vol[ch] = v;
        const FMSound& s = sound[ch];
        for (int op = 0; op < 4; op++)
        {
            int d = op_table[op] * 4 + ch;
            if (vol_tl_flag[s.GetAlg()][op_table[op]])
                opn.SetReg(FM_TOTAL_LEVEL_REGISTER_BASE + d, std::min(s.Op[op].Tl + 4 * (16 - v), FM_TOTAL_LEVEL_MAX));
        }
    }

    void FMWrap::KeyOnOff(int ch, bool on)
    {
        assert(0 <= ch && ch < 3);

        int val;
        if (on)
            val = FM_KEY_ON_ALL_OPERATORS | ch;
        else
            val = FM_KEY_OFF_ALL_OPERATORS | ch;
        opn.SetReg(FM_KEY_ON_OFF_REGISTER, val);
    }

    void FMWrap::SetTone(int ch, int block, int fnumber, int pitch_offset)
    {
        assert(0 <= ch && ch < 3);

        if (ch == 2)
        {
            int operator_fnumber[4] = {fnumber, fnumber, fnumber, fnumber};
            SetOperatorTones(ch, block, operator_fnumber, pitch_offset);
        }
        else
        {
            SetToneReg(FM_TONE_HIGH_REGISTER_BASE + ch, FM_TONE_LOW_REGISTER_BASE + ch, block, fnumber + pitch_offset);
        }
    }

    void FMWrap::SetOperatorTones(int ch, int block, const int fnumber[4], int pitch_offset)
    {
        assert(ch == 2);

        const FMSound& s = sound[ch];
        for (int op = 0; op < 4; op++)
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

    SSGWrap::SSGWrap(FM::OPN& o) : opn(o), mixer_control_(SSG_MIXER_IO_INITIAL_VALUE)
    {
        fill_n(tone, 3, true);
        fill_n(noise, 3, false);
        fill_n(keyon, 3, false);
        fill_n(env, 3, false);
        fill_n(env_form, 3, 0);
        fill_n(vol, 3, 15);
    }

    void SSGWrap::SetEnvForm(int ch, int form)
    {
        assert(0 <= ch && ch < 3);
        env_form[ch] = form;
    }

    void SSGWrap::SetEnvPeriod(int period)
    {
        opn.SetReg(SSG_ENV_PERIOD_LOW_REGISTER, period & BYTE_MASK);
        opn.SetReg(SSG_ENV_PERIOD_HIGH_REGISTER, (period >> BYTE_SHIFT) & BYTE_MASK);
    }

    void SSGWrap::PrepareKeyOn(int ch)
    {
        assert(0 <= ch && ch < 3);

        if (env[ch])
        {
            opn.SetReg(SSG_VOLUME_REGISTER_BASE + ch, SSG_ENV_ENABLE);
            opn.SetReg(SSG_ENV_FORM_REGISTER, env_form[ch]);
        }
        else
        {
            opn.SetReg(SSG_VOLUME_REGISTER_BASE + ch, vol[ch] & SSG_VOLUME_MASK);
        }
    }

    void SSGWrap::SetTonePeriod(int ch, int tone)
    {
        assert(0 <= ch && ch < 3);

        int d = ch * 2;
        opn.SetReg(SSG_TONE_PERIOD_LOW_REGISTER_BASE + d, tone & BYTE_MASK);
        opn.SetReg(SSG_TONE_PERIOD_HIGH_REGISTER_BASE + d, (tone >> BYTE_SHIFT) & SSG_TONE_PERIOD_HIGH_MASK);
    }
    void SSGWrap::SetNoisePeriod(int period)
    {
        opn.SetReg(SSG_NOISE_PERIOD_REGISTER, period & SSG_NOISE_PERIOD_MASK);
    }

    void SSGWrap::SetVolume(int ch, int v)
    {
        assert(0 <= ch && ch < 3);
        env[ch] = false;
        vol[ch] = v;
        opn.SetReg(SSG_VOLUME_REGISTER_BASE + ch, v & SSG_VOLUME_MASK);
    }

    void SSGWrap::SetEffectVolume(int ch, int v)
    {
        assert(0 <= ch && ch < 3);
        opn.SetReg(SSG_VOLUME_REGISTER_BASE + ch, v & SSG_VOLUME_MASK);
    }

    void SSGWrap::SetMixer(int value)
    {
        mixer_control_ = value & SSG_MIXER_IO_MASK;
        SetNoiseToneEnable();
    }

    void SSGWrap::SetEnv(int ch, bool on)
    {
        assert(0 <= ch && ch < 3);
        env[ch] = on;
    }
    void SSGWrap::SetToneEnabled(int ch, bool on)
    {
        assert(0 <= ch && ch < 3);
        tone[ch] = on;
    }
    void SSGWrap::SetNoiseEnabled(int ch, bool on)
    {
        assert(0 <= ch && ch < 3);
        noise[ch] = on;
    }
    void SSGWrap::KeyOnOff(int ch, bool on)
    {
        assert(0 <= ch && ch < 3);
        keyon[ch] = on;
        SetNoiseToneEnable();
    }

    void SSGWrap::SetNoiseToneEnable()
    {
        int val = 0;
        for (int ch = 0; ch < 3; ch++)
        {
            int n = static_cast<int>(!(noise[ch] && keyon[ch]) << SSG_NOISE_DISABLE_SHIFT);
            int t = static_cast<int>(!(tone[ch] && keyon[ch]));
            val |= (n | t) << ch;
        }
        val |= mixer_control_;

        opn.SetReg(SSG_MIXER_REGISTER, val);
    }

} // namespace MusicCom
