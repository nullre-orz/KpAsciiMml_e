#define NOMINMAX
#include "test_support.h"
#include "../src/KbAsciiMml/musiccom/sounddata.h"
#include "../src/KbAsciiMml/musiccom/soundparser.h"
#include "../src/KbAsciiMml/musiccom/soundsequencer.h"
#include "../src/KbAsciiMml/musiccom/ssgwrap.h"
#include <fmgen/opna.h>
#include <algorithm>
#include <utility>
#include <vector>

using namespace MusicCom;

void RunSequencerTests(TestRunner& runner, Fixtures& fixtures)
{
    runner.Run("MML command execution limit", [&] {
        for (int count : {9998, 9999, 10000})
        {
            std::string commands;
            for (int i = 0; i < count; ++i) commands += "V1 ";
            auto music = fixtures.Parse("1:" + commands + "R64\n1:C64\n");
            ToneProbe probe(*music);
            probe.Initialize();
            probe.IncreaseFrame(0);
            const bool within_limit = count <= 9999;
            Check(probe.IsPlaying() == within_limit, "execution limit boundary is incorrect");
            if (!within_limit)
            {
                const int keyoffs = probe.KeyOffCount();
                probe.Resume();
                probe.IncreaseFrame(1000);
                Check(!probe.IsPlaying() && probe.KeyOnCount() == 0 && probe.KeyOffCount() == keyoffs, "stopped part resumed");
            }
        }
    });

    runner.Run("MML lookahead command limit", [&] {
        for (int count : {9998, 9999})
        {
            std::string commands;
            for (int i = 0; i < count; ++i) commands += "V1 ";
            auto music = fixtures.Parse("1:C64 " + commands + "R64\n");
            ToneProbe probe(*music);
            probe.Initialize();
            probe.IncreaseFrame(0);
            Check(probe.IsPlaying() == (count == 9998), "lookahead limit boundary is incorrect");
            if (count == 9999) { probe.Resume(); Check(!probe.IsPlaying(), "lookahead stop resumed"); }
        }
    });

    for (const auto& [tempo, expected] : std::vector<std::pair<int,int>>{{0,5427},{30,5427},{120,1356},{5880,27},{5881,7089},{65535,7089},{65536,5427}})
    {
        runner.Run("tempo " + std::to_string(tempo), [&] {
            auto music = fixtures.Parse("1:T" + std::to_string(tempo) + " C64\n");
            ToneProbe probe(*music);
            probe.Initialize();
            probe.IncreaseFrame(0);
            Check(probe.GetRemainFrameSize() == expected, "incorrect samples per tick");
        });
    }
    for (const auto& [octave, expected] : std::vector<std::pair<int,int>>{{0,1},{6,7},{7,7},{8,8}})
    {
        runner.Run("relative octave " + std::to_string(octave), [&] {
            auto music = fixtures.Parse("1:O" + std::to_string(octave) + " > C64\n");
            ToneProbe probe(*music);
            probe.Initialize();
            probe.IncreaseFrame(0);
            Check(probe.Octave() == expected, "incorrect octave at note start");
        });
    }
    runner.Run("D Y callback and paused command progression", [&] {
        auto music = fixtures.Parse("D:Y7,128 Y11,42\nD:Y12,99\n");
        FM::OPN opn;
        Check(opn.Init(3993600, 48000), "OPN init failed");
        SSGWrap ssg(opn);
        SoundData sound;
        std::vector<std::pair<int, int>> writes;
        SoundSequencer sequence([&](int address, int value) { writes.emplace_back(address, value); }, ssg, *music, sound, 195, 48000);
        sequence.Initialize();
        sequence.IncreaseFrame(0);
        Check(!sequence.IsPlaying(), "D did not pause at line end");
        Check(writes == std::vector<std::pair<int,int>>{{7,128},{11,42}}, "incorrect Y writes");
        sequence.IncreaseFrame(1000);
        Check(writes.size() == 2, "commands advanced while paused");
        sequence.Resume();
        sequence.IncreaseFrame(sequence.GetRemainFrameSize());
        Check(writes == std::vector<std::pair<int,int>>{{7,128},{11,42},{12,99}}, "resume did not execute next line");
    });
    runner.Run("D waiting sound reaches next frame and termination", [&] {
        fixtures.Write("SOUND.DAT", "Sound:@0,4,1,1\nTone1:100,30,3,1\nVol1:15,0,1,1\n");
        auto music = fixtures.Parse("D:A64\n");
        music->SetTempo(5880);
        auto sound = ParseSound((fixtures.Directory() / "case.mml").string());
        Check(sound->GetRhythm(0).length() == 4, "custom sound fixture not loaded");
        FM::OPN opn;
        Check(opn.Init(3993600, 48000), "OPN init failed");
        SSGWrap ssg(opn);
        SoundSequencer sequence([](int, int) {}, ssg, *music, *sound, 195, 48000);
        sequence.Initialize();
        sequence.IncreaseFrame(0);
        sequence.IncreaseFrame(27);
        Check(!sequence.IsPlaying(), "D did not reach line pause");
        Check(opn.GetReg(0) == 100, "sound advanced before its own tick");
        int elapsed = 27;
        for (const auto& [deadline, tone] : std::vector<std::pair<int,int>>{{830,110},{1660,120},{2490,130}})
        {
            while (elapsed < deadline)
            {
                const int step = std::min(sequence.GetRemainFrameSize(), deadline - elapsed);
                Check(step > 0, "scheduler stalled");
                sequence.IncreaseFrame(step);
                elapsed += step;
            }
            Check(!sequence.IsPlaying(), "paused command stream resumed unexpectedly");
            Check(opn.GetReg(0) == tone, "sound did not advance while paused");
        }
        while (elapsed < 3320)
        {
            const int step = std::min(sequence.GetRemainFrameSize(), 3320 - elapsed);
            Check(step > 0, "scheduler stalled at termination");
            sequence.IncreaseFrame(step);
            elapsed += step;
        }
        Check((opn.GetReg(7) & 0x1b) == 0x1b, "effect channels did not key off");
    });
}
