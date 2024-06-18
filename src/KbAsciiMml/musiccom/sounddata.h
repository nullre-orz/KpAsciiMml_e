#pragma once

#include <map>
#include <tuple>
#include <vector>

namespace MusicCom
{
    struct TonePart
    {
        int initial_value;
        int final_value;
        int period;
        bool loop;
    };
    using VolumePart = TonePart;
    struct NoisePart
    {
        int channel_type;
        int initial_value;
        int final_value;
        int period;
        bool loop;
    };

    struct Block
    {
        int length;
        TonePart tone[2];
        VolumePart volume[2];
        NoisePart noise;
    };

    struct RhythmData
    {
        std::vector<Block> blocks;
    };

    class SoundData
    {
    public:
        SoundData();

        void SetRhythm(int no, const RhythmData& rhythm);
        const RhythmData& GetRhythm(int no);

    private:
        std::map<int, RhythmData> rhythms;
    };
} // namespace MusicCom
