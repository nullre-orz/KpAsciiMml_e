#include "parser_test_support.h"
#include <string>

void RunParserErrorTests(TestRunner& runner, Fixtures& fixtures)
{
    runner.Run("MML error detail limit", [&] {
        for (int count : {99, 100, 101, 102})
        {
            std::string input;
            for (int i = 0; i < count; ++i) input += "1:X123\n";
            const auto error = fixtures.Error(input);
            Check(error.find("(99):") != std::string::npos, "earlier errors are missing");
            if (count >= 100) Check(error.find("(100):") != std::string::npos, "100th error is missing");
            Check(error.find("(101):") == std::string::npos, "error detail exceeds limit");
            const auto omitted = count > 100 ? std::to_string(count - 100) + " additional errors omitted" : "additional errors omitted";
            Check((error.find(omitted) != std::string::npos) == (count > 100), "omitted error count is incorrect");
        }
    });

    runner.Run("MML error excerpt limit", [&] {
        const auto error = fixtures.Error("1:X" + std::string(400, 'Z') + "\n");
        Check(error.find(std::string(255, 'Z')) != std::string::npos, "excerpt is too short");
        Check(error.find(std::string(256, 'Z')) == std::string::npos, "excerpt exceeds limit");
        Check(error.find("...\"") != std::string::npos, "truncated excerpt marker is missing");
    });

    runner.Run("reject unknown command in channel data", [&] {
        const auto error = fixtures.Error("1:C64 X123 D64\n");
        Check(error.find("(1):") != std::string::npos, "unknown command line is missing");
    });

    runner.Run("collect errors through last line", [&] {
        const auto error = fixtures.Error("STR:A=$MISSING$\n1:" + std::string(16, '{') + "C4" + std::string(16, '}') + "\n2:X123\n");
        Check(error.find("undefined STR") != std::string::npos, "missing STR error");
        Check(error.find("loop nesting too deep") != std::string::npos, "missing loop error");
        Check(error.find("(3):") != std::string::npos, "last line was not checked");
        Check(error.find("undefined STR") < error.find("loop nesting too deep") && error.find("loop nesting too deep") < error.find("(3):"),
              "parse errors are not reported in source order");
    });
}
