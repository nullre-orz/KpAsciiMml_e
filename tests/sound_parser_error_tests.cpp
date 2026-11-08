#include "test_support.h"
#include "../src/KbAsciiMml/musiccom/soundparser.cpp"
#include <stdexcept>
#include <string>

namespace
{
    std::string ParseError(const std::string& input)
    {
        try
        {
            MusicCom::ParseSoundImpl(input.begin(), input.end(), "SOUND.DAT");
        }
        catch (const std::runtime_error& error)
        {
            return error.what();
        }
        throw std::runtime_error("invalid SOUND.DAT was accepted");
    }
}

void RunSoundParserErrorTests(TestRunner& runner, Fixtures&)
{
    runner.Run("SOUND.DAT error detail limit", [] {
        for (int count : {99, 100, 101, 102})
        {
            std::string input;
            for (int i = 0; i < count; ++i) input += "invalid\n";
            const auto error = ParseError(input);
            Check(error.find("(99):") != std::string::npos, "earlier errors are missing");
            if (count >= 100) Check(error.find("(100):") != std::string::npos, "100th error is missing");
            Check(error.find("(101):") == std::string::npos, "error detail exceeds limit");
            const auto omitted = count > 100 ? std::to_string(count - 100) + " additional errors omitted" : "additional errors omitted";
            Check((error.find(omitted) != std::string::npos) == (count > 100), "omitted error count is incorrect");
        }
    });
    runner.Run("SOUND.DAT error excerpt limit", [] {
        const auto error = ParseError(std::string(400, 'Z') + "\n");
        Check(error.find(std::string(256, 'Z') + "...\"") != std::string::npos, "excerpt or truncation marker is incorrect");
        Check(error.find(std::string(257, 'Z')) == std::string::npos, "excerpt exceeds limit");
    });
}
