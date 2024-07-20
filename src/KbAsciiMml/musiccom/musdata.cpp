#include "musdata.h"

using namespace std;

namespace MusicCom
{
    MusicData::MusicData()
    {
        fill_n(channel_present, channel_count, false);
        tempo = 120;
    }

    CommandIterator MusicData::GetChannelHead(int channel) const
    {
        assert(IsChannelPresent(channel));

        return channels[channel].begin();
    }

    CommandIterator MusicData::GetChannelTail(int channel) const
    {
        assert(IsChannelPresent(channel));

        return channels[channel].end();
    }

    CommandIterator MusicData::GetRhythmPartHead() const
    {
        assert(IsRhythmPartPresent());

        return rhythm_part.begin();
    }

    CommandIterator MusicData::GetRhythmPartTail() const
    {
        assert(IsRhythmPartPresent());

        return rhythm_part.end();
    }

    CommandIterator MusicData::GetMacroHead(const string& name) const
    {
        assert(IsMacroPresent(name));

        return macros.find(name)->second.begin();
    }

    void MusicData::AddCommandToChannel(int channel, const Command& command)
    {
        CommandList& cl = channels[channel];
        if (!IsChannelPresent(channel))
        {
            channel_present[channel] = true;
        }

        cl.insert(cl.end(), command);
    }

    void MusicData::AddCommandToRhythmPart(const Command& command)
    {
        CommandList& cl = rhythm_part;
        if (!IsRhythmPartPresent())
        {
            rhythm_part_present = true;
        }

        cl.insert(cl.end(), command);
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
