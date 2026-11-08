#include "parser_test_support.h"
#include <string>

using namespace MusicCom;

void RunParserLoopTests(TestRunner& runner, Fixtures& fixtures)
{
    runner.Run("loop commands in channel data", [&] {
        auto music = fixtures.Parse("1:{2 C64 } D64\n");
        auto command = music->GetChannelHead(0);
        CheckCommand(*command, CommandType::TYPE_LOOP, {2, 0, 0});
        CheckCommand(*++command, CommandType::TYPE_NOTE, {0, 1, 0});
        CheckCommand(*++command, CommandType::TYPE_EXIT);
        CheckCommand(*++command, CommandType::TYPE_NOTE, {2, 1, 0});
    });

    for (const std::string prefix : {"1:","D:","STR:A="})
    {
        runner.Run("loop depth " + prefix, [&] {
            auto music = fixtures.Parse(prefix + std::string(15, '{') + "C4" + std::string(15, '}') + "\n");
            if (prefix == "1:")
            {
                CheckChannelCommand(*music, 0, CommandType::TYPE_LOOP);
            }
            else if (prefix == "D:")
            {
                Check(music->IsRhythmPartPresent(), "D loop data was not stored");
                CheckCommand(*music->GetRhythmPartHead(), CommandType::TYPE_LOOP);
            }
            else
            {
                Check(music->IsMacroPresent("A"), "STR loop data was not stored");
                CheckCommand(*music->GetMacroHead("A"), CommandType::TYPE_LOOP);
            }
            Check(fixtures.Error(prefix + std::string(16, '{') + "C4" + std::string(16, '}') + "\n").find("loop nesting too deep") != std::string::npos,
                  "missing depth error");
        });
    }
}
