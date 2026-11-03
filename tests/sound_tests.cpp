#include "test_support.h"
#include "../src/KbAsciiMml/musiccom/sounddata.h"
#include "../src/KbAsciiMml/musiccom/soundparser.h"
#include <array>
#include <vector>

using namespace MusicCom;

namespace
{
    std::vector<RhythmData::Element> Elements(const RhythmData& rhythm)
    {
        std::vector<RhythmData::Element> result;
        for (auto iterator = rhythm.begin(); iterator != rhythm.end(); ++iterator)
        {
            result.push_back(*iterator);
        }
        return result;
    }
}

void RunSoundDataTests(TestRunner& runner, Fixtures& fixtures)
{
    runner.Run("missing SOUND.DAT uses embedded definitions", [&] {
        auto data = ParseSound((fixtures.Directory() / "missing.mml").string());
        Check(data != nullptr, "missing SOUND.DAT returned null");
        Check(data->GetRhythm(0).length() == 8, "embedded SOUND.DAT was not loaded");
    });

    runner.Run("valid SOUND.DAT values and iterator", [&] {
        fixtures.Write("SOUND.DAT",
                       "Sound:@2,4,0,0 trailing text\n"
                       "Tone1:100,30,3,1\n"
                       "Vol1:20,-10,2,1\n"
                       "Noise:3,70,-20,2,1\n");
        auto data = ParseSound((fixtures.Directory() / "case.mml").string());
        const auto& rhythm = data->GetRhythm(2);
        Check(rhythm.length() == 4, "incorrect parsed sound length");
        const auto elements = Elements(rhythm);
        Check(elements.size() == 4, "incorrect iterator length");
        const std::array<int, 4> tones{100, 110, 120, 130};
        const std::array<int, 4> volumes{15, 15, 10, 10};
        const std::array<int, 4> noises{63, 60, 50, 50};
        for (size_t index = 0; index < elements.size(); ++index)
        {
            Check(elements[index].tone[0] == tones[index], "incorrect tone change");
            Check(elements[index].volume[0] == volumes[index], "incorrect volume change or saturation");
            Check(elements[index].noise_period == noises[index], "incorrect noise change or saturation");
            Check(elements[index].tone_enabled[0] && !elements[index].tone_enabled[1], "incorrect tone enable flags");
            Check(elements[index].noise_enabled[0] && elements[index].noise_enabled[1], "incorrect noise enable flags");
        }
        auto first = rhythm.begin();
        auto copy = first;
        Check(first == copy, "copied iterator differs at the same position");
        ++copy;
        Check(first != copy, "iterator comparison ignores the position");
    });

    runner.Run("SOUND.DAT omitted arguments and comma-free values", [&] {
        fixtures.Write("SOUND.DAT",
                       "Sound:@3\n"
                       "Tone1:400\n"
                       "Sound:@4 4 2 3 ignored\n"
                       "Tone2:90 -30 3 1\n");
        auto data = ParseSound((fixtures.Directory() / "case.mml").string());
        Check(data->GetRhythm(3).length() == 60, "omitted sound length did not use the default");
        const auto omitted = *data->GetRhythm(3).begin();
        Check(omitted.tone[0] == 400 && omitted.tone_enabled[0], "omitted tone arguments were not defaulted");
        const auto values = Elements(data->GetRhythm(4));
        Check(values.size() == 4, "comma-free sound length was not parsed");
        Check(values[0].tone[1] == 90 && values[1].tone[1] == 85 && values[3].tone[1] == 75,
              "comma-free values or tone multiplier were not applied");
    });

    runner.Run("invalid SOUND.DAT falls back to embedded definitions", [&] {
        fixtures.Write("SOUND.DAT", "Sound:@9,2,1,1\nTone3:100,10,1,1\ninvalid line");
        auto data = ParseSound((fixtures.Directory() / "case.mml").string());
        Check(data != nullptr, "invalid SOUND.DAT returned null");
        Check(data->GetRhythm(0).length() == 8, "invalid SOUND.DAT did not use embedded definitions");
        Check(data->GetRhythm(9).length() == 0, "partially parsed invalid data escaped fallback");
    });
    runner.Run("SOUND.DAT length wrapping and blocks", [&] {
        fixtures.Write("SOUND.DAT", "Sound:@9,65536\nTone1:100\nSound:65537\nTone1:200\nSound:2\nTone1:300\n");
        auto data = ParseSound((fixtures.Directory() / "case.mml").string());
        const auto& rhythm = data->GetRhythm(9);
        Check(rhythm.length() == 63, "wrapped or combined length is incorrect");
        auto it = rhythm.begin();
        for (int i = 0; i < 60; ++i) { Check(it != rhythm.end() && (*it).tone[0] == 100, "first block is incorrect"); ++it; }
        Check(it != rhythm.end() && (*it).tone[0] == 200, "second block is incorrect");
        ++it;
        for (int i = 0; i < 2; ++i) { Check(it != rhythm.end() && (*it).tone[0] == 300, "third block is incorrect"); ++it; }
        Check(it == rhythm.end(), "iterator did not reach the end");
        ++it;
        Check(it == rhythm.end(), "incrementing the end changed the iterator");
    });

    runner.Run("SOUND.DAT total length exceeds 32 bits", [&] {
        std::string sound = "Sound:@9,65535\n";
        for (int i = 0; i < 65537; ++i) sound += "Sound:65535\n";
        fixtures.Write("SOUND.DAT", sound);
        auto data = ParseSound((fixtures.Directory() / "case.mml").string());
        Check(data->GetRhythm(9).length() == 65538ULL * 65535, "total sound length overflowed");
    });

    runner.Run("SOUND.DAT signed arithmetic and loop", [&] {
        fixtures.Write("SOUND.DAT", "Sound:@9,5,1,1\nTone1:100,65526,2,0\nVol1:10,254,2,0\nNoise:1,10,254,2,0\n");
        auto data = ParseSound((fixtures.Directory() / "case.mml").string());
        const auto values = Elements(data->GetRhythm(9));
        Check(values.size() == 5, "sound length is incorrect");
        const std::array<int, 5> tones{100,95,90,100,95};
        const std::array<int, 5> levels{10,9,8,10,9};
        for (size_t i = 0; i < values.size(); ++i)
            Check(values[i].tone[0] == tones[i] && values[i].volume[0] == levels[i] && values[i].noise_period == levels[i], "signed change or looping is incorrect at " + std::to_string(i) + ": " + std::to_string(values[i].tone[0]) + "," + std::to_string(values[i].volume[0]) + "," + std::to_string(values[i].noise_period));
    });

    runner.Run("SOUND.DAT invalid division fallback", [&] {
        for (const auto& [header, line] : std::vector<std::pair<std::string, std::string>>{{"Sound:@9,2,2,2\n", "Tone1:100,1,32768,0"}, {"Sound:@9,2,1,1\n", "Tone1:100,-32768,65535,0"}, {"Sound:@9,2,1,1\n", "Vol1:10,-128,65535,0"}, {"Sound:@9,2,1,1\n", "Noise:1,10,-128,65535,0"}})
        {
            fixtures.Write("SOUND.DAT", header + line + "\n");
            auto data = ParseSound((fixtures.Directory() / "case.mml").string());
            Check(data && data->GetRhythm(0).length() == 8 && data->GetRhythm(9).length() == 0, "invalid arithmetic did not use embedded sounds");
        }
    });
}
