#include "soundparser.h"
#include "sounddata.h"
#include <boost/spirit/include/classic_file_iterator.hpp>
#include <boost/spirit/home/x3.hpp>
#include <filesystem>
#include <iostream>
#include <memory>

namespace x3 = boost::spirit::x3;

namespace MusicCom
{
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
        SoundParserState(SoundData& container) : sound_data(container),
                                                 editing_data(),
                                                 line(1),
                                                 current_sound(-1),
                                                 channel(0),
                                                 args(),
                                                 finished(false)
        {
        }

        SoundData& sound_data;
        std::unique_ptr<RhythmData> editing_data;

        int line;

        int current_sound;
        int channel;
        std::vector<int> args;

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
                    state.channel = channel - '1';
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
                    if (state.args.size() < 1 || !state.editing_data)
                    {
                        return;
                    }

                    // ブロックを追加
                    Block block({state.args[0]});
                    state.editing_data->blocks.push_back(block);
                };
            };

            static decltype(auto) set_tone()
            {
                return [](auto& ctx)
                {
                    SoundParserState& state = x3::get<SoundParserState>(ctx);
                    if (state.args.size() < 4 || !state.editing_data)
                    {
                        return;
                    }

                    auto& target = state.editing_data->blocks.back();
                    auto& tone = target.tone[state.channel];
                    tone.initial_value = state.args[0];
                    tone.final_value = state.args[0] + state.args[1];
                    tone.period = state.args[2];
                    tone.loop = (state.args[3] == 0);
                };
            };

            static decltype(auto) set_volume()
            {
                return [](auto& ctx)
                {
                    SoundParserState& state = x3::get<SoundParserState>(ctx);
                    if (state.args.size() < 4 || !state.editing_data)
                    {
                        return;
                    }

                    auto& target = state.editing_data->blocks.back();
                    auto& volume = target.volume[state.channel];
                    volume.initial_value = state.args[0];
                    volume.final_value = state.args[0] + state.args[1];
                    volume.period = state.args[2];
                    volume.loop = (state.args[3] == 0);
                };
            };

            static decltype(auto) set_noise()
            {
                return [](auto& ctx)
                {
                    SoundParserState& state = x3::get<SoundParserState>(ctx);
                    if (state.args.size() < 5 || !state.editing_data)
                    {
                        return;
                    }

                    auto& target = state.editing_data->blocks.back();
                    auto& noise = target.noise;
                    noise.channel_type = state.args[0];
                    noise.initial_value = state.args[1];
                    noise.final_value = state.args[1] + state.args[2];
                    noise.period = state.args[3];
                    noise.loop = (state.args[4] == 0);
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

        const auto line_def =
            (sound_line[detail::set_sound()] | tone_line[detail::set_tone()] | volume_line[detail::set_volume()] | noise_line[detail::set_noise()] | comment_line | blank_line)
            >> (x3::eoi[detail::finish()] | x3::eol[detail::increment_line()] | x3::char_(0x1a));

        const auto sound_line_def =
            (x3::no_case["sound:"])[detail::begin_operator()]
            >> -(x3::lit('@') >> (x3::uint_)[detail::set_current_sound()] >> ',')
            >> (x3::int_)[detail::append_arg()] % ',';

        const auto tone_line_def =
            (x3::no_case["tone"] >> x3::char_('1', '2')[detail::set_current_channel()] >> x3::lit(':'))[detail::begin_operator()]
            >> (x3::int_)[detail::append_arg()] % ',';

        const auto volume_line_def =
            (x3::no_case["vol"] >> x3::char_('1', '2')[detail::set_current_channel()] >> x3::lit(':'))[detail::begin_operator()]
            >> (x3::int_)[detail::append_arg()] % ',';

        const auto noise_line_def =
            (x3::no_case["noise:"])[detail::begin_operator()]
            >> (x3::int_)[detail::append_arg()] % ',';

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
            blank_line
        );
    } // namespace grammer

    template<typename Iterator>
    SoundData* ParseSoundImpl(const Iterator begin, const Iterator end)
    {
        auto data = std::make_unique<SoundData>();
        SoundParserState state(*data);
        const auto parser = x3::with<SoundParserState>(state)[grammer::line];

        bool success = false;
        auto current = Iterator(begin);
        while (!state.finished)
        {
            success = x3::phrase_parse(current, end, parser, x3::blank);
            if (!success)
            {
                break;
            }
        }

        return data.release();
    }

    static SoundData* ParseDefaultSound()
    {
        auto begin(std::cbegin(DEFAULT_SOUND_DAT));
        const auto end(std::cend(DEFAULT_SOUND_DAT));

        return ParseSoundImpl(begin, end);
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
            return ParseSoundImpl(begin, end);
        }
        catch (...)
        {
            // 失敗した場合は組み込みのサンプルデータ(Dante98ベース)を読み込む
            return ParseDefaultSound();
        }
    }
} // namespace MusicCom
