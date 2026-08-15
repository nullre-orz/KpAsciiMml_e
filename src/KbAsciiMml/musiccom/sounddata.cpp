#include "sounddata.h"
#include <algorithm>
#include <numeric>

namespace MusicCom
{
    RhythmData::RhythmData()
    {
    }

    int get_tone_increment(const Block::TonePart& part)
    {
        const int diff = part.final_value - part.initial_value;
        return diff / std::max(part.period, 1);
    }

    template<typename Part>
    int get_fixed_increment(const Part& part)
    {
        int diff = (part.final_value - part.initial_value) & 0xff;
        if ((diff & 0x80) != 0)
        {
            diff -= 0x100;
        }
        return (diff * 0x100) / std::max(part.period, 1);
    }

    template<typename Part>
    int get_period_index(const Part& part, int index)
    {
        const int period = std::max(part.period, 1);
        const int cycle = period + 1;
        if (index >= cycle)
        {
            // ループしない場合は周期回数分を加算した値のまま
            index = part.loop ? index % cycle : period;
        }
        return index;
    }

    int get_tone_value(const Block::TonePart& part, int increment, int index)
    {
        index = get_period_index(part, index);
        return std::clamp(part.initial_value + increment * index, 0, 0x0fff);
    }

    template<typename Part>
    int get_fixed_value(const Part& part, int increment, int index, int maximum)
    {
        index = get_period_index(part, index);
        const int initial_value = (part.initial_value & 0xff) << 8;
        const int value = std::clamp(initial_value + increment * index, 0, maximum << 8);
        return value >> 8;
    }

    RhythmData::const_iterator::const_iterator(BlockIterator block_ptr, BlockIterator sentinel, int part_index)
        : block_ptr_(block_ptr),
          sentinel_(sentinel),
          part_index_(part_index)
    {
        // キャッシュ計算
        make_cache(true);
    }

    RhythmData::const_iterator& RhythmData::const_iterator::operator++()
    {
        bool block_moved = false;
        ++part_index_;

        auto block = *block_ptr_;
        while (part_index_ >= block.length)
        {
            // 現在のブロックが終了した場合は次のブロックに移動
            ++block_ptr_;
            part_index_ = 0;
            block_moved = true;
        }
        // キャッシュ再計算
        make_cache(block_moved);

        return *this;
    }

    RhythmData::const_iterator RhythmData::const_iterator::operator++(int)
    {
        const_iterator return_value(*this);
        ++(*this);
        return return_value;
    }

    bool RhythmData::const_iterator::operator==(const_iterator& other) const
    {
        return (block_ptr_ == other.block_ptr_ && part_index_ == other.part_index_);
    }

    bool RhythmData::const_iterator::operator!=(const_iterator& other) const
    {
        return !(*this == other);
    }

    RhythmData::Element RhythmData::const_iterator::operator*() const
    {
        return data_cache_;
    }

    void RhythmData::const_iterator::make_cache(bool diff_update)
    {
        // 末尾に達している場合はキャッシュ計算しない
        if (block_ptr_ == sentinel_)
        {
            return;
        }

        // 現在のブロック
        auto block = *block_ptr_;

        if (diff_update)
        {
            // Toneは整数、VolumeとNoiseは8.8固定小数点形式で差分を保持する
            diff_cache_.tone[0] = get_tone_increment(block.tone[0]);
            diff_cache_.tone[1] = get_tone_increment(block.tone[1]);
            diff_cache_.volume[0] = get_fixed_increment(block.volume[0]);
            diff_cache_.volume[1] = get_fixed_increment(block.volume[1]);
            diff_cache_.noise = get_fixed_increment(block.noise);
        }

        // データキャッシュの計算
        data_cache_.tone[0] = get_tone_value(block.tone[0], diff_cache_.tone[0], part_index_);
        data_cache_.tone[1] = get_tone_value(block.tone[1], diff_cache_.tone[1], part_index_);
        data_cache_.volume[0] = get_fixed_value(block.volume[0], diff_cache_.volume[0], part_index_, 15);
        data_cache_.volume[1] = get_fixed_value(block.volume[1], diff_cache_.volume[1], part_index_, 15);
        data_cache_.noise_period = get_fixed_value(block.noise, diff_cache_.noise, part_index_, 63);
        data_cache_.tone_enabled[0] = block.tone[0].enabled;
        data_cache_.tone_enabled[1] = block.tone[1].enabled;
        data_cache_.noise_enabled[0] = (block.noise.channel_type & 0x1);
        data_cache_.noise_enabled[1] = (block.noise.channel_type & 0x2);
    }

    int RhythmData::length() const
    {
        return std::accumulate(
            blocks_.begin(),
            blocks_.end(),
            0,
            [](int acc, const Block& item)
            {
                return acc + item.length;
            });
    }

    RhythmData::const_iterator RhythmData::begin() const
    {
        auto begin = blocks_.begin();
        auto end = blocks_.end();
        auto iterator = const_iterator(begin, end, 0);
        return iterator;
    }

    RhythmData::const_iterator RhythmData::end() const
    {
        auto end = blocks_.end();
        return const_iterator(end, end, 0);
    }

    SoundData::SoundData()
    {
    }

    void SoundData::SetRhythm(int no, const RhythmData& rhythm)
    {
        rhythms[no] = rhythm;
    }

    const RhythmData& SoundData::GetRhythm(int no) const
    {
        return rhythms[no];
    }
} // namespace MusicCom
