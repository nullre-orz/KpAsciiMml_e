#include "parser_test_support.h"
#include <string>

using namespace MusicCom;

void RunParserControlTests(TestRunner& runner, Fixtures& fixtures)
{
    struct Case { const char* text; CommandType type; int a, b, c; };
    const Case cases[] = {
        {"V-1",CommandType::TYPE_VOLUME,255,0,0}, {"V255",CommandType::TYPE_VOLUME,255,0,0},
        {"V256",CommandType::TYPE_VOLUME,0,0,0}, {"V65535",CommandType::TYPE_VOLUME,255,0,0},
        {"V65536",CommandType::TYPE_VOLUME,0,0,0}, {"M-1",CommandType::TYPE_ENV_PERIOD,65535,0,0},
        {"M65536",CommandType::TYPE_ENV_PERIOD,0,0,0}, {"N-1",CommandType::TYPE_DETUNE,-1,0,0},
        {"N65535",CommandType::TYPE_DETUNE,-1,0,0}, {"N256",CommandType::TYPE_DETUNE,0,0,0},
        {"O0",CommandType::TYPE_OCTAVE,0,0,0}, {"O8",CommandType::TYPE_OCTAVE,8,0,0},
        {"O65536",CommandType::TYPE_OCTAVE,0,0,0}, {"@0",CommandType::TYPE_TONE,0,0,0},
        {"@20",CommandType::TYPE_TONE,20,0,0}, {"@65556",CommandType::TYPE_TONE,20,0,0},
        {"Q4",CommandType::TYPE_GATE_TIME,4,0,0}, {"S13",CommandType::TYPE_ENV_FORM,13,0,0}};
    for (const auto& entry : cases)
    {
        runner.Run(std::string("control ") + entry.text, [&] {
            auto music = fixtures.Parse(std::string("1:") + entry.text + " C64\n");
            CheckChannelCommand(*music, 0, entry.type, {entry.a, entry.b, entry.c});
        });
    }

    runner.Run("relative octave commands", [&] {
        auto music = fixtures.Parse("1:O4 < > C64\n");
        auto command = music->GetChannelHead(0);
        CheckCommand(*command, CommandType::TYPE_OCTAVE, {4, 0, 0});
        CheckCommand(*++command, CommandType::TYPE_OCTAVE_DOWN);
        CheckCommand(*++command, CommandType::TYPE_OCTAVE_UP);
        CheckCommand(*++command, CommandType::TYPE_NOTE, {0, 1, 0});
    });

    for (const std::string token : {"V10,20","Q4,8",">1","V+1","O9","O-1","@21","@-1"})
    {
        runner.Run("reject control " + token, [&] { fixtures.Error("1:C64 " + token + " D64\n"); });
    }
}
