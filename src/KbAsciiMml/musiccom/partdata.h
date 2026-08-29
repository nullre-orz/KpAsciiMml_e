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
            CommandState()
                : CallStack(),
                  LoopStack(),
                  CommandPtr(),
                  LinkedItem(std::nullopt)
            {
            }

            std::stack<CommandIterator> CallStack;
            std::stack<std::pair<CommandIterator, int>> LoopStack;
            CommandIterator CommandPtr;
            std::optional<CommandType> LinkedItem;
        };

        struct NoteState
        {
            NoteState()
                : BeginFrame(0),
                  EndFrame(0),
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
                  DefaultLength(64),
                  Detune(0),
                  GateTime(8)
            {
            }

            int BeginFrame;
            int EndFrame;
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
            int DefaultLength;
            int Detune;
            int GateTime;
        };

        struct EffectState
        {
            EffectState()
                : PortamentoLength(0),
                  VibratoLength(0),
                  VibratoDepth(0),
                  VibratoDelay(0),
                  TremoloLength(0),
                  TremoloDepth(0),
                  TremoloDelay(0)
            {
            }

            int PortamentoLength;
            int VibratoLength;
            int VibratoDepth;
            int VibratoDelay;
            int TremoloLength;
            int TremoloDepth;
            int TremoloDelay;
        };

        struct PlaybackState
        {
            PlaybackState()
                : HasPreviousNote(false),
                  Playing(false),
                  InfiniteLooping(false)
            {
            }

            bool HasPreviousNote;
            bool Playing;
            // 無限ループ検出
            bool InfiniteLooping;
        };

        PartData()
            : Commands(),
              Note(),
              Effect(),
              Playback()
        {
        }

        CommandState Commands;
        NoteState Note;
        EffectState Effect;
        PlaybackState Playback;
    };

} // namespace MusicCom
