#pragma once

#include "fmwrap.h"
#include "musdata.h"
#include "partdata.h"
#include <stack>

namespace FM
{
    class OPN;
}

namespace MusicCom
{
    // 適当すぎ
    class Sequencer
    {
    public:
        Sequencer(FM::OPN& o, MusicData* pmd);
        bool Init(int rate);
        void Mix(__int16* dest, int nsamples);
        int GetSamplesPerFrame()
        {
            return samplesPerFrame;
        }

    private:
        void NextFrame();
        void NextFramePart(int ch);
        void KeyOnOff(int ch, bool on);
        std::optional<CommandIterator> ProcessLoop(PartData& part, CommandIterator ptr);
        void ProcessCommand(int ch);
        void ProcessEffect(int ch);
        std::optional<CommandType> FindLinkedItem(const PartData& part, CommandIterator ptr);
        int GetFmTone(int base_tone, int detune) const;
        int GetSsgTone(int base_octave, int base_tone, int detune) const;

        static const int* const FNumber;
        static const int FNumberBase[14];
        static const int SSGToneNum[10][12];

        PartData partData[6];

        FM::OPN& opn;
        FMWrap fmwrap;
        SSGWrap ssgwrap;
        MusicData& musicdata;

        int rate;
        int samplesPerFrame;
        int samplesLeft; // このフレーム(64分音符)でmixすべき残りのサンプル数
        int currentFrame;
        bool partPlaying[6];
    };

} // namespace MusicCom
