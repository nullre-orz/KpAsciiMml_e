#include "musdata.h"

using namespace std;

namespace MusicCom
{
    namespace
    {
        const FMSound DEFAULT_FM_SOUND{};
        const SSGEnv DEFAULT_SSG_ENV{0};
    }

    MusicData::MusicData()
    {
        channel_present.fill(false);
        tempo = 120;
    }

    const FMSound& MusicData::GetFMSound(int no) const
    {
        auto it = fmsounds.find(no);
        return it != fmsounds.end() ? it->second : DEFAULT_FM_SOUND;
    }

    const SSGEnv& MusicData::GetSSGEnv(int no) const
    {
        auto it = ssgenvs.find(no);
        return it != ssgenvs.end() ? it->second : DEFAULT_SSG_ENV;
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

    void MusicData::DefineMacro(const string& name)
    {
        if (!IsMacroPresent(name))
        {
            CommandList& cl = macros[name];
            // マクロからのreturnを追加
            cl.push_back(Command(CommandType::TYPE_RETURN));
        }
    }

    void MusicData::AddCommandToMacro(const string& name, const Command& command)
    {
        assert(IsMacroPresent(name));

        CommandList& cl = macros[name];
        cl.insert(--cl.end(), command);
    }

} // namespace MusicCom
