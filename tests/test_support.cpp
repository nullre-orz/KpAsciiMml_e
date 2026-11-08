#define NOMINMAX
#include "test_support.h"
#include "test_report.h"
#include "../src/KbAsciiMml/musiccom/mmlparser.h"
#include <boost/test/unit_test.hpp>
#include <windows.h>
#include <fstream>
#include <stdexcept>

using namespace MusicCom;

namespace
{
    struct TestFailure : std::runtime_error
    {
        TestFailure(const std::string& message, const std::source_location& location)
            : std::runtime_error(message), File(location.file_name()), Line(location.line())
        {
        }

        std::string File;
        unsigned int Line;
    };

}

TestRunner::TestRunner(std::string suite_name) : suite_name_(std::move(suite_name))
{
}

void TestRunner::Run(const std::string& name, const std::function<void()>& body) const
{
    TestResult result{suite_name_, name, "OK"};
    try
    {
        body();
    }
    catch (const TestFailure& error)
    {
        result.Status = "NG";
        result.Category = "expectation mismatch";
        result.File = error.File;
        result.Line = error.Line;
        result.Message = error.what();
    }
    catch (const std::exception& error)
    {
        result.Status = "NG";
        result.Category = "unexpected exception";
        result.Message = error.what();
    }
    catch (...)
    {
        result.Status = "NG";
        result.Category = "unknown exception";
        result.Message = "non-standard exception";
    }

    RecordTestResult(result);
    if (result.Status == "NG")
    {
        BOOST_ERROR(result.Name + ": " + result.Message);
    }
}

void Check(bool condition, const std::string& message, const std::source_location& location)
{
    if (!condition)
    {
        throw TestFailure(message, location);
    }
}

Fixtures::Fixtures()
{
    directory_ = std::filesystem::temp_directory_path() / ("MusicComTests-" + std::to_string(GetCurrentProcessId()));
    Check(std::filesystem::create_directory(directory_), "fixture directory already exists");
}

Fixtures::~Fixtures()
{
    std::error_code error;
    std::filesystem::remove_all(directory_, error);
}

std::filesystem::path Fixtures::Write(const std::string& name, const std::string& text)
{
    const auto path = directory_ / name;
    std::ofstream output(path, std::ios::binary);
    output << text;
    output.close();
    Check(!output.fail(), "cannot write fixture");
    return path;
}

std::unique_ptr<MusicData> Fixtures::Parse(const std::string& text)
{
    return ParseMML(Write("case.mml", text).string().c_str());
}

std::string Fixtures::Error(const std::string& text)
{
    try
    {
        Parse(text);
    }
    catch (const std::runtime_error& error)
    {
        return error.what();
    }
    throw std::runtime_error("invalid MML was accepted: " + text);
}

ToneProbe::ToneProbe(const MusicData& music, int rate)
    : ToneSequencerBase([](int, int) {}, music, music.GetChannelTail(0), rate)
{
}

void ToneProbe::InitializeImpl(PartData&)
{
    key_on_count_ = 0;
    key_off_count_ = 0;
    vibrato_count_ = 0;
    portamento_count_ = 0;
    last_vibrato_depth_ = 0;
    last_volume_ = -1;
}

void ToneProbe::ApplyVibratoEffect(int, int, int depth)
{
    ++vibrato_count_;
    last_vibrato_depth_ = depth;
}

CommandIterator ToneProbe::GetHead() const
{
    return music_data_.GetChannelHead(0);
}
