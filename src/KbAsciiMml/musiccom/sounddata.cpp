#include "sounddata.h"
#include <algorithm>
#include <limits>
#include <numeric>

namespace MusicCom
{
    namespace
    {
        const RhythmData DEFAULT_RHYTHM{};
    }

    RhythmData::RhythmData()
    {
    }

    namespace
    {
        int signed_word(int value)
        {
            value &= 0xffff;
            return value < 0x8000 ? value : value - 0x10000;
        }

        int divisor_for_period(int period)
        {
            return period == 0 ? 1 : signed_word(period);
        }

        int get_tone_increment(const Block::TonePart& part)
        {
            return signed_word(part.final_value - part.initial_value) / divisor_for_period(part.period);
        }

        template<typename Part>
        int get_fixed_increment(const Part& part)
        {
            const int change = (part.final_value - part.initial_value) & 0xff;
            return signed_word(change << 8) / divisor_for_period(part.period);
        }

        template<typename Part>
        int update_value(const Part& part, int increment, int index, int initial, int maximum, int& current, bool reset)
        {
            const int period = std::max(part.period, 1);
            const int position = part.loop ? index % (period + 1) : index;
            if (reset || (part.loop && position == 0))
            {
                current = initial;
            }
            else if (part.loop || position <= period)
            {
                current = std::clamp(signed_word(current + increment), 0, maximum);
            }
            // ループしない場合は周期回数分を加算した値のまま
            return std::clamp(current, 0, maximum);
        }

    }

    RhythmData::const_iterator::const_iterator(BlockIterator block_ptr, BlockIterator sentinel, int part_index)
        : block_ptr_(block_ptr),
          sentinel_(sentinel),
          part_index_(part_index)
    {
        while (block_ptr_ != sentinel_ && block_ptr_->length <= 0)
            ++block_ptr_;
        // キャッシュ計算
        make_cache(true);
    }

    RhythmData::const_iterator& RhythmData::const_iterator::operator++()
    {
        if (block_ptr_ == sentinel_)
            return *this;

        bool block_moved = false;
        ++part_index_;

        while (block_ptr_ != sentinel_ && part_index_ >= block_ptr_->length)
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

    bool RhythmData::const_iterator::operator==(const const_iterator& other) const
    {
        return (block_ptr_ == other.block_ptr_ && part_index_ == other.part_index_);
    }

    bool RhythmData::const_iterator::operator!=(const const_iterator& other) const
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
        const auto& block = *block_ptr_;

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
        // 更新ごとに16bitで循環させた後、元実装と同じ範囲に収める
        data_cache_.tone[0] = update_value(block.tone[0], diff_cache_.tone[0], part_index_, block.tone[0].initial_value, 0x0fff, value_cache_.tone[0], diff_update);
        data_cache_.tone[1] = update_value(block.tone[1], diff_cache_.tone[1], part_index_, block.tone[1].initial_value, 0x0fff, value_cache_.tone[1], diff_update);
        data_cache_.volume[0] = update_value(block.volume[0], diff_cache_.volume[0], part_index_, (block.volume[0].initial_value & 0xff) << 8, 15 << 8, value_cache_.volume[0], diff_update) >> 8;
        data_cache_.volume[1] = update_value(block.volume[1], diff_cache_.volume[1], part_index_, (block.volume[1].initial_value & 0xff) << 8, 15 << 8, value_cache_.volume[1], diff_update) >> 8;
        data_cache_.noise_period = update_value(block.noise, diff_cache_.noise, part_index_, (block.noise.initial_value & 0xff) << 8, 63 << 8, value_cache_.noise, diff_update) >> 8;
        data_cache_.tone_enabled[0] = block.tone[0].enabled;
        data_cache_.tone_enabled[1] = block.tone[1].enabled;
        data_cache_.noise_enabled[0] = (block.noise.channel_type & 0x1);
        data_cache_.noise_enabled[1] = (block.noise.channel_type & 0x2);
    }

    std::uint64_t RhythmData::length() const
    {
        return std::accumulate(
            blocks_.begin(),
            blocks_.end(),
            std::uint64_t{0},
            [](std::uint64_t acc, const Block& item)
            {
                const auto length = static_cast<std::uint64_t>(std::max(item.length, 0));
                const auto maximum = std::numeric_limits<std::uint64_t>::max();
                return length > maximum - acc ? maximum : acc + length;
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
        auto it = rhythms.find(no);
        return it != rhythms.end() ? it->second : DEFAULT_RHYTHM;
    }
} // namespace MusicCom
