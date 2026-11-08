#pragma once

#include "musdata.h"
#include <array>
#include <span>

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
        void SetOperatorTones(int ch, int block, std::span<const int, FMSound::OPERATOR_COUNT> fnumber, int pitch_offset = 0);
        void SetVolume(int ch, int vol);

        void KeyOnOff(int ch, bool on);

    private:
        void SetToneReg(int highaddr, int lowaddr, int block, int fnumber);

        FM::OPN& opn;
        std::array<FMSound, MUSIC_CHANNEL_FM_COUNT> sound{};
        std::array<int, MUSIC_CHANNEL_FM_COUNT> vol{15, 15, 15};
        static const std::array<std::array<bool, FMSound::OPERATOR_COUNT>, 8> vol_tl_flag;
        static const std::array<int, FMSound::OPERATOR_COUNT> op_table;
    };
} // namespace MusicCom
