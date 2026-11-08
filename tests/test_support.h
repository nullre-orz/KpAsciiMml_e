#pragma once

#include "../src/KbAsciiMml/musiccom/musdata.h"
#include "../src/KbAsciiMml/musiccom/tonesequencerbase.h"
#include <filesystem>
#include <functional>
#include <memory>
#include <source_location>
#include <string>

class TestRunner
{
public:
    explicit TestRunner(std::string suite_name);
    void Run(const std::string& name, const std::function<void()>& body) const;

private:
    std::string suite_name_;
};

void Check(bool condition, const std::string& message,
           const std::source_location& location = std::source_location::current());

class Fixtures
{
public:
    Fixtures();
    ~Fixtures();
    std::filesystem::path Write(const std::string& name, const std::string& text);
    std::unique_ptr<MusicCom::MusicData> Parse(const std::string& text);
    std::string Error(const std::string& text);
    const std::filesystem::path& Directory() const { return directory_; }

private:
    std::filesystem::path directory_;
};

class ToneProbe : public MusicCom::ToneSequencerBase
{
public:
    explicit ToneProbe(const MusicCom::MusicData& music, int rate = 48000);
    int Octave() const { return part_data_.Note.Octave; }
    const MusicCom::PartData& State() const { return part_data_; }
    int KeyOnCount() const { return key_on_count_; }
    int KeyOffCount() const { return key_off_count_; }
    int VibratoCount() const { return vibrato_count_; }
    int PortamentoCount() const { return portamento_count_; }
    int LastVibratoDepth() const { return last_vibrato_depth_; }
    int LastVolume() const { return last_volume_; }

private:
    MusicCom::CommandIterator GetHead() const override;
    void InitializeImpl(MusicCom::PartData&) override;
    void KeyOn() override { ++key_on_count_; }
    void KeyOff() override { ++key_off_count_; }
    bool NeedsVolumeEffect(const MusicCom::PartData&) const override { return true; }
    void UpdateTone(int tone, MusicCom::PartData& data) override { data.Note.Tone = tone; }
    void ApplyVibratoEffect(int, int, int depth) override;
    void ApplyPortamentoEffect(int, int, int, int, int, int) override { ++portamento_count_; }
    void SetTone(int, int) override {}
    void SetVolume(int volume) override { last_volume_ = volume; }

    int key_on_count_ = 0;
    int key_off_count_ = 0;
    int vibrato_count_ = 0;
    int portamento_count_ = 0;
    int last_vibrato_depth_ = 0;
    int last_volume_ = -1;
};

void RunParserChannelTests(TestRunner& runner, Fixtures& fixtures);
void RunParserNoteTests(TestRunner& runner, Fixtures& fixtures);
void RunParserControlTests(TestRunner& runner, Fixtures& fixtures);
void RunParserEffectTests(TestRunner& runner, Fixtures& fixtures);
void RunParserDirectRegisterTests(TestRunner& runner, Fixtures& fixtures);
void RunParserLoopTests(TestRunner& runner, Fixtures& fixtures);
void RunParserStrTests(TestRunner& runner, Fixtures& fixtures);
void RunParserSSGEnvTests(TestRunner& runner, Fixtures& fixtures);
void RunParserErrorTests(TestRunner& runner, Fixtures& fixtures);
void RunSequencerTests(TestRunner& runner, Fixtures& fixtures);
void RunDataTests(TestRunner& runner, Fixtures& fixtures);
void RunSoundDataTests(TestRunner& runner, Fixtures& fixtures);
void RunBehaviorTests(TestRunner& runner, Fixtures& fixtures);
void RunRegisterTests(TestRunner& runner, Fixtures& fixtures);
void RunMusicComTests(TestRunner& runner, Fixtures& fixtures);
void RunSoundParserErrorTests(TestRunner& runner, Fixtures& fixtures);
void RunPluginTests(TestRunner& runner, Fixtures& fixtures);
