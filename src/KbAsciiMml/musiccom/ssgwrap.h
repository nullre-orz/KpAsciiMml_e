#pragma once

#include <array>
#include <span>

namespace FM
{
    class OPN;
}

namespace MusicCom
{
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
        void SetEffectFrame(int noise_period, std::span<const int, 2> tone_period, std::span<const int, 2> volume, std::span<const bool, 2> tone_enabled, std::span<const bool, 2> noise_enabled);
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

        enum SSGChannel : int
        {
            SSG_CHANNEL_A = 0,
            SSG_CHANNEL_B = 1,
            SSG_CHANNEL_C = 2,
            SSG_CHANNEL_COUNT = 3,
            SSG_EFFECT_CHANNEL_COUNT = 2,
        };
        struct ChannelState
        {
            bool tone = true;
            bool noise = false;
            bool keyon = false;
            bool env = false;
            int env_form = 0;
            int vol = 15;
        };
        struct EffectChannelState
        {
            bool tone = true;
            bool noise = false;
            bool keyon = false;
        };

        FM::OPN& opn;
        std::array<ChannelState, SSG_CHANNEL_COUNT> channel;
        std::array<EffectChannelState, SSG_EFFECT_CHANNEL_COUNT> effect_channel;
        bool effect_active_;
        int mixer_control_; // レジスタ07hのI/Oポート制御bit(D7-D6)
    };
} // namespace MusicCom
