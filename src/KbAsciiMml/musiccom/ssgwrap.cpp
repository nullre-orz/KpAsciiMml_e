#include "ssgwrap.h"
#include <cassert>
#include <fmgen/opna.h>

namespace MusicCom
{
    namespace
    {
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
        constexpr int SSG_ENV_ENABLE = 0x10;
        constexpr int SSG_MIXER_IO_INITIAL_VALUE = 0x80;
        constexpr int SSG_TONE_PERIOD_HIGH_MASK = 0x0f;
        constexpr int SSG_NOISE_PERIOD_MASK = 0x1f;
        constexpr int SSG_VOLUME_MASK = 0x0f;
        constexpr int SSG_MIXER_IO_MASK = 0xc0;
        constexpr int BYTE_MASK = 0xff;
        constexpr int BYTE_SHIFT = 8;
        constexpr int SSG_NOISE_DISABLE_SHIFT = 3;
        bool IsEffectReservedRegister(int address)
        {
            return address <= SSG_TONE_PERIOD_HIGH_REGISTER_BASE + 2 ||
                   address == SSG_NOISE_PERIOD_REGISTER ||
                   address == SSG_VOLUME_REGISTER_BASE ||
                   address == SSG_VOLUME_REGISTER_BASE + 1;
        }
    }

    SSGWrap::SSGWrap(FM::OPN& o)
        : opn(o),
          effect_active_(false),
          mixer_control_(SSG_MIXER_IO_INITIAL_VALUE)
    {
    }
    void SSGWrap::SetEnv(int ch, bool on)
    {
        assert(0 <= ch && ch < SSG_CHANNEL_COUNT);
        channel[ch].env = on;
    }
    void SSGWrap::SetEnvForm(int ch, int form)
    {
        assert(0 <= ch && ch < SSG_CHANNEL_COUNT);
        channel[ch].env_form = form;
    }
    void SSGWrap::SetEnvPeriod(int period)
    {
        WriteRegister(SSG_ENV_PERIOD_LOW_REGISTER, period & BYTE_MASK, WriteSource::MUSIC);
        WriteRegister(SSG_ENV_PERIOD_HIGH_REGISTER, (period >> BYTE_SHIFT) & BYTE_MASK, WriteSource::MUSIC);
    }
    void SSGWrap::PrepareKeyOn(int ch)
    {
        assert(0 <= ch && ch < SSG_CHANNEL_COUNT);
        if (channel[ch].env)
        {
            WriteRegister(SSG_VOLUME_REGISTER_BASE + ch, SSG_ENV_ENABLE, WriteSource::MUSIC);
            WriteRegister(SSG_ENV_FORM_REGISTER, channel[ch].env_form, WriteSource::MUSIC);
        }
        else
        {
            WriteRegister(SSG_VOLUME_REGISTER_BASE + ch, channel[ch].vol & SSG_VOLUME_MASK, WriteSource::MUSIC);
        }
    }
    void SSGWrap::SetTonePeriod(int ch, int tone)
    {
        assert(0 <= ch && ch < SSG_CHANNEL_COUNT);
        int d = ch * 2;
        WriteRegister(SSG_TONE_PERIOD_LOW_REGISTER_BASE + d, tone & BYTE_MASK, WriteSource::MUSIC);
        WriteRegister(SSG_TONE_PERIOD_HIGH_REGISTER_BASE + d, (tone >> BYTE_SHIFT) & SSG_TONE_PERIOD_HIGH_MASK, WriteSource::MUSIC);
    }
    void SSGWrap::SetNoisePeriod(int period)
    {
        WriteRegister(SSG_NOISE_PERIOD_REGISTER, period & SSG_NOISE_PERIOD_MASK, WriteSource::MUSIC);
    }
    void SSGWrap::SetVolume(int ch, int v)
    {
        assert(0 <= ch && ch < SSG_CHANNEL_COUNT);
        channel[ch].env = false;
        channel[ch].vol = v;
        WriteRegister(SSG_VOLUME_REGISTER_BASE + ch, v & SSG_VOLUME_MASK, WriteSource::MUSIC);
    }
    void SSGWrap::KeyOnOff(int ch, bool on)
    {
        assert(0 <= ch && ch < SSG_CHANNEL_COUNT);
        channel[ch].keyon = on;
        SetNoiseToneEnable();
    }
    void SSGWrap::BeginEffect()
    {
        effect_active_ = true;
        effect_channel.fill({});
    }
    void SSGWrap::SetEffectFrame(int noise_period, std::span<const int, 2> tone_period, std::span<const int, 2> volume, std::span<const bool, 2> tone_enabled, std::span<const bool, 2> noise_enabled)
    {
        WriteRegister(SSG_NOISE_PERIOD_REGISTER, noise_period & SSG_NOISE_PERIOD_MASK, WriteSource::EFFECT);
        for (int ch = 0; ch < SSG_EFFECT_CHANNEL_COUNT; ch++)
        {
            int d = ch * 2;
            WriteRegister(SSG_TONE_PERIOD_LOW_REGISTER_BASE + d, tone_period[ch] & BYTE_MASK, WriteSource::EFFECT);
            WriteRegister(SSG_TONE_PERIOD_HIGH_REGISTER_BASE + d, (tone_period[ch] >> BYTE_SHIFT) & SSG_TONE_PERIOD_HIGH_MASK, WriteSource::EFFECT);
            WriteRegister(SSG_VOLUME_REGISTER_BASE + ch, volume[ch] & SSG_VOLUME_MASK, WriteSource::EFFECT);
            effect_channel[ch].tone = tone_enabled[ch];
            effect_channel[ch].noise = noise_enabled[ch];
        }
        SetEffectNoiseToneEnable();
    }
    void SSGWrap::EffectKeyOnOff(bool on)
    {
        for (auto& channel : effect_channel)
        {
            channel.keyon = on;
        }
        SetEffectNoiseToneEnable();
    }
    void SSGWrap::EndEffect()
    {
        effect_active_ = false;
    }
    void SSGWrap::WriteMusicRegister(int address, int value)
    {
        if (address == SSG_MIXER_REGISTER)
        {
            SetMixer(value);
            return;
        }
        WriteRegister(address, value, WriteSource::MUSIC);
    }
    void SSGWrap::WriteRegister(int address, int value, WriteSource source)
    {
        // 効果音が使用するSSGチャンネルA/Bのレジスタ書込みだけを抑止する
        if (effect_active_ && source == WriteSource::MUSIC && IsEffectReservedRegister(address))
        {
            return;
        }

        opn.SetReg(address, value);
    }
    void SSGWrap::SetToneEnabled(int ch, bool on)
    {
        assert(0 <= ch && ch < SSG_CHANNEL_COUNT);
        channel[ch].tone = on;
    }
    void SSGWrap::SetNoiseEnabled(int ch, bool on)
    {
        assert(0 <= ch && ch < SSG_CHANNEL_COUNT);
        channel[ch].noise = on;
    }
    void SSGWrap::SetMixer(int value)
    {
        mixer_control_ = value & SSG_MIXER_IO_MASK;
        if (effect_active_)
        {
            SetEffectNoiseToneEnable();
        }
        else
        {
            SetNoiseToneEnable();
        }
    }
    void SSGWrap::SetNoiseToneEnable()
    {
        int val = 0;
        for (int ch = 0; ch < SSG_CHANNEL_COUNT; ch++)
        {
            int n = static_cast<int>(!(channel[ch].noise && channel[ch].keyon) << SSG_NOISE_DISABLE_SHIFT);
            int t = static_cast<int>(!(channel[ch].tone && channel[ch].keyon));
            val |= (n | t) << ch;
        }
        val |= mixer_control_;
        if (effect_active_)
        {
            SetEffectNoiseToneEnable();
        }
        else
        {
            WriteRegister(SSG_MIXER_REGISTER, val, WriteSource::MUSIC);
        }
    }
    void SSGWrap::SetEffectNoiseToneEnable()
    {
        int val = mixer_control_;
        for (int ch = 0; ch < SSG_EFFECT_CHANNEL_COUNT; ch++)
        {
            int n = static_cast<int>(!(effect_channel[ch].noise && effect_channel[ch].keyon) << SSG_NOISE_DISABLE_SHIFT);
            int t = static_cast<int>(!(effect_channel[ch].tone && effect_channel[ch].keyon));
            val |= (n | t) << ch;
        }
        int n = static_cast<int>(!(channel[SSG_CHANNEL_C].noise && channel[SSG_CHANNEL_C].keyon) << SSG_NOISE_DISABLE_SHIFT);
        int t = static_cast<int>(!(channel[SSG_CHANNEL_C].tone && channel[SSG_CHANNEL_C].keyon));
        val |= (n | t) << SSG_CHANNEL_C;
        WriteRegister(SSG_MIXER_REGISTER, val, WriteSource::EFFECT);
    }
} // namespace MusicCom
