#include "parser_test_support.h"

using namespace MusicCom;

void RunParserDirectRegisterTests(TestRunner& runner, Fixtures& fixtures)
{
    runner.Run("Y omitted data", [&] {
        auto music = fixtures.Parse("1:Y7 C64\n");
        CheckChannelCommand(*music, 0, CommandType::TYPE_DIRECT, {7, 0, 0});
    });
    runner.Run("Y address and data", [&] {
        auto music = fixtures.Parse("1:Y7,128 C64\n");
        CheckChannelCommand(*music, 0, CommandType::TYPE_DIRECT, {7, 128, 0});
    });
    runner.Run("reject Y excess argument", [&] { fixtures.Error("1:C64 Y7,1,2 D64\n"); });
}
