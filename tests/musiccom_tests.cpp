#include "test_support.h"
#include "../src/KbAsciiMml/musiccom/musiccom.h"
#include <stdexcept>
#include <vector>
#include <algorithm>
#include <limits>

using namespace MusicCom;

void RunMusicComTests(TestRunner& runner, Fixtures& fixtures)
{
    runner.Run("sample rate boundaries and prescaler", [&] {
        const auto path = fixtures.Write("rate.mml", "4:T5880 Y47,0 O4 C64\n");
        ::MusicCom::MusicCom player;
        Check(player.Load(path.string().c_str()), "sample rate fixture did not load");
        const auto max_rate = static_cast<unsigned>((std::numeric_limits<int>::max)());
        for (const auto rate : {0u, 7999u, max_rate + 1u})
            Check(!player.PrepareMix(rate), "invalid sample rate was accepted");
        for (const auto rate : {8000u, 48000u, 55466u, max_rate})
        {
            Check(player.PrepareMix(rate), "valid sample rate was rejected");
            std::vector<FM_SAMPLETYPE> pcm(256 * 2);
            player.Mix(pcm.data(), 256);
        }
    });

    runner.Run("invalid sample rate preserves playback", [&] {
        const auto path = fixtures.Write("rate_state.mml",
            "SOUND:@0\nLFO:0,0,0,7,0\n"
            "OP1:31,0,0,15,0,0,0,1,0,0\n"
            "OP2:31,0,0,15,0,0,0,1,0,0\n"
            "OP3:31,0,0,15,0,0,0,1,0,0\n"
            "OP4:31,0,0,15,0,0,0,1,0,0\n"
            "1:T120 @0 V15 Q8 O4 C1 D1\n");
        ::MusicCom::MusicCom player;
        ::MusicCom::MusicCom control;
        Check(player.Load(path.string().c_str()) && control.Load(path.string().c_str()), "playback did not load");
        Check(player.PrepareMix(48000) && control.PrepareMix(48000), "playback did not initialize");
        std::vector<FM_SAMPLETYPE> first(256 * 2), control_first(256 * 2);
        player.Mix(first.data(), 256);
        control.Mix(control_first.data(), 256);
        Check(first == control_first, "initial playback differs from control");
        Check(std::any_of(first.begin(), first.end(), [](auto sample) { return sample != 0; }), "reference PCM is silent");
        Check(!player.PrepareMix(0), "zero sample rate was accepted");
        Check(!player.PrepareMix(7999), "low sample rate was accepted");
        Check(!player.PrepareMix(static_cast<unsigned>((std::numeric_limits<int>::max)()) + 1u), "high sample rate was accepted");
        std::vector<FM_SAMPLETYPE> next(256 * 2), control_next(256 * 2);
        player.Mix(next.data(), 256);
        control.Mix(control_next.data(), 256);
        Check(next == control_next, "invalid sample rate changed playback state");
    });

    runner.Run("FM restart and failed MML load preserve playback", [&] {
        for (int channel = 1; channel <= 3; ++channel)
        {
            const auto path = fixtures.Write("restart.mml",
                "SOUND:@0\nLFO:0,0,0,7,0\n"
                "OP1:31,0,0,15,0,0,0,1,0,0\n"
                "OP2:31,0,0,15,0,0,0,1,0,0\n"
                "OP3:31,0,0,15,0,0,0,1,0,0\n"
                "OP4:31,0,0,15,0,0,0,1,0,0\n" +
                std::to_string(channel) + ":T120 @0 V15 Q8 O4 C1 D1\n");
            ::MusicCom::MusicCom player;
            Check(player.Load(path.string().c_str()) && player.PrepareMix(48000), "restart fixture did not initialize");
            std::vector<FM_SAMPLETYPE> first(2048 * 2);
            player.Mix(first.data(), 2048);
            Check(std::any_of(first.begin(), first.end(), [](auto sample) { return sample != 0; }),
                  "restart reference PCM is silent");
            for (int restart = 0; restart < 2; ++restart)
            {
                Check(player.PrepareMix(48000), "restart initialization failed");
                std::vector<FM_SAMPLETYPE> replay(first.size());
                player.Mix(replay.data(), 2048);
                Check(first == replay, "restart during FM key on changed the opening PCM");
            }
        }

        fixtures.Write("SOUND.DAT", "Sound:@0,2,1,1\nTone1:100,10,1,1\nVol1:15,0,1,1\n");
        const auto mml_path = fixtures.Write("case.mml", "1:T5880 O4 C64\n");

        ::MusicCom::MusicCom retained;
        Check(retained.Load(mml_path.string().c_str()), "valid MML did not load");
        fixtures.Write("case.mml", "1:X123\n");
        bool rejected = false;
        try
        {
            retained.Load(mml_path.string().c_str());
        }
        catch (const std::runtime_error&)
        {
            rejected = true;
        }
        Check(rejected, "invalid replacement MML was accepted");
        Check(retained.PrepareMix(48000), "previous data was unavailable after failed load");
        std::vector<FM_SAMPLETYPE> retained_pcm(256 * 2);
        retained.Mix(retained_pcm.data(), 256);

        fixtures.Write("case.mml", "1:T5880 O4 C64\n");
        ::MusicCom::MusicCom control;
        Check(control.Load(mml_path.string().c_str()) && control.PrepareMix(48000), "control MML did not initialize");
        std::vector<FM_SAMPLETYPE> control_pcm(256 * 2);
        control.Mix(control_pcm.data(), 256);
        Check(retained_pcm == control_pcm, "failed load changed the previously loaded playback data");
    });
}
