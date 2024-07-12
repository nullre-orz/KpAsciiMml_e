#include "mmlparser.h"
#include "musdata.h"

#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_file_iterator.hpp>
#include <boost/spirit/include/classic_utility.hpp>
#include <format>
#include <memory>
#include <numeric>
#include <string>
#include <vector>

using namespace std;
using namespace boost::spirit::classic;
using boost::bad_lexical_cast;
using boost::lexical_cast;

namespace MusicCom
{
    int ParseLength(string s)
    {
        if (s.empty())
            return 0;
        bool dotted;
        if (*s.rbegin() == '.')
        {
            s.erase(--s.end());
            dotted = true;
        }
        else
        {
            dotted = false;
        }

        int len = lexical_cast<int>(s);
        if (len <= 0 || 64 < len)
        {
            //throw runtime_error("illegal note length");
            return 64;
        }
        len = 64 / len;
        if (dotted)
            len += len / 2;

        return len;
    }

    int StringToInt(string s)
    {
        try
        {
            return lexical_cast<int>(s);
        }
        catch (bad_lexical_cast)
        {
            return 0;
        }
    }

    // LFO/OP用
    vector<int> ParseSoundArgs(vector<string>& args, int num)
    {
        vector<int> container(args.size());
        try
        {
            transform(args.begin(), args.end(), container.begin(), [](std::string& s)
                      { return lexical_cast<int>(s); });
        }
        catch (bad_lexical_cast)
        {
            // 数値以外が含まれていた場合は処理を中断
            // 以降の値はすべて 0 として扱う
        }
        container.resize(num, 0);

        return container;
    }

    struct MMLParser : public grammar<MMLParser>
    {
        enum MMLLineType
        {
            UNDEFINED,
            CH,
            RHYTHM,
            SOUND,
            LFO,
            OP,
            SSGENV,
            STR
        };

        struct MMLParserState
        {
            MMLParserState()
                : pMusicData(nullptr),
                  LineNumber(1),
                  LineType(UNDEFINED),
                  ChNumber(),
                  CommandType(),
                  SoundNumber(),
                  Finished(false)
            {
            }

            vector<string> args;

            MusicData* pMusicData;

            // Line
            int LineNumber;
            MMLLineType LineType;

            int ChNumber; // ch番号 (0-origin)

            CommandType CommandType;
            string MacroName;

            // Sound
            FMSound Sound;
            int SoundNumber;

            bool Finished;
        };

        MMLParser(MMLParserState& s) : state(s)
        {
        }
        MMLParserState& state;

    private:
        static void AddCommand(MMLParserState& state, const Command& command)
        {
            switch (state.LineType)
            {
            case CH:
                state.pMusicData->AddCommandToChannel(state.ChNumber, command);
                break;
            case RHYTHM:
                state.pMusicData->AddCommandToRhythmPart(command);
                break;
            case STR:
                state.pMusicData->AddCommandToMacro(state.MacroName, command);
                break;
            default:
                assert(0);
                break;
            }
        }

        struct ChangeLine
        {
            ChangeLine(MMLParserState& s) : state(s) {}

            template<typename IteratorT>
            void operator()(IteratorT first, IteratorT last) const
            {
                // パート解析の場合は一時停止コマンドを追加
                if (state.LineType == CH || state.LineType == RHYTHM)
                {
                    AddCommand(state, CommandType::TYPE_PAUSE);
                }

                state.LineNumber++;
                state.LineType = UNDEFINED;
            }

            MMLParserState& state;
        };

        struct Finish
        {
            Finish(MMLParserState& s) : state(s) {}

            template<typename IteratorT>
            void operator()(IteratorT first, IteratorT last) const
            {
                // パート解析の場合は一時停止コマンドを追加
                if (state.LineType == CH || state.LineType == RHYTHM)
                {
                    AddCommand(state, CommandType::TYPE_PAUSE);
                }

                state.Finished = true;
            }

            MMLParserState& state;
        };

        template<enum MMLLineType t>
        struct BeginLine
        {
            BeginLine(MMLParserState& s) : state(s) {}

            template<typename IteratorT>
            void operator()(IteratorT first, IteratorT last) const
            {
                state.args.clear();
                state.LineType = t;
            }

            MMLParserState& state;
        };

        struct SetChNumber
        {
            SetChNumber(MMLParserState& s) : state(s) {}

            void operator()(char c) const
            {
                state.ChNumber = c - '1';
            }

            MMLParserState& state;
        };

        struct SetMacroName
        {
            SetMacroName(MMLParserState& s) : state(s) {}

            template<typename IteratorT>
            void operator()(IteratorT first, IteratorT last) const
            {
                state.MacroName.assign(first, last);
            }

            MMLParserState& state;
        };

        struct PushArg
        {
            PushArg(MMLParserState& s) : state(s) {}

            template<typename IteratorT>
            void operator()(IteratorT first, IteratorT last) const
            {
                string s(first, last);
                state.args.push_back(s);
            }

            MMLParserState& state;
        };

        // Sound
        struct ProcessLFO
        {
            ProcessLFO(MMLParserState& s) : state(s) {}

            template<typename IteratorT>
            void operator()(IteratorT first, IteratorT last) const
            {
                auto a = ParseSoundArgs(state.args, 5);

                // LFO:	WF,SPEED,DEPTH,ALG,FB
                FMSound& sound = state.Sound;
                sound.SetLFO(a[0], a[1], a[2]);
                sound.SetAlgFb(a[3], a[4]);
                state.pMusicData->SetFMSound(state.SoundNumber, sound);
            }

            MMLParserState& state;
        };

        struct ProcessOP
        {
            ProcessOP(MMLParserState& s) : state(s) {}

            template<typename IteratorT>
            void operator()(IteratorT first, IteratorT last) const
            {
                int op = state.ChNumber;
                auto a = ParseSoundArgs(state.args, 10);

                // OP1:	AR,DR,SR,RR,SL,TL,KS,ML,DT,DT2
                FMSound& sound = state.Sound;
                sound.SetDtMl(op, a[8], a[7]);
                sound.SetTl(op, a[5]);
                sound.SetKsAr(op, a[6], a[0]);
                sound.SetDr(op, a[1]);
                //			sound.SetSr(op, a[2]);
                sound.SetSr(op, a[4]); // music.comでSrとしてSlが使われるバグ！？
                sound.SetSlRr(op, a[4], a[3]);
                sound.SetDt2(op, a[9]);
                state.pMusicData->SetFMSound(state.SoundNumber, state.Sound);
            }

            MMLParserState& state;
        };

        // SSGEnv
        struct ProcessSSGEnv
        {
            ProcessSSGEnv(MMLParserState& s) : state(s) {}

            template<typename IteratorT>
            void operator()(IteratorT first, IteratorT last) const
            {
                SSGEnv env;

                if (state.args.size() < 3)
                {
                    //throw std::runtime_error("SSGEnvの引数が足りない");
                    return;
                }

                int no = StringToInt(state.args[0]);
                env.Unit = StringToInt(state.args[1]);
                env.Env.clear();
                transform(state.args.begin() + 2, state.args.end(), back_inserter(env.Env), StringToInt);
                state.pMusicData->SetSSGEnv(no, env);
            }

            MMLParserState& state;
        };

        // MML コマンド
        struct BeginCommand
        {
            BeginCommand(MMLParserState& s) : state(s) {}

            void operator()(char c) const
            {
                state.args.clear();
                state.CommandType = static_cast<CommandType>(toupper(c));
            }

            MMLParserState& state;
        };

        struct ProcessNote
        {
            ProcessNote(MMLParserState& s) : state(s) {}

            template<typename IteratorT>
            void operator()(IteratorT first, IteratorT last) const
            {
                static const int note_numbers[] = {
                    // A,  B, C, D, E, F, G
                    9,
                    11,
                    0,
                    2,
                    4,
                    5,
                    7,
                };

                vector<string>& a = state.args;

                int note = note_numbers[static_cast<char>(state.CommandType) - 'A'];
                if (!a[0].empty())
                {
                    switch (a[0][0])
                    {
                    case '+':
                        note++;
                        break;
                    case '-':
                        note--;
                        break;
                    }
                }
                int len = ParseLength(state.args[1]);

                AddCommand(state, Command(CommandType::TYPE_NOTE, note, len));
            }

            MMLParserState& state;
        };

        struct ProcessCtrl
        {
            ProcessCtrl(MMLParserState& s) : state(s) {}

            template<typename IteratorT>
            void operator()(IteratorT first, IteratorT last) const
            {
                const static struct CtrlDef
                {
                    CommandType Type;
                    bool IsNoteLength;
                    int MinArgs;
                    int MaxArgs;
                    int Defaults[3];
                } ctrl_defs[] = {
                    {CommandType::TYPE_TIE, false, 0, 0, {}},
                    {CommandType::TYPE_TEMPO, false, 1, 1, {}},
                    {CommandType::TYPE_REST, true, 0, 1, {0}},
                    {CommandType::TYPE_WAIT, true, 0, 1, {0}},
                    {CommandType::TYPE_LENGTH, true, 1, 1, {}},
                    {CommandType::TYPE_OCTAVE, false, 1, 1, {}},
                    {CommandType::TYPE_OCTAVE_DOWN, false, 0, 0, {}},
                    {CommandType::TYPE_OCTAVE_UP, false, 0, 0, {}},
                    {CommandType::TYPE_VOLUME, false, 1, 1, {}},
                    {CommandType::TYPE_TONE, false, 1, 1, {}},
                    {CommandType::TYPE_GATE_TIME, false, 1, 1, {}},
                    {CommandType::TYPE_DETUNE, false, 1, 1, {}},
                    {CommandType::TYPE_PORTAMENTO, false, 1, 1, {}},
                    {CommandType::TYPE_TREMOLO, false, 1, 3, {0, 0, 0}},
                    {CommandType::TYPE_VIBRATO, false, 1, 3, {0, 0, 0}},
                    {CommandType::TYPE_ENV_FORM, false, 1, 1, {}},
                    {CommandType::TYPE_ENV_PERIOD, false, 1, 1, {}},
                    {CommandType::TYPE_DIRECT, false, 2, 2, {}},
                    {CommandType::TYPE_LOOP, false, 0, 1, {0}},
                    {CommandType::TYPE_EXIT, false, 0, 0, {}},
                };

                const CtrlDef* pctrldef = NULL;
                for (int i = 0; i < sizeof(ctrl_defs) / sizeof(ctrl_defs[0]); i++)
                {
                    if (ctrl_defs[i].Type == state.CommandType)
                    {
                        pctrldef = &ctrl_defs[i];
                        break;
                    }
                }

                if (pctrldef == NULL)
                {
                    //ostringstream ss;
                    //ss << "unknown command '" << state.CommandType << "'";
                    //throw runtime_error(ss.str());
                    return;
                }

                int args_supplied = (int)state.args.size();
                if (args_supplied < pctrldef->MinArgs || pctrldef->MaxArgs < args_supplied)
                {
                    //ostringstream ss;
                    //ss << "illegal number of argument (" << args_supplied << ") for command '" << state.CommandType << "'";
                    //throw runtime_error(ss.str());
                    return;
                }

                // 引数を取得
                vector<int> a;
                // とりあえずintに変換
                transform(state.args.begin(), state.args.end(), back_inserter(a), StringToInt);
                // 引数が足りなければデフォルト引数を補充
                for (int i = args_supplied; i < pctrldef->MaxArgs; i++)
                {
                    a.push_back(pctrldef->Defaults[i]);
                }
                // 音長指定なら音長で変換し直す
                if (pctrldef->IsNoteLength && state.args.size() > 0)
                {
                    a[0] = ParseLength(state.args[0]);
                }

                // Tのみ特別処理
                if (state.CommandType == CommandType::TYPE_TEMPO)
                {
                    state.pMusicData->SetTempo(a[0]);
                }
                else
                {
                    AddCommand(state, Command(state.CommandType, a.begin(), a.end()));
                }
                //cout << state.CommandType << "(";
                //copy(state.args.begin(), state.args.end(), ostream_iterator<string>(cout, ","));
                //cout << ");";
            }

            MMLParserState& state;
        };

        struct ProcessCall
        {
            ProcessCall(MMLParserState& s) : state(s) {}

            template<typename IteratorT>
            void operator()(IteratorT first, IteratorT last) const
            {
                AddCommand(state, Command(CommandType::TYPE_MACRO, state.args[0]));
            }

            MMLParserState& state;
        };

    public:
        template<typename ScannerT>
        struct definition
        {
            definition(MMLParser const& self)
            {
                MMLParserState& s = self.state;

                // 0x1a = [EOF]
                line =
                    (ch_line | drum_line | sound_line | lfo_line | op_line | ssgenv_line | str_line | arrow_line | blank_line)
                    >> !comment
                    >> (eol_p[ChangeLine(s)] | end_p[Finish(s)] | ch_p(0x1a));

                args =
                    arg % *ch_p(',') // !: スペースで区切るMML対策
                    >> *ch_p(',');
                arg =
                    (int_p || ch_p('.'))[PushArg(s)];
                // LFO/OP用
                sound_args =
                    sound_arg % *ch_p(',') // !: スペースで区切るMML対策
                    >> *ch_p(',');
                sound_arg =
                    ((sound_invalid_arg | int_p) >> eps_p)[PushArg(s)]
                    >> *sound_invalid_arg[PushArg(s)];
                sound_invalid_arg =
                    lexeme_d[+(~digit_p - sign_p - blank_p - cntrl_p - ch_p(','))];

                macro_name =
                    lexeme_d[+(~chset<>("$,=") - blank_p - cntrl_p)];
                mml_Command =
                    mml_note[ProcessNote(s)] | mml_ctrl[ProcessCtrl(s)] | mml_call[ProcessCall(s)];
                mml_note =
                    as_lower_d[range_p('a', 'g')][BeginCommand(s)]
                    >> (ch_p('+') | ch_p('-') | eps_p)[PushArg(s)]
                    >> *ch_p(',') // 引数の前にカンマを置くMML対策
                    >> (arg | eps_p[PushArg(s)])
                    >> *ch_p(','); // 引数の後にカンマを置くMML対策
                // '&' はコマンドとして扱う
                //				>> (ch_p('&') | eps_p)[PushArg(s)];
                mml_ctrl =
                    as_lower_d[chset<>("h-z@{}<>&")][BeginCommand(s)]
                    >> *ch_p(',') // 第1引数の前にカンマを置くMML対策
                    >> !args;
                //				>> !ch_p('&');	// 変なところに&を置くMML対策
                mml_call =
                    ch_p('$')[BeginCommand(s)]
                    >> macro_name[PushArg(s)]
                    >> ch_p('$')
                    >> !ch_p(','); // なぜかマクロ呼び出しを','で区切るMML対策

                // 空行
                blank_line = eps_p;

                // コメント
                comment =
                    ch_p(';')
                    >> *(anychar_p - eol_p);

                // チャンネル定義 1:, ...
                ch_line =
                    (range_p('1', '6')[SetChNumber(s)] >> ch_p(':'))[BeginLine<CH>(s)]
                    >> *mml_Command;

                // D: パート (SOUND.DATのリズム音)
                drum_line =
                    (as_lower_d[str_p("d:")])[BeginLine<RHYTHM>(s)]
                    >> *mml_Command;

                sound_line =
                    (as_lower_d[str_p("sound")] >> ch_p(':'))[BeginLine<SOUND>(s)]
                    >> !ch_p('@')
                    >> int_p[assign(s.SoundNumber)];
                lfo_line =
                    (as_lower_d[str_p("lfo")] >> ch_p(':'))[BeginLine<LFO>(s)]
                    >> sound_args[ProcessLFO(s)];
                op_line =
                    (as_lower_d[str_p("op")]
                     >> range_p('1', '4')[SetChNumber(s)] >> ch_p(':'))[BeginLine<OP>(s)]
                    >> sound_args[ProcessOP(s)];
                ssgenv_line =
                    (as_lower_d[str_p("ssgenv")] >> ch_p(':'))[BeginLine<SSGENV>(s)]
                    >> !ch_p('@')
                    >> ((int_p | eps_p)[PushArg(s)] % (((eol_p[ChangeLine(s)] % !(comment | blank_line)) >> str_p("->") | ch_p(','))))[ProcessSSGEnv(s)];
                str_line =
                    (as_lower_d[str_p("str")] >> ch_p(':'))[BeginLine<STR>(s)]
                    >> macro_name[SetMacroName(s)]
                    >> !ch_p('$') >> ch_p('=')
                    >> *mml_Command;
                // SSGENV外の -> は無視する
                arrow_line =
                    str_p("->")
                    >> *(anychar_p - eol_p);
            }

            rule<ScannerT> line;
            rule<ScannerT> blank_line, ch_line, drum_line, sound_line, lfo_line, op_line, ssgenv_line, str_line, arrow_line;
            rule<ScannerT> mml_Command, mml_note, mml_ctrl, mml_call;
            rule<ScannerT> args, arg, sound_args, sound_arg, sound_invalid_arg, macro_name, comment;

            rule<ScannerT> const&
            start() const { return line; }
        };
    };

    MusicData* ParseMML(const char* filename)
    {
        auto pMusicData = std::make_unique<MusicData>();
        MMLParser::MMLParserState state;
        state.pMusicData = pMusicData.get();
        MMLParser mmlparser(state);

        typedef file_iterator<char> iterator_t;

        // ファイルを開いて、その先頭を指すイテレータを生成
        iterator_t begin(filename);
        if (!begin)
        {
            return nullptr;
        }

        iterator_t first = begin;
        iterator_t last = first.make_end();

        vector<string> error_list = {};

        while (!state.Finished)
        {
            parse_info<iterator_t> info;
            try
            {
                info = parse(first, last, mmlparser, blank_p);
                if (!info.hit)
                {
                    iterator_t i = find_if(
                        info.stop,
                        last,
                        [](char c)
                        {
                            return c == '\r' || c == '\n';
                        });
                    error_list.push_back(format("({:d}): parse error at \"{}\"", state.LineNumber, string(info.stop, i)));
                    first = i;
                }
                else
                {
                    first = info.stop;
                }
            }
            catch (exception& e)
            {
                error_list.push_back(format("({:d}): {}", state.LineNumber, e.what()));
                break;
            }
        }

        // エラーリストが空でない場合は例外をthrow
        if (error_list.size() > 0)
        {
            // 改行区切りで連結
            auto msg = accumulate(
                error_list.begin(),
                error_list.end(),
                string(filename),
                [](const string& acc, const string& str)
                {
                    return format("{}\n{}", acc, str);
                });

            throw std::runtime_error(msg);
            return nullptr;
        }

        return pMusicData.release();
    }

} // namespace MusicCom
