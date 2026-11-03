#include "parser_test_support.h"
#include <string>
#include <utility>
#include <vector>

using namespace MusicCom;

void RunParserNoteTests(TestRunner& runner, Fixtures& fixtures)
{
    const std::vector<std::pair<std::string, int>> notes = {
        {"C",0},{"D",2},{"E",4},{"F",5},{"G",7},{"A",9},{"B",11},{"C+",1},{"C#",1},{"D-",1}};
    for (const auto& [token, tone] : notes)
    {
        runner.Run("note " + token, [&] {
            auto music = fixtures.Parse("1:" + token + "64\n");
            CheckChannelCommand(*music, 0, CommandType::TYPE_NOTE, {tone, 1, 0});
        });
    }

    const std::vector<std::pair<std::string, int>> lengths = {
        {"1",64},{"2",32},{"4",16},{"8",8},{"16",4},{"32",2},{"64",1},
        {"2.",48},{"4.",24},{"8.",12},{"16.",6},{"32.",3}};
    for (const auto& [token, ticks] : lengths)
    {
        for (const std::string command : {"C", "L", "R", "W"})
        {
            runner.Run("length " + command + token, [&] {
                auto music = fixtures.Parse("1:" + command + token + " D64\n");
                const auto& item = *music->GetChannelHead(0);
                const auto type = command == "C" ? CommandType::TYPE_NOTE :
                                  command == "L" ? CommandType::TYPE_LENGTH :
                                  command == "R" ? CommandType::TYPE_REST : CommandType::TYPE_WAIT;
                Check(item.GetType() == type, "incorrect length command type");
                Check(item.GetArg(command == "C" ? 1 : 0) == ticks, "incorrect tick length");
            });
        }
    }
    for (const std::string token : {"0","3","5","65","128","1.","64.","4.."})
    {
        for (const std::string command : {"C", "L", "R", "W"})
        {
            runner.Run("reject length " + command + token, [&] {
                fixtures.Error("1:C64 " + command + token + " D64\n");
            });
        }
    }

    struct OptionalCase { const char* text; int ticks; };
    for (const auto& entry : std::array<OptionalCase, 3>{{{"C",0},{"C,4,",16},{"C,,4,,",16}}})
    {
        runner.Run(std::string("optional length ") + entry.text, [&] {
            auto music = fixtures.Parse(std::string("1:") + entry.text + " D64\n");
            CheckChannelCommand(*music, 0, CommandType::TYPE_NOTE, {0, entry.ticks, 0});
        });
    }

    runner.Run("tie command in channel data", [&] {
        auto music = fixtures.Parse("1:C64 & D64\n");
        auto command = music->GetChannelHead(0);
        CheckCommand(*command, CommandType::TYPE_NOTE, {0, 1, 0});
        CheckCommand(*++command, CommandType::TYPE_TIE);
        CheckCommand(*++command, CommandType::TYPE_NOTE, {2, 1, 0});
    });
    runner.Run("reject tie argument", [&] { fixtures.Error("1:C64 &1 D64\n"); });
    runner.Run("reject missing default length", [&] { fixtures.Error("1:C64 L D64\n"); });
}
