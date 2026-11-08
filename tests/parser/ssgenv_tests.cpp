#include "parser_test_support.h"
#include <vector>

using namespace MusicCom;

void RunParserSSGEnvTests(TestRunner& runner, Fixtures& fixtures)
{
    runner.Run("SSGENV values omissions replacement and channel use", [&] {
        auto music = fixtures.Parse(
            "SSGENV:@1,2,15,14,255,13\n"
            "SSGENV:@2\n"
            "SSGENV:@3,2,15\n"
            "->14\n"
            "SSGENV:@4,3,12\n"
            "SSGENV:@4\n"
            "4:@1 C64 @2 D64 @3 E64 @4 F64\n");
        Check(music->GetSSGEnv(1).Unit == 2 && music->GetSSGEnv(1).Env == std::vector<unsigned char>({15, 14}),
              "SSGENV terminator or values are incorrect");
        Check(music->GetSSGEnv(2).Unit == 1 && music->GetSSGEnv(2).Env.empty(), "omitted SSGENV values are incorrect");
        Check(music->GetSSGEnv(3).Unit == 2 && music->GetSSGEnv(3).Env == std::vector<unsigned char>({15, 14}),
              "continued SSGENV is incorrect");
        Check(music->GetSSGEnv(4).Unit == 1 && music->GetSSGEnv(4).Env.empty(), "empty SSGENV did not replace the previous definition");
        auto command = music->GetChannelHead(MUSIC_CHANNEL_SSG_A);
        for (int number = 1; number <= 4; ++number)
        {
            CheckCommand(*command, CommandType::TYPE_TONE, {number, 0, 0});
            ++command;
            Check(command->GetType() == CommandType::TYPE_NOTE, "SSG note was not stored after envelope selection");
            ++command;
        }
    });

    runner.Run("SSGENV errors use the header line", [&] {
        const auto error = fixtures.Error("SSGENV:@21,1,15\n->14\n4:@1 C64\n");
        Check(error.find("(1):") != std::string::npos, "SSGENV error does not use the header line");
    });
}
