#include "parser_test_support.h"

using namespace MusicCom;

void RunParserChannelTests(TestRunner& runner, Fixtures& fixtures)
{
    runner.Run("channel data and global tempo", [&] {
        auto music = fixtures.Parse("1:C4 V10 T150\n");
        Check(music != nullptr, "ParseMML returned null");
        Check(music->GetTempo() == 150, "global tempo was not stored");
        auto command = music->GetChannelHead(0);
        CheckCommand(*command, CommandType::TYPE_NOTE, {0, 16, 0});
        CheckCommand(*++command, CommandType::TYPE_VOLUME, {10, 0, 0});
    });

    runner.Run("last tempo in the same channel line wins", [&] {
        auto music = fixtures.Parse("1:T120 C64 T150 D64\n");
        Check(music->GetTempo() == 150, "last tempo in the line was not retained");
        auto command = music->GetChannelHead(0);
        CheckCommand(*command, CommandType::TYPE_NOTE, {0, 1, 0});
        CheckCommand(*++command, CommandType::TYPE_NOTE, {2, 1, 0});
        Check((++command)->GetType() == CommandType::TYPE_PAUSE, "tempo was stored in the command stream");
    });

    runner.Run("last tempo across lines of the same channel wins", [&] {
        auto music = fixtures.Parse("1:T90 C64\n1:T180 D64\n");
        Check(music->GetTempo() == 180, "last tempo across channel lines was not retained");
        auto command = music->GetChannelHead(0);
        CheckCommand(*command, CommandType::TYPE_NOTE, {0, 1, 0});
        Check((++command)->GetType() == CommandType::TYPE_PAUSE, "first channel line did not end with a pause");
        CheckCommand(*++command, CommandType::TYPE_NOTE, {2, 1, 0});
    });

    runner.Run("last tempo across different channels wins", [&] {
        auto music = fixtures.Parse("6:T200 A64\n1:T100 C64\n3:T240 E64\n");
        Check(music->GetTempo() == 240, "tempo did not follow source order across channels");
        CheckChannelCommand(*music, 5, CommandType::TYPE_NOTE, {9, 1, 0});
        CheckChannelCommand(*music, 0, CommandType::TYPE_NOTE, {0, 1, 0});
        CheckChannelCommand(*music, 2, CommandType::TYPE_NOTE, {4, 1, 0});
    });

    runner.Run("all music channels and multiple lines", [&] {
        auto music = fixtures.Parse("1:C64\n2:D64\n3:E64\n4:F64\n5:G64\n6:A64\n1:B64\n");
        const std::array<int, MUSIC_CHANNEL_COUNT> tones{0, 2, 4, 5, 7, 9};
        for (int channel = 0; channel < MUSIC_CHANNEL_COUNT; ++channel)
        {
            CheckChannelCommand(*music, channel, CommandType::TYPE_NOTE, {tones[channel], 1, 0});
        }
        auto first = music->GetChannelHead(0);
        CheckCommand(*first, CommandType::TYPE_NOTE, {0, 1, 0});
        ++first;
        Check(first->GetType() == CommandType::TYPE_PAUSE, "channel line did not end with a pause");
        CheckCommand(*++first, CommandType::TYPE_NOTE, {11, 1, 0});
    });

    runner.Run("rhythm part data", [&] {
        auto music = fixtures.Parse("D:@2 A64\n");
        Check(music->IsRhythmPartPresent(), "rhythm part was not stored");
        auto command = music->GetRhythmPartHead();
        CheckCommand(*command, CommandType::TYPE_TONE, {2, 0, 0});
        CheckCommand(*++command, CommandType::TYPE_NOTE, {9, 1, 0});
    });
}
