#pragma once

#include "command.h"
#include <optional>
#include <stack>
#include <utility>

namespace MusicCom
{
    struct PartData
    {
        PartData() : CallStack(),
                     LoopStack(),
                     CommandPtr(),
                     NoteBeginFrame(0),
                     NoteEndFrame(0),
                     KeyOnFrame(0),
                     KeyOffFrame(0),
                     Octave(4),
                     LastOctave(4),
                     ReservedOctave(4),
                     Volume(15),
                     Tone(0),
                     LastTone(0),
                     SSGEnvOn(false),
                     SoundNo(0),
                     DefaultNoteLength(64),
                     Detune(0),
                     GateTime(8),
                     PLength(0),
                     ILength(0),
                     IDepth(0),
                     IDelay(0),
                     ULength(0),
                     UDepth(0),
                     UDelay(0),
                     LinkedItem(std::nullopt),
                     Playing(false),
                     InfiniteLooping(false)
        {
        }

        std::stack<CommandIterator> CallStack;
        std::stack<std::pair<CommandIterator, int>> LoopStack;
        CommandIterator CommandPtr;

        int NoteBeginFrame;
        int NoteEndFrame;
        int KeyOnFrame;
        int KeyOffFrame;

        int Octave;
        int LastOctave;
        int ReservedOctave;
        int Volume;
        int Tone;
        int LastTone;

        bool SSGEnvOn;
        int SoundNo;
        int DefaultNoteLength;
        int Detune;
        int GateTime;

        // いい加減適当
        int PLength;

        int ILength;
        int IDepth;
        int IDelay;

        int ULength;
        int UDepth;
        int UDelay;

        std::optional<CommandType> LinkedItem;
        bool Playing;
        // 無限ループ検出
        bool InfiniteLooping;
    };

} // namespace MusicCom
