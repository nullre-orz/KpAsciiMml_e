#include "parser_test_support.h"
#include <string>

using namespace MusicCom;

void RunParserEffectTests(TestRunner& runner, Fixtures& fixtures)
{
    struct Case { const char* text; CommandType type; int a, b, c; };
    const Case cases[] = {
        {"P3",CommandType::TYPE_PORTAMENTO,3,0,0},
        {"U",CommandType::TYPE_TREMOLO,0,0,0},
        {"U1",CommandType::TYPE_TREMOLO,1,0,0},
        {"I2,3",CommandType::TYPE_VIBRATO,2,3,0}};
    for (const auto& entry : cases)
    {
        runner.Run(std::string("effect ") + entry.text, [&] {
            auto music = fixtures.Parse(std::string("1:") + entry.text + " C64\n");
            CheckChannelCommand(*music, 0, entry.type, {entry.a, entry.b, entry.c});
        });
    }
    for (const std::string token : {"U1,2,3,4","I1,2,3,4"})
    {
        runner.Run("reject effect " + token, [&] { fixtures.Error("1:C64 " + token + " D64\n"); });
    }
}
