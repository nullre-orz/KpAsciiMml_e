#include "test_support.h"

void RunBehaviorTests(TestRunner& runner, Fixtures& fixtures)
{
    runner.Run("note state and Q gate calculation", [&] {
        auto music = fixtures.Parse("1:Q4 C4\n");
        ToneProbe probe(*music);
        probe.Initialize();
        probe.IncreaseFrame(0);
        const auto& state = probe.State();
        Check(state.Note.BeginFrame == 0 && state.Note.EndFrame == 16, "incorrect note frame range");
        Check(state.Note.KeyOnFrame == 0 && state.Note.KeyOffFrame == 8, "incorrect Q gate frame");
        Check(probe.KeyOnCount() == 1 && probe.KeyOffCount() == 0, "incorrect initial key state");
    });

    runner.Run("tie suppresses second key on", [&] {
        auto music = fixtures.Parse("1:C4 & C4\n");
        ToneProbe probe(*music);
        probe.Initialize();
        probe.IncreaseFrame(0);
        for (int tick = 0; tick < 16; ++tick)
        {
            probe.IncreaseFrame(probe.GetRemainFrameSize());
        }
        Check(probe.KeyOnCount() == 1, "tie retriggered the note");
        Check(probe.KeyOffCount() == 0, "tie keyed off between notes");
        Check(probe.State().Note.EndFrame == 32, "tied note did not advance to the second note");
    });

    runner.Run("P U I exclusivity and zero periods", [&] {
        auto coexist = fixtures.Parse("1:P3 U2,0,0 C64\n");
        ToneProbe coexist_probe(*coexist);
        coexist_probe.Initialize();
        coexist_probe.IncreaseFrame(0);
        Check(coexist_probe.State().Effect.PortamentoLength == 3, "U disabled P");
        Check(coexist_probe.State().Effect.TremoloDepth == 2 && coexist_probe.State().Effect.TremoloLength == 1, "U0 period was not normalized");
        Check(coexist_probe.PortamentoCount() == 1, "P was not processed");

        auto vibrato = fixtures.Parse("1:P3 I4,0,0 C64\n");
        ToneProbe vibrato_probe(*vibrato);
        vibrato_probe.Initialize();
        vibrato_probe.IncreaseFrame(0);
        Check(vibrato_probe.State().Effect.PortamentoLength == 0, "I did not disable P");
        Check(vibrato_probe.State().Effect.VibratoDepth == 4 && vibrato_probe.State().Effect.VibratoLength == 1, "I0 period was not normalized");
        Check(vibrato_probe.VibratoCount() == 1 && vibrato_probe.LastVibratoDepth() == 4, "I was not processed");

        auto portamento = fixtures.Parse("1:I4,2,0 U2,2,0 P3 C64\n");
        ToneProbe portamento_probe(*portamento);
        portamento_probe.Initialize();
        portamento_probe.IncreaseFrame(0);
        Check(portamento_probe.State().Effect.VibratoDepth == 0, "P did not disable I");
        Check(portamento_probe.State().Effect.TremoloDepth == 2, "P disabled U");
        Check(portamento_probe.State().Effect.PortamentoLength == 3, "P was not retained");
    });

    runner.Run("pause resume and fresh initialization", [&] {
        auto music = fixtures.Parse("1:C64\n1:D64\n");
        ToneProbe probe(*music);
        probe.Initialize();
        probe.IncreaseFrame(0);
        probe.IncreaseFrame(probe.GetRemainFrameSize());
        Check(!probe.IsPlaying(), "part did not pause at line end");
        const int key_on_before_pause = probe.KeyOnCount();
        probe.IncreaseFrame(1000);
        Check(probe.KeyOnCount() == key_on_before_pause, "paused part advanced");
        probe.Resume();
        probe.IncreaseFrame(probe.GetRemainFrameSize());
        Check(probe.KeyOnCount() == key_on_before_pause + 1, "resume did not play the next line");
        ToneProbe fresh_probe(*music);
        fresh_probe.Initialize();
        Check(fresh_probe.IsPlaying() && fresh_probe.KeyOnCount() == 0 && fresh_probe.KeyOffCount() == 0,
              "fresh initialization has incorrect playback state");
        fresh_probe.IncreaseFrame(0);
        Check(fresh_probe.KeyOnCount() == 1 && fresh_probe.State().Note.Tone == 0,
              "fresh sequencer did not start from the first line");
    });
}
