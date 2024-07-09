#include "sounddata.h"
#include <cmath>
#include <numeric>

namespace MusicCom
{
    RhythmData::RhythmData()
    {
    }

    template<typename ReturnType, typename Part>
    ReturnType get_incremental(Part part)
    {
        int diff = (part.final_value - part.initial_value);
        return diff / static_cast<ReturnType>(std::max(part.period, 1));
    };

    template<typename DiffType, typename Part>
    int get_value(const Part& part, DiffType diff, int index, int length)
    {
        // indexが周期を超えている場合は補正
        int period = std::max(part.period + 1, 1);
        if (index > period)
        {
            // ループしない場合は最終値のまま
            index = (part.loop) ? (index % period) : period;
        }

        return std::max(part.initial_value + static_cast<int>(std::round(diff * index)), 0);
    };

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
            // 差分キャッシュの計算
            // Toneの変化量は小数以下切り捨てのためint, それ以外はdouble
            diff_cache_.tone[0] = get_incremental<int>(block.tone[0]);
            diff_cache_.tone[1] = get_incremental<int>(block.tone[1]);
            diff_cache_.volume[0] = get_incremental<double>(block.volume[0]);
            diff_cache_.volume[1] = get_incremental<double>(block.volume[1]);
            diff_cache_.noise = get_incremental<double>(block.noise);
        }

        // データキャッシュの計算
        int len = block.length;
        data_cache_.tone[0] = get_value(block.tone[0], diff_cache_.tone[0], part_index_, len);
        data_cache_.tone[1] = get_value(block.tone[1], diff_cache_.tone[1], part_index_, len);
        data_cache_.volume[0] = get_value(block.volume[0], diff_cache_.volume[0], part_index_, len);
        data_cache_.volume[1] = get_value(block.volume[1], diff_cache_.volume[1], part_index_, len);
        data_cache_.noise_period = get_value(block.noise, diff_cache_.noise, part_index_, len);
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
