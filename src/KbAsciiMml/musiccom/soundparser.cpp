#include "soundparser.h"
#include "sounddata.h"
#include <algorithm>
#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/include/classic_file_iterator.hpp>
#include <filesystem>
#include <format>
#include <memory>
#include <numeric>
#include <stdexcept>

namespace x3 = boost::spirit::x3;

namespace MusicCom
{
    namespace
    {
        enum SoundArgument : int
        {
            SOUND_LENGTH_ARGUMENT,
            SOUND_TONE_MULTIPLIER_ARGUMENT,
            SOUND_VOLUME_MULTIPLIER_ARGUMENT,
            SOUND_ARGUMENT_COUNT,
        };

        enum ToneArgument : int
        {
            TONE_INITIAL_VALUE_ARGUMENT,
            TONE_INCREMENT_ARGUMENT,
            TONE_PERIOD_ARGUMENT,
            TONE_LOOP_ARGUMENT,
            TONE_ARGUMENT_COUNT,
        };

        enum VolumeArgument : int
        {
            VOLUME_INITIAL_VALUE_ARGUMENT,
            VOLUME_INCREMENT_ARGUMENT,
            VOLUME_PERIOD_ARGUMENT,
            VOLUME_LOOP_ARGUMENT,
            VOLUME_ARGUMENT_COUNT,
        };

        enum NoiseArgument : int
        {
            NOISE_CHANNEL_TYPE_ARGUMENT,
            NOISE_INITIAL_VALUE_ARGUMENT,
            NOISE_INCREMENT_ARGUMENT,
            NOISE_PERIOD_ARGUMENT,
            NOISE_LOOP_ARGUMENT,
            NOISE_ARGUMENT_COUNT,
        };

        constexpr int FIRST_LINE_NUMBER = 1;
        constexpr int NO_SOUND_NUMBER = -1;
        constexpr int FIRST_CHANNEL_INDEX = 0;
        constexpr char FIRST_CHANNEL_CHARACTER = '1';
        constexpr char LAST_CHANNEL_CHARACTER = '2';
        constexpr int DEFAULT_SOUND_LENGTH = 60;
        constexpr int DEFAULT_MULTIPLIER = 1;
        constexpr int DEFAULT_PERIOD = 1;
        constexpr int USE_DEFAULT_VALUE = 0;
        constexpr int LOOP_ENABLED_VALUE = 0;
        constexpr char DOS_EOF = 0x1a;
    } // namespace

    const std::string DEFAULT_SOUND_DAT = R"(
; comment
Sound:	@0, 8 ,1, 8
Tone1:	400,100,2,0
Vol1:	15,-7 , 1, 1
Vol2:	15,-7 , 1, 1
Noise:	3, 20 ,-10 ,4,0

Sound:	@1,4 ,2 ,4
Vol1:	13,-10, 1, 1
Vol2:	13,-10, 1, 1
Noise:	3, 0 , 10 ,1,0

Sound:	@2,10 ,2 ,10
Tone2:	100,-60 ,1,0
Vol1:	13,-5, 1, 1
Vol2:	11,-5, 1, 1
Noise:	3, 0 ,4 ,1,0

Sound:	@3,7 ,2 ,7
Tone1:	100,-15 ,1,0
Tone2:	100,-30 ,1,0
Vol1:	15,-15, 1, 1
Vol2:	15,-15, 1, 1

Sound:	@4, 10 ,1, 10
Tone1:	500,200 ,2,0
Tone1:	1500,550 ,2,0
Vol1:	13,-6 , 1, 1
Vol2:	15,-6 , 1, 1
Noise:	1, 30 ,0 ,4,0

Sound:	@5, 16 ,1, 16
Tone1:	400,100 ,2,0
Vol1:	15,-4 , 1, 1
Vol2:	15,-4 , 1, 1
Noise:	3, 20 ,-5 ,4,0
)";

    struct SoundParserState
    {
        SoundParserState(SoundData& container)
            : sound_data(container),
              editing_data(),
              line(FIRST_LINE_NUMBER),
              current_sound(NO_SOUND_NUMBER),
              channel(FIRST_CHANNEL_INDEX),
              tone_multiplier(DEFAULT_MULTIPLIER),
              volume_multiplier(DEFAULT_MULTIPLIER),
              args(),
              errors(),
              finished(false)
        {
        }

        std::vector<Block>& GetEditingBlocks() const
        {
            return editing_data->blocks_;
        }

        SoundData& sound_data;
        std::unique_ptr<RhythmData> editing_data;

        int line;

        int current_sound;
        int channel;
        int tone_multiplier;
        int volume_multiplier;
        std::vector<int> args;
        std::vector<std::string> errors;

        bool finished;
    };

    static void update_rhythm_data(SoundParserState& state)
    {
        // 編集中のリズムデータを反映
        if (state.editing_data)
        {
            state.sound_data.SetRhythm(state.current_sound, *state.editing_data);
        }
    }

    namespace grammer
    {
        namespace detail
        {
            static decltype(auto) increment_line()
            {
                return [](auto& ctx)
                {
                    auto& state = x3::get<SoundParserState>(ctx);
                    ++state.line;
                };
            };

            static decltype(auto) begin_operator()
            {
                return [](auto& ctx)
                {
                    // 引数をリセット
                    auto& state = x3::get<SoundParserState>(ctx);
                    state.args.clear();
                };
            };

            static decltype(auto) set_current_sound()
            {
                return [](auto& ctx)
                {
                    auto number = x3::_attr(ctx);
                    SoundParserState& state = x3::get<SoundParserState>(ctx);

                    // 編集中のリズムデータを反映し初期化
                    if (state.editing_data)
                    {
                        update_rhythm_data(state);
                    }
                    state.editing_data = std::make_unique<RhythmData>();
                    state.current_sound = number;
                };
            };

            static decltype(auto) set_current_channel()
            {
                return [](auto& ctx)
                {
                    auto channel = x3::_attr(ctx);
                    SoundParserState& state = x3::get<SoundParserState>(ctx);
                    state.channel = channel - FIRST_CHANNEL_CHARACTER;
                };
            };

            static decltype(auto) append_arg()
            {
                return [](auto& ctx)
                {
                    auto arg = x3::_attr(ctx);
                    SoundParserState& state = x3::get<SoundParserState>(ctx);
                    state.args.push_back(arg);
                };
            };

            static decltype(auto) set_sound()
            {
                return [](auto& ctx)
                {
                    SoundParserState& state = x3::get<SoundParserState>(ctx);
                    if (!state.editing_data)
                    {
                        return;
                    }

                    state.args.resize(SOUND_ARGUMENT_COUNT);
                    int len = state.args[SOUND_LENGTH_ARGUMENT];
                    // 音長0は60として扱う
                    len = len == USE_DEFAULT_VALUE ? DEFAULT_SOUND_LENGTH : len;

                    // ブロックを追加
                    Block block({len});
                    state.GetEditingBlocks().push_back(block);

                    // 周期の倍率0は1として扱う
                    state.tone_multiplier = state.args[SOUND_TONE_MULTIPLIER_ARGUMENT] == USE_DEFAULT_VALUE ? DEFAULT_MULTIPLIER : state.args[SOUND_TONE_MULTIPLIER_ARGUMENT];
                    state.volume_multiplier = state.args[SOUND_VOLUME_MULTIPLIER_ARGUMENT] == USE_DEFAULT_VALUE ? DEFAULT_MULTIPLIER : state.args[SOUND_VOLUME_MULTIPLIER_ARGUMENT];
                };
            };

            static decltype(auto) set_tone()
            {
                return [](auto& ctx)
                {
                    SoundParserState& state = x3::get<SoundParserState>(ctx);
                    if (!state.editing_data)
                    {
                        return;
                    }

                    state.args.resize(TONE_ARGUMENT_COUNT);
                    auto& editing_blocks = state.GetEditingBlocks();
                    if (!editing_blocks.empty())
                    {
                        auto& target = editing_blocks.back();
                        auto& tone = target.tone[state.channel];
                        tone.enabled = true;
                        tone.initial_value = state.args[TONE_INITIAL_VALUE_ARGUMENT];
                        tone.final_value = state.args[TONE_INITIAL_VALUE_ARGUMENT] + state.args[TONE_INCREMENT_ARGUMENT];
                        tone.period = (state.args[TONE_PERIOD_ARGUMENT] == USE_DEFAULT_VALUE ? DEFAULT_PERIOD : state.args[TONE_PERIOD_ARGUMENT]) * state.tone_multiplier;
                        tone.loop = (state.args[TONE_LOOP_ARGUMENT] == LOOP_ENABLED_VALUE);
                    }
                };
            };

            static decltype(auto) set_volume()
            {
                return [](auto& ctx)
                {
                    SoundParserState& state = x3::get<SoundParserState>(ctx);
                    if (!state.editing_data)
                    {
                        return;
                    }

                    state.args.resize(VOLUME_ARGUMENT_COUNT);
                    auto& editing_blocks = state.GetEditingBlocks();
                    if (!editing_blocks.empty())
                    {
                        auto& target = editing_blocks.back();
                        auto& volume = target.volume[state.channel];
                        volume.initial_value = state.args[VOLUME_INITIAL_VALUE_ARGUMENT];
                        volume.final_value = state.args[VOLUME_INITIAL_VALUE_ARGUMENT] + state.args[VOLUME_INCREMENT_ARGUMENT];
                        volume.period = (state.args[VOLUME_PERIOD_ARGUMENT] == USE_DEFAULT_VALUE ? DEFAULT_PERIOD : state.args[VOLUME_PERIOD_ARGUMENT]) * state.volume_multiplier;
                        volume.loop = (state.args[VOLUME_LOOP_ARGUMENT] == LOOP_ENABLED_VALUE);
                    }
                };
            };

            static decltype(auto) set_noise()
            {
                return [](auto& ctx)
                {
                    SoundParserState& state = x3::get<SoundParserState>(ctx);
                    if (!state.editing_data)
                    {
                        return;
                    }

                    state.args.resize(NOISE_ARGUMENT_COUNT);
                    auto& editing_blocks = state.GetEditingBlocks();
                    if (!editing_blocks.empty())
                    {
                        auto& target = editing_blocks.back();
                        auto& noise = target.noise;
                        noise.channel_type = state.args[NOISE_CHANNEL_TYPE_ARGUMENT];
                        noise.initial_value = state.args[NOISE_INITIAL_VALUE_ARGUMENT];
                        noise.final_value = state.args[NOISE_INITIAL_VALUE_ARGUMENT] + state.args[NOISE_INCREMENT_ARGUMENT];
                        noise.period = (state.args[NOISE_PERIOD_ARGUMENT] == USE_DEFAULT_VALUE ? DEFAULT_PERIOD : state.args[NOISE_PERIOD_ARGUMENT]) * state.tone_multiplier;
                        noise.loop = (state.args[NOISE_LOOP_ARGUMENT] == LOOP_ENABLED_VALUE);
                    }
                };
            };
            static decltype(auto) finish()
            {
                return [](auto& ctx)
                {
                    SoundParserState& state = x3::get<SoundParserState>(ctx);

                    // 編集中のリズムデータを反映
                    if (state.editing_data)
                    {
                        update_rhythm_data(state);
                    }

                    // 完了処理
                    state.finished = true;
                };
            };
        } // namespace detail

        const x3::rule<class line, std::string> line = "line";
        const x3::rule<class sound_line, std::string> sound_line = "sound";
        const x3::rule<class tone_line, std::string> tone_line = "tone";
        const x3::rule<class volume_line, std::string> volume_line = "volume";
        const x3::rule<class noise_line, std::string> noise_line = "noise";
        const x3::rule<class comment_line, std::string> comment_line = "comment";
        const x3::rule<class blank_line, std::string> blank_line = "blank";

        // カンマは単なる区切りとして扱い、省略された末尾引数は処理側で0補完する
        const auto arguments = *x3::lit(',') >> *((x3::int_)[detail::append_arg()] >> *x3::lit(','));
        const auto trailing_text = x3::lexeme[*(x3::char_ - x3::eol)];

        const auto line_def =
            (sound_line[detail::set_sound()] | tone_line[detail::set_tone()] | volume_line[detail::set_volume()] | noise_line[detail::set_noise()] | comment_line | blank_line)
            >> (x3::eoi[detail::finish()] | x3::eol[detail::increment_line()] | x3::char_(DOS_EOF));

        const auto sound_line_def =
            (x3::no_case["sound:"])[detail::begin_operator()]
            >> -(x3::lit('@') >> (x3::uint_)[detail::set_current_sound()])
            >> arguments >> trailing_text;

        const auto tone_line_def =
            (x3::no_case["tone"] >> x3::char_(FIRST_CHANNEL_CHARACTER, LAST_CHANNEL_CHARACTER)[detail::set_current_channel()] >> x3::lit(':'))[detail::begin_operator()]
            >> arguments >> trailing_text;

        const auto volume_line_def =
            (x3::no_case["vol"] >> x3::char_(FIRST_CHANNEL_CHARACTER, LAST_CHANNEL_CHARACTER)[detail::set_current_channel()] >> x3::lit(':'))[detail::begin_operator()]
            >> arguments >> trailing_text;

        const auto noise_line_def =
            (x3::no_case["noise:"])[detail::begin_operator()]
            >> arguments >> trailing_text;

        const auto comment_line_def =
            x3::lit(';')
            >> x3::lexeme[*(x3::char_ - x3::eol)];

        const auto blank_line_def = x3::eps;

        BOOST_SPIRIT_DEFINE(
            line,
            sound_line,
            tone_line,
            volume_line,
            noise_line,
            comment_line,
            blank_line);
    } // namespace grammer

    template<typename Iterator>
    SoundData* ParseSoundImpl(const Iterator begin, const Iterator end, const std::string& source_name)
    {
        auto data = std::make_unique<SoundData>();
        SoundParserState state(*data);
        const auto parser = x3::with<SoundParserState>(state)[grammer::line];

        auto current = Iterator(begin);
        while (!state.finished)
        {
            const auto line_begin = current;
            const bool success = x3::phrase_parse(current, end, parser, x3::blank);
            if (!success)
            {
                const auto line_end = std::find_if(line_begin, end, [](char c)
                                                   { return c == '\r' || c == '\n'; });
                state.errors.push_back(std::format("({:d}): parse error at \"{}\"", state.line, std::string(line_begin, line_end)));
                current = line_end;
            }
        }

        if (!state.errors.empty())
        {
            const auto message = std::accumulate(
                state.errors.begin(),
                state.errors.end(),
                source_name,
                [](const std::string& accumulated, const std::string& error)
                {
                    return std::format("{}\n{}", accumulated, error);
                });
            throw std::runtime_error(message);
        }

        return data.release();
    }

    static SoundData* ParseDefaultSound()
    {
        auto begin(std::cbegin(DEFAULT_SOUND_DAT));
        const auto end(std::cend(DEFAULT_SOUND_DAT));

        return ParseSoundImpl(begin, end, "embedded SOUND.DAT");
    }

    SoundData* ParseSound(const std::string& mml_filename)
    {
        using path = std::filesystem::path;
        using file_iterator = boost::spirit::classic::file_iterator<char>;

        try
        {
            // MMLファイルと同ディレクトリにあるSOUND.DATを読み込む
            path mml_path(mml_filename);
            auto dir_path = mml_path.parent_path();
            auto sound_dat_path = dir_path / "SOUND.DAT";
            file_iterator begin(sound_dat_path.string());
            if (!begin)
            {
                // 読み込めない場合は組み込みのサンプルデータ(Dante98ベース)を読み込む
                return ParseDefaultSound();
            }
            file_iterator end = begin.make_end();
            return ParseSoundImpl(begin, end, sound_dat_path.string());
        }
        catch (...)
        {
            // 失敗した場合は組み込みのサンプルデータ(Dante98ベース)を読み込む
            return ParseDefaultSound();
        }
    }
} // namespace MusicCom
