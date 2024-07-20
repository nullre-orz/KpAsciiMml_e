#pragma once

#include <cassert>
#include <list>
#include <string>

namespace MusicCom
{
    // clang-format off
    enum class CommandType : char
    {
        TYPE_UNKNOWN     =   -1, // unknown (no settings)
        TYPE_NOTE        =  '#', // note
        TYPE_REST        =  'R', // rest
        TYPE_WAIT        =  'W', // rest (no key off)
        TYPE_TIE         =  '&', // tie
        TYPE_LENGTH      =  'L', // length
        TYPE_OCTAVE      =  'O', // octave
        TYPE_OCTAVE_DOWN =  '<', // octave down
        TYPE_OCTAVE_UP   =  '>', // octave up
        TYPE_TEMPO       =  'T', // tempo
        TYPE_VOLUME      =  'V', // volume
        TYPE_TONE        =  '@', // tone
        TYPE_GATE_TIME   =  'Q', // gate time (duration)
        TYPE_DETUNE      =  'N', // detune
        TYPE_PORTAMENTO  =  'P', // portamento
        TYPE_TREMOLO     =  'U', // tremolo
        TYPE_VIBRATO     =  'I', // vibrato
        TYPE_ENV_FORM    =  'S', // envelope form
        TYPE_ENV_PERIOD  =  'M', // envelope period
        TYPE_DIRECT      =  'Y', // direct write
        TYPE_LOOP        =  '{', // loop start
        TYPE_EXIT        =  '}', // loop exit
        TYPE_MACRO       =  '$', // macro start
        TYPE_RETURN      =  '%', // macro return
        TYPE_PAUSE       =  '_'  // pause
    };
    // clang-format on

    class Command
    {
    public:
        Command()
        {
            type = CommandType::TYPE_UNKNOWN;
            std::memset(args, 0, sizeof(args));
        }
        Command(CommandType t, int arg0 = 0, int arg1 = 0, int arg2 = 0)
        {
            SetType(t);
            SetArg(0, arg0);
            SetArg(1, arg1);
            SetArg(2, arg2);
        }
        Command(CommandType t, std::string arg)
        {
            SetType(t);
            std::memset(args, 0, sizeof(args));
            SetStrArg(arg);
        }
        template<class InIt>
        Command(CommandType t, InIt first, InIt last)
        {
            SetType(t);
            std::memset(args, 0, sizeof(args));
            for (int i = 0; i < sizeof(args) / sizeof(args[0]) && first != last; i++, first++)
            {
                SetArg(i, *first);
            }
        }
        void SetType(CommandType t)
        {
            type = t;
        }
        CommandType GetType() const
        {
            return type;
        }
        void SetArg(int index, int val)
        {
            assert(0 <= index && index < 3);
            args[index] = val;
        }
        int GetArg(int index) const
        {
            assert(0 <= index && index < 3);
            return args[index];
        }

        // （; ＾ω＾）
        void SetStrArg(std::string val)
        {
            strarg = val;
        }
        const std::string& GetStrArg() const
        {
            return strarg;
        }

    private:
        CommandType type;
        int args[3];
        std::string strarg;
    };

    using CommandList = std::list<Command>;
    using CommandIterator = CommandList::const_iterator;
} // namespace MusicCom
