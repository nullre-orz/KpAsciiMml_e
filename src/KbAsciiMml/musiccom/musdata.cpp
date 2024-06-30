﻿#include "musdata.h"

using namespace std;

namespace MusicCom
{
    MusicData::MusicData()
    {
        fill_n(channel_present, channel_count, false);
        tempo = 120;
    }

    CommandIterator MusicData::GetChannelHead(int index) const
    {
        assert(IsChannelPresent(index));

        return channels[index].begin();
    }

    CommandIterator MusicData::GetRhythmPartHead() const
    {
        assert(IsRhythmPartPresent());

        return rhythm_part.begin();
    }

    CommandIterator MusicData::GetMacroHead(const string& name) const
    {
        assert(IsMacroPresent(name));

        return macros.find(name)->second.begin();
    }

    void MusicData::AddCommandToChannel(int index, const Command& command)
    {
        CommandList& cl = channels[index];
        if (!IsChannelPresent(index))
        {
            // 終了検知のためパート終端を追加
            cl.push_back(Command(CommandType::TYPE_END));
            channel_present[index] = true;
        }

        cl.insert(--cl.end(), command);
    }

    void MusicData::AddCommandToRhythmPart(const Command& command)
    {
        CommandList& cl = rhythm_part;
        if (!IsRhythmPartPresent())
        {
            // 終了検知のためパート終端を追加
            cl.push_back(Command(CommandType::TYPE_END));
            rhythm_part_present = true;
        }

        cl.insert(--cl.end(), command);
    }

    void MusicData::AddCommandToMacro(std::string name, const Command& command)
    {
        if (!IsMacroPresent(name))
        {
            CommandList& cl = macros[name];
            // マクロからのreturnを追加
            cl.push_back(Command(CommandType::TYPE_RETURN));
        }

        CommandList& cl = macros[name];
        cl.insert(--cl.end(), command);
    }

} // namespace MusicCom
