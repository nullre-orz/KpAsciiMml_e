#pragma once

#include <array>
#include <cstdint>
#include <map>
#include <tuple>
#include <vector>

namespace MusicCom
{
    struct Block
    {
        struct TonePart
        {
            bool enabled = false;
            int initial_value = 0;
            int final_value = 0;
            int period = 0;
            bool loop = false;
        };
        struct VolumePart
        {
            int initial_value = 0;
            int final_value = 0;
            int period = 0;
            bool loop = false;
        };
        struct NoisePart
        {
            int channel_type = 0;
            int initial_value = 0;
            int final_value = 0;
            int period = 0;
            bool loop = false;
        };

        int length = 0;
        std::array<TonePart, 2> tone{};
        std::array<VolumePart, 2> volume{};
        NoisePart noise{};
    };

    class RhythmData
    {
    public:
        RhythmData();

        // Element
        struct Element
        {
            std::array<int, 2> tone{};
            std::array<int, 2> volume{};
            int noise_period = 0;
            std::array<bool, 2> tone_enabled{};
            std::array<bool, 2> noise_enabled{};
        };

        // Iterator
        class const_iterator
        {
        public:
            using BlockIterator = std::vector<Block>::const_iterator;
            const_iterator(BlockIterator block_ptr, BlockIterator sentinel, int part_index);

            const_iterator& operator++();
            const_iterator operator++(int);
            bool operator==(const const_iterator& other) const;
            bool operator!=(const const_iterator& other) const;
            Element operator*() const;

        private:
            struct diff_cache
            {
                std::array<int, 2> tone{};
                std::array<int, 2> volume{};
                int noise = 0;
            };

            friend class RhythmData;
            void make_cache(bool diff_update);

            BlockIterator block_ptr_;
            BlockIterator sentinel_;
            int part_index_;

            diff_cache diff_cache_{};
            diff_cache value_cache_{};
            Element data_cache_{};
        };

        std::uint64_t length() const;
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
        std::map<int, RhythmData> rhythms;
    };
} // namespace MusicCom
