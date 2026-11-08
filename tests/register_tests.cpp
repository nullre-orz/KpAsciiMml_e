#define NOMINMAX
#include "test_support.h"
#include "../src/KbAsciiMml/musiccom/ssgwrap.h"
#include <fmgen/opna.h>

using namespace MusicCom;

void RunRegisterTests(TestRunner& runner, Fixtures&)
{
    runner.Run("SSG registers envelope mixer and effect exclusion", [] {
        FM::OPN opn;
        Check(opn.Init(3993600, 48000), "OPN init failed");
        SSGWrap ssg(opn);
        ssg.SetTonePeriod(2, 0x1234);
        Check(opn.GetReg(4) == 0x34 && opn.GetReg(5) == 0x02, "SSG tone period is incorrect");
        ssg.SetNoisePeriod(0x3f);
        Check(opn.GetReg(6) == 0x1f, "SSG noise period mask is incorrect");
        ssg.SetVolume(2, 20);
        Check(opn.GetReg(10) == 4, "SSG volume mask is incorrect");
        ssg.SetEnv(2, true);
        ssg.SetEnvForm(2, 13);
        ssg.SetEnvPeriod(0x1234);
        ssg.PrepareKeyOn(2);
        Check(opn.GetReg(11) == 0x34 && opn.GetReg(12) == 0x12, "SSG envelope period is incorrect");
        Check(opn.GetReg(10) == 0x10 && opn.GetReg(13) == 13, "SSG envelope key-on preparation is incorrect");
        ssg.WriteMusicRegister(7, 0xc0);
        ssg.KeyOnOff(2, true);
        Check(opn.GetReg(7) == 0xfb, "SSG mixer or I/O control bits are incorrect");

        ssg.SetVolume(0, 7);
        ssg.BeginEffect();
        ssg.SetVolume(0, 3);
        ssg.SetVolume(2, 4);
        Check(opn.GetReg(8) == 7, "effect did not reserve SSG channel A");
        Check(opn.GetReg(10) == 4, "effect incorrectly reserved SSG channel C");
        ssg.EndEffect();
        ssg.SetVolume(0, 3);
        Check(opn.GetReg(8) == 3, "SSG channel A did not resume after the effect");
    });
}
