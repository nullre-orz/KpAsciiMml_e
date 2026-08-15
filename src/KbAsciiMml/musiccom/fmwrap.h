#pragma once

#include "musdata.h"

namespace FM
{
    class OPN;
}

namespace MusicCom
{
    // ch は0-origin
    class FMWrap
    {
    public:
        FMWrap(FM::OPN& o);
        void SetSound(int ch, const FMSound& sound);
        void SetTone(int ch, int block, int fnumber, int pitch_offset = 0);
        void SetOperatorTones(int ch, int block, const int fnumber[4], int pitch_offset = 0);
        void SetVolume(int ch, int vol);

        void KeyOnOff(int ch, bool on);

    private:
        void SetToneReg(int highaddr, int lowaddr, int block, int fnumber);

        FM::OPN& opn;
        FMSound sound[3];
        int vol[3];
        static const bool vol_tl_flag[8][4];
        static const int op_table[4];
    };

    class SSGWrap
    {
    public:
        SSGWrap(FM::OPN& o);

        void SetEnv(int ch, bool on);
        void SetEnvForm(int ch, int form);
        void SetEnvPeriod(int period);
        void PrepareKeyOn(int ch);
        void SetTonePeriod(int ch, int tone);
        void SetNoisePeriod(int period);
        void SetVolume(int ch, int vol);
        void KeyOnOff(int ch, bool on);

        void BeginEffect();
        void SetEffectFrame(int noise_period, const int tone_period[2], const int volume[2], const bool tone_enabled[2], const bool noise_enabled[2]);
        void EffectKeyOnOff(bool on);
        void EndEffect();

        void WriteMusicRegister(int address, int value);

    private:
        enum class WriteSource : int
        {
            MUSIC,
            EFFECT,
        };

        void WriteRegister(int address, int value, WriteSource source);

        void SetToneEnabled(int ch, bool on);
        void SetNoiseEnabled(int ch, bool on);
        // YM2203のSSGミキサーレジスタ(07h)を更新する
        void SetMixer(int value);
        void SetNoiseToneEnable();

        void SetEffectNoiseToneEnable();

        FM::OPN& opn;
        bool tone[3];
        bool noise[3];
        bool keyon[3];
        bool effect_tone_[2];
        bool effect_noise_[2];
        bool effect_keyon_[2];
        bool env[3];
        int env_form[3];
        int vol[3];
        bool effect_active_;
        int mixer_value_;
        int mixer_control_; // レジスタ07hのI/Oポート制御bit(D7-D6)
    };

} // namespace MusicCom
