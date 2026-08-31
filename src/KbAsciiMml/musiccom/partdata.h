#pragma once

#include "command.h"
#include <optional>
#include <stack>
#include <utility>

namespace MusicCom
{
    struct PartData
    {
        struct CommandState
        {
            CommandState() = default;

            std::stack<CommandIterator> CallStack{};
            std::stack<std::pair<CommandIterator, int>> LoopStack{};
            CommandIterator CommandPtr{};
            std::optional<CommandType> LinkedItem{};
        };

        struct NoteState
        {
            NoteState() = default;

            int BeginFrame = 0;
            int EndFrame = 0;
            int KeyOnFrame = 0;
            int KeyOffFrame = 0;
            int Octave = 4;
            int LastOctave = 4;
            int ReservedOctave = 4;
            int Volume = 15;
            int Tone = 0;
            int LastTone = 0;
            bool SSGEnvOn = false;
            int SoundNo = 0;
            int DefaultLength = 64;
            int Detune = 0;
            int GateTime = 8;
        };

        struct EffectState
        {
            EffectState() = default;

            int PortamentoLength = 0;
            int VibratoLength = 0;
            int VibratoDepth = 0;
            int VibratoDelay = 0;
            int TremoloLength = 0;
            int TremoloDepth = 0;
            int TremoloDelay = 0;
        };

        struct PlaybackState
        {
            PlaybackState() = default;

            bool HasPreviousNote = false;
            bool Playing = false;
            // 無限ループ検出
            bool InfiniteLooping = false;
        };

        PartData() = default;

        CommandState Commands;
        NoteState Note;
        EffectState Effect;
        PlaybackState Playback;
    };

} // namespace MusicCom
