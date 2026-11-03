#include "parser_test_support.h"

using namespace MusicCom;

void RunParserStrTests(TestRunner& runner, Fixtures& fixtures)
{
    runner.Run("defined STR and part forward reference", [&] {
        auto music = fixtures.Parse("1:$A$ D64\nSTR:A=C64\nSTR:B=$A$\n");
        Check(music->IsMacroPresent("A") && music->IsMacroPresent("B"), "STR definition is missing");
        CheckChannelCommand(*music, 0, CommandType::TYPE_MACRO);
        Check(music->GetChannelHead(0)->GetStrArg() == "A", "channel STR reference is incorrect");
        CheckCommand(*music->GetMacroHead("A"), CommandType::TYPE_NOTE, {0, 1, 0});
        CheckCommand(*music->GetMacroHead("B"), CommandType::TYPE_MACRO);
        Check(music->GetMacroHead("B")->GetStrArg() == "A", "nested STR reference is incorrect");
    });

    runner.Run("duplicate STR keeps the first definition", [&] {
        auto music = fixtures.Parse("STR:A=C64\nSTR:A=D64\nSTR:B=E64\n1:$A$ $B$\n");
        Check(music->IsMacroPresent("A") && music->IsMacroPresent("B"), "macro definition is missing");
        CheckCommand(*music->GetMacroHead("A"), CommandType::TYPE_NOTE, {0, 1, 0});
        CheckCommand(*music->GetMacroHead("B"), CommandType::TYPE_NOTE, {4, 1, 0});
        auto command = music->GetChannelHead(0);
        CheckCommand(*command, CommandType::TYPE_MACRO);
        Check(command->GetStrArg() == "A", "first channel STR reference is incorrect");
        ++command;
        CheckCommand(*command, CommandType::TYPE_MACRO);
        Check(command->GetStrArg() == "B", "second channel STR reference is incorrect");
    });
}
