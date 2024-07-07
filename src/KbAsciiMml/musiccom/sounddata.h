#pragma once

#include <map>
#include <tuple>
#include <vector>

namespace MusicCom
{
    struct Block
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

        int length;
        TonePart tone[2];
        VolumePart volume[2];
        NoisePart noise;
    };

    class RhythmData
    {
    public:
        RhythmData();

        // Element
        struct Element
        {
            int tone[2];
            int volume[2];
            int noise_period;
            bool noise_enabled[2];
        };

        // Iterator
        class const_iterator
        {
        public:
            using BlockIterator = std::vector<Block>::const_iterator;
            const_iterator(BlockIterator block_ptr, BlockIterator sentinel, int part_index);

            const_iterator& operator++();
            const_iterator operator++(int);
            bool operator==(const_iterator& other) const;
            bool operator!=(const_iterator& other) const;
            Element operator*() const;

        private:
            struct diff_cache
            {
                int tone[2];
                double volume[2];
                double noise;
            };

            friend class RhythmData;
            void make_cache(bool diff_update);

            BlockIterator block_ptr_;
            BlockIterator sentinel_;
            int part_index_;

            diff_cache diff_cache_;
            Element data_cache_;
        };

        int length() const;
        const_iterator begin() const;
        const_iterator end() const;

    private:
        friend struct SoundParserState;
        std::vector<Block> blocks_;
    };

    class SoundData
    {
    public:
        SoundData();

        void SetRhythm(int no, const RhythmData& rhythm);
        const RhythmData& GetRhythm(int no) const;

    private:
        mutable std::map<int, RhythmData> rhythms;
    };
} // namespace MusicCom
