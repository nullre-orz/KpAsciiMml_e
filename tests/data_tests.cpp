#include "test_support.h"
#include "../src/KbAsciiMml/musiccom/command.h"
#include "../src/KbAsciiMml/musiccom/sounddata.h"
#include <array>

using namespace MusicCom;

void RunDataTests(TestRunner& runner, Fixtures&)
{
    runner.Run("MusicData defaults and undefined definitions", [] {
        const MusicData data;
        Check(data.GetTempo() == 120, "incorrect default tempo");
        for (int channel = 0; channel < MUSIC_CHANNEL_COUNT; ++channel)
        {
            Check(!data.IsChannelPresent(channel), "default channel is present");
        }
        Check(!data.IsRhythmPartPresent(), "default rhythm part is present");
        const auto& fm1 = data.GetFMSound(1);
        const auto& fm2 = data.GetFMSound(2);
        Check(&fm1 == &fm2, "undefined FM sounds do not share the default");
        Check(fm1.AlgFb == 0, "undefined FM sound is not zero initialized");
        for (const auto& op : fm1.Op)
        {
            Check(op.DtMl == 0 && op.Tl == 0 && op.KsAr == 0 && op.Dr == 0 && op.Sr == 0 && op.SlRr == 0 && op.Dt2 == 0,
                  "undefined FM operator is not zero initialized");
        }
        const auto& env1 = data.GetSSGEnv(1);
        const auto& env2 = data.GetSSGEnv(2);
        Check(&env1 == &env2, "undefined SSG envelopes do not share the default");
        Check(env1.Unit == 64 && env1.Env == std::vector<unsigned char>{0}, "incorrect undefined SSG envelope");
    });

    runner.Run("SoundData default and undefined rhythms", [] {
        const SoundData data;
        const auto& rhythm1 = data.GetRhythm(1);
        const auto& rhythm2 = data.GetRhythm(2);
        Check(&rhythm1 == &rhythm2, "undefined rhythms do not share the default");
        Check(rhythm1.length() == 0, "undefined rhythm is not empty");
        Check(rhythm1.begin() == rhythm1.end(), "undefined rhythm iterator is not empty");
    });

    runner.Run("Command and PartData defaults", [] {
        const Command command;
        Check(command.GetType() == CommandType::TYPE_UNKNOWN, "incorrect default command type");
        Check(command.GetArg(0) == 0 && command.GetArg(1) == 0 && command.GetArg(2) == 0, "default command arguments are not zero");
        const PartData part;
        Check(part.Note.Octave == 4 && part.Note.ReservedOctave == 4, "incorrect default octave");
        Check(part.Note.Volume == 15 && part.Note.DefaultLength == 64 && part.Note.GateTime == 8, "incorrect default note state");
        Check(!part.Playback.Playing && !part.Playback.HasPreviousNote && !part.Playback.InfiniteLooping, "incorrect default playback state");
        Check(part.Effect.PortamentoLength == 0 && part.Effect.VibratoDepth == 0 && part.Effect.TremoloDepth == 0, "incorrect default effect state");
    });

    runner.Run("FMSound setters and Clear", [] {
        FMSound sound;
        sound.SetLFO(0x101, 0x1ff, -1);
        sound.SetAlgFb(9, 10);
        sound.SetDtMl(0, 9, 17);
        sound.SetTl(1, 0xff);
        sound.SetKsAr(2, 7, 0x3f);
        sound.SetDr(3, 0x3f);
        sound.SetSr(0, 0x3f);
        sound.SetSlRr(1, 0x1f, 0x1f);
        sound.SetDt2(2, 4);
        sound.SetDt2(3, 0xff);
        Check(sound.LFOForm == 1 && sound.LFOSpeed == 0xff && sound.LFODepth == 0xff, "incorrect LFO byte conversion");
        Check(sound.AlgFb == 0x11 && sound.GetAlg() == 1, "incorrect algorithm/feedback conversion");
        Check(sound.Op[0].DtMl == 0x11 && sound.Op[0].Sr == 0x1f, "incorrect operator 1 conversion");
        Check(sound.Op[1].Tl == 0x7f && sound.Op[1].SlRr == 0xff, "incorrect operator 2 conversion");
        Check(sound.Op[2].KsAr == 0xdf && sound.Op[2].Dt2 == 3, "incorrect operator 3 conversion");
        Check(sound.Op[3].Dr == 0x1f && sound.Op[3].Dt2 == 3, "incorrect operator 4 conversion");
        sound.Clear();
        Check(sound.LFOForm == 0 && sound.LFOSpeed == 0 && sound.LFODepth == 0 && sound.AlgFb == 0, "FMSound header was not cleared");
        for (const auto& op : sound.Op)
        {
            Check(op.DtMl == 0 && op.Tl == 0 && op.KsAr == 0 && op.Dr == 0 && op.Sr == 0 && op.SlRr == 0 && op.Dt2 == 0,
                  "FMSound operator was not cleared");
        }
    });
}
