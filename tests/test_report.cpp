#include "test_report.h"
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <regex>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace
{
    constexpr int EXPECTED_CONTEXT_COUNT = 188;

    std::vector<TestResult>& Results()
    {
        static std::vector<TestResult> results;
        return results;
    }

    std::string EscapeMarkdown(std::string text)
    {
        for (std::size_t pos = 0; (pos = text.find('|', pos)) != std::string::npos; pos += 2)
        {
            text.replace(pos, 1, "\\|");
        }
        std::replace(text.begin(), text.end(), '\r', ' ');
        std::replace(text.begin(), text.end(), '\n', ' ');
        return text;
    }

    std::string SuiteName(const std::string& suite)
    {
        static const std::unordered_map<std::string, std::string> names = {
            {"Parser", "MML解析"}, {"Registers", "レジスタ制御"}, {"MusicComFacade", "再生初期化"},
            {"DataModel", "データモデル"}, {"SoundData", "SOUND.DAT解析"},
            {"Sequencer", "シーケンサー"}, {"Behavior", "再生状態"}};
        return names.at(suite);
    }

    std::string ContextDescription(const std::string& context)
    {
        static const std::unordered_map<std::string, std::string> fixed = {
            {"channel data and global tempo", "「1:C4 V10 T150」を解析し、テンポ150とNOTE C・VOLUME 10のコマンド列が得られることを確認"},
            {"last tempo in the same channel line wins", "同じチャンネル行の「T120 C64 T150 D64」を解析し、最後のT150が全体テンポとなりTがコマンド列へ残らないことを確認"},
            {"last tempo across lines of the same channel wins", "同一チャンネルの複数行でT90、T180を指定し、後のT180が全体テンポになることを確認"},
            {"last tempo across different channels wins", "複数チャンネルでT200、T100、T240を指定し、ソース上で最後のT240が全体テンポになることを確認"},
            {"all music channels and multiple lines", "全6音楽チャンネルとチャンネル1の複数行を解析し、チャンネル割当てとPAUSEを挟むコマンド連結を確認"},
            {"rhythm part data", "「D:@2 A64」を解析し、DパートへTONE 2とNOTE Aが格納されることを確認"},
            {"tie command in channel data", "「C64 & D64」を解析し、NOTE・TIE・NOTEの順でコマンドが格納されることを確認"},
            {"reject tie argument", "引数付きタイ「&1」を解析し、構文エラーとして収集されることを確認"},
            {"reject missing default length", "引数のない既定音長「L」を解析し、構文エラーとして収集されることを確認"},
            {"relative octave commands", "「O4 < > C64」を解析し、OCTAVE・OCTAVE_DOWN・OCTAVE_UP・NOTEの順で格納されることを確認"},
            {"Y omitted data", "「Y7」を解析し、省略されたデータが0で補完されることを確認"},
            {"Y address and data", "Yコマンドのアドレスとデータを解析し、両方が8bit値として格納されることを確認"},
            {"reject Y excess argument", "引数過剰の「Y7,1,2」を解析し、構文エラーとして収集されることを確認"},
            {"loop commands in channel data", "チャンネル内のループ開始・脱出・終端を解析し、コマンド種別と回数が正しく格納されることを確認"},
            {"defined STR and part forward reference", "定義済みSTRとパートからの前方参照を解析し、参照先とコマンド列が正しく解決されることを確認"},
            {"duplicate STR keeps the first definition", "同名STRを重複定義し、最初の定義が維持されることを確認"},
            {"SSGENV values omissions replacement and channel use", "SSGENVの値、省略値、再定義、チャンネルからの選択を解析し、期待するエンベロープが格納されることを確認"},
            {"SSGENV errors use the header line", "複数行SSGENVの範囲外番号を解析し、エラー位置が継続行ではなくヘッダー行になることを確認"},
            {"reject unknown command in channel data", "未知のMMLコマンドを解析し、構文エラーとして収集されることを確認"},
            {"collect errors through last line", "複数の不正コマンドを含むファイルを解析し、最終行までエラーが一括収集されることを確認"},
            {"SSG registers envelope mixer and effect exclusion", "SSGの音程・ノイズ・音量・エンベロープ・ミキサーの各レジスタ値と、効果音によるチャンネル排他を確認"},
            {"plugin zero sample rate uses default", "プラグインをサンプルレート0で開き、既定値55466 Hzが返されることを確認"},
            {"SOUND.DAT error detail limit", "SOUND.DATの解析エラー99〜102件で詳細100件と省略件数を確認"},
            {"SOUND.DAT error excerpt limit", "長いSOUND.DAT解析エラーの抜粋が256バイトで省略されることを確認"},
            {"SOUND.DAT length wrapping and blocks", "SOUND.DATの音長16bit循環と複数ブロックのイテレーター終端を確認"},
            {"SOUND.DAT total length exceeds 32 bits", "複数ブロックの合計音長が32bitを超えても正しく計算されることを確認"},
            {"SOUND.DAT signed arithmetic and loop", "SOUND.DATの符号付き変化量とループの音程・音量・ノイズを確認"},
            {"SOUND.DAT invalid division fallback", "除算できないSOUND.DATで組込み効果音にフォールバックすることを確認"},
            {"MML error detail limit", "MMLの解析エラー99〜102件で詳細100件と省略件数を確認"},
            {"MML error excerpt limit", "長いMML解析エラーの抜粋が256バイトで省略されることを確認"},
            {"MML command execution limit", "1 tickのコマンド実行上限の境界と停止後の再開抑止を確認"},
            {"MML lookahead command limit", "音符後の先読みを含む1 tickのコマンド上限と再開抑止を確認"},
            {"sample rate boundaries and prescaler", "0、7999、8000、48000、55466、int最大値とその直後を指定し、受理範囲とY47,0・T5880での再生を確認"},
            {"invalid sample rate preserves playback", "再生中の無効なサンプルレート指定後のPCMを、指定しない場合と比較して再生状態の維持を確認"},
            {"FM restart and failed MML load preserve playback", "FM全3チャンネルの発音中に再初期化して曲頭PCMが一致することと、MML読込み失敗後も以前の再生データが維持されることを確認"},
            {"MusicData defaults and undefined definitions", "MusicDataの既定テンポ、チャンネル状態、未定義FM音色とSSGENVの共有既定値を確認"},
            {"SoundData default and undefined rhythms", "未定義リズムが空の共有既定値を返し、イテレーターが終端を示すことを確認"},
            {"Command and PartData defaults", "Command、PartDataの音符・再生・エフェクト状態が規定の初期値になることを確認"},
            {"FMSound setters and Clear", "FMSoundの各設定値が所定bitへ変換・飽和され、Clearで全フィールドが0になることを確認"},
            {"missing SOUND.DAT uses embedded definitions", "SOUND.DATが存在しない場合に組込み定義へ切り替わり、リズム0の長さが8になることを確認"},
            {"valid SOUND.DAT values and iterator", "正常なSOUND.DATを解析し、4 tick分の音程・音量・ノイズ変化、飽和、イテレーターを確認"},
            {"SOUND.DAT omitted arguments and comma-free values", "SOUND.DATの省略引数と空白区切りを解析し、既定音長、0補完、周期倍率が反映されることを確認"},
            {"invalid SOUND.DAT falls back to embedded definitions", "不正なSOUND.DATを解析し、部分結果を残さず組込み定義へ全面的に切り替えることを確認"},
            {"D Y callback and paused command progression", "DパートのY書込みが順番どおり通知され、行末待機中は停止し、再開後に次行が実行されることを確認"},
            {"D waiting sound reaches next frame and termination", "Dパートの行末待機中も効果音が独立周期で100→110→120→130と進み、終端でキーオフすることを確認"},
            {"note state and Q gate calculation", "「Q4 C4」を再生し、音符範囲0～16 tick、キーオン0 tick、キーオフ8 tickになることを確認"},
            {"tie suppresses second key on", "「C4 & C4」を再生し、タイ境界で再キーオンとキーオフが発生せず音符終端が32 tickへ延長されることを確認"},
            {"P U I exclusivity and zero periods", "P・U・Iの共存と排他、およびU/Iの周期0が1へ補正されることを確認"},
            {"pause resume and fresh initialization", "行末で停止し、待機中は進行せず、再開後に次行を発音し、新規初期化では曲頭へ戻ることを確認"}};
        if (const auto item = fixed.find(context); item != fixed.end())
        {
            return item->second;
        }

        std::smatch match;
        if (std::regex_match(context, match, std::regex("note (.+)")))
        {
            static const std::unordered_map<std::string, int> tones =
                {{"C",0},{"D",2},{"E",4},{"F",5},{"G",7},{"A",9},{"B",11},{"C+",1},{"C#",1},{"D-",1}};
            return "音符「" + match[1].str() + "4」を解析し、NOTEコマンドの内部音程が" +
                   std::to_string(tones.at(match[1].str())) + "、音長が16 tickになることを確認";
        }
        if (std::regex_match(context, match, std::regex("(length|reject length) ([CLRW])(.+)")))
        {
            static const std::unordered_map<std::string, std::string> kinds =
                {{"C","音符"},{"L","既定音長"},{"R","休符"},{"W","待機"}};
            const auto command = match[2].str();
            const auto token = match[3].str();
            if (match[1] == "reject length")
            {
                return kinds.at(command) + "「" + command + token + "」を解析し、規定外の音長が構文エラーとして収集されることを確認";
            }
            static const std::unordered_map<std::string, int> ticks =
                {{"1",64},{"2",32},{"4",16},{"8",8},{"16",4},{"32",2},{"64",1},{"2.",48},{"4.",24},{"8.",12},{"16.",6},{"32.",3}};
            return kinds.at(command) + "「" + command + token + "」を解析し、音長が" +
                   std::to_string(ticks.at(token)) + " tickとして格納されることを確認";
        }
        if (std::regex_match(context, match, std::regex("optional length (.+)")))
        {
            return "音符「" + match[1].str() + "」を解析し、省略またはカンマ区切りの音長が規定値として格納されることを確認";
        }
        if (std::regex_match(context, match, std::regex("control (.+)")))
        {
            static const std::unordered_map<std::string, std::string> expected = {
                {"V-1","VOLUME 255"},{"V255","VOLUME 255"},{"V256","VOLUME 0"},{"V65535","VOLUME 255"},{"V65536","VOLUME 0"},
                {"M-1","ENV_PERIOD 65535"},{"M65536","ENV_PERIOD 0"},{"N-1","DETUNE -1"},{"N65535","DETUNE -1"},{"N256","DETUNE 0"},
                {"O0","OCTAVE 0"},{"O8","OCTAVE 8"},{"O65536","OCTAVE 0"},{"@0","TONE 0"},{"@20","TONE 20"},{"@65556","TONE 20"},
                {"Q4","GATE_TIME 4"},{"S13","ENV_FORM 13"}};
            const auto token = match[1].str();
            return "制御コマンド「" + token + "」を解析し、16bit循環と8bit変換後の値が" + expected.at(token) + "として格納されることを確認";
        }
        if (std::regex_match(context, match, std::regex("reject control (.+)")))
            return "制御コマンド「" + match[1].str() + "」を解析し、範囲外値または不正な引数が構文エラーとして収集されることを確認";
        if (std::regex_match(context, match, std::regex("effect (.+)")))
            return "エフェクトコマンド「" + match[1].str() + "」を解析し、省略引数を0で補完した3引数が正しく格納されることを確認";
        if (std::regex_match(context, match, std::regex("reject effect (.+)")))
            return "エフェクトコマンド「" + match[1].str() + "」を解析し、4番目の過剰引数が構文エラーとして収集されることを確認";
        if (std::regex_match(context, match, std::regex("loop depth (.+)")))
            return "「" + match[1].str() + "」で始まるパートにループを入れ、15重を受理し16重を構文エラーとして収集することを確認";
        if (std::regex_match(context, match, std::regex("tempo (.+)")))
        {
            static const std::unordered_map<std::string, int> samples =
                {{"0",5427},{"30",5427},{"120",1356},{"5880",27},{"5881",7089},{"65535",7089},{"65536",5427}};
            const auto token = match[1].str();
            return "テンポ「T" + token + "」を48 kHzで初期化し、1 tickの残りサンプル数が" + std::to_string(samples.at(token)) + "になることを確認";
        }
        if (std::regex_match(context, match, std::regex("relative octave (.+)")))
        {
            static const std::unordered_map<std::string, int> octaves = {{"0",1},{"6",7},{"7",7},{"8",8}};
            const auto token = match[1].str();
            return "「O" + token + " > C64」を再生し、相対オクターブ上昇後の内部オクターブが" + std::to_string(octaves.at(token)) + "になることを確認";
        }
        throw std::runtime_error("検査内容が未定義です: " + context);
    }

    std::string CategoryName(const std::string& category)
    {
        static const std::unordered_map<std::string, std::string> names =
            {{"expectation mismatch","期待値不一致"},{"unexpected exception","予期しない例外"},{"unknown exception","不明な例外"}};
        return names.at(category);
    }

    void WriteReport()
    {
        const char* path = std::getenv("MUSICCOM_TEST_REPORT");
        if (path == nullptr || *path == '\0') return;
        const auto& results = Results();
        if (results.size() > EXPECTED_CONTEXT_COUNT) throw std::runtime_error("検査項目が188件を超えています");
        const int ok = static_cast<int>(std::count_if(results.begin(), results.end(), [](const auto& result) { return result.Status == "OK"; }));
        const int ng = static_cast<int>(results.size()) - ok;
        const int outside = EXPECTED_CONTEXT_COUNT - static_cast<int>(results.size());
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        output << "# MusicComTests 自動テスト結果\n\n## 集計\n\n"
               << "| 結果 | 件数 |\n|---|---:|\n| OK | " << ok << " |\n| NG | " << ng
               << " |\n| 対象外 | " << outside << " |\n| 合計 | " << EXPECTED_CONTEXT_COUNT
               << " |\n\n## 検査結果\n\n| No. | 分類 | 検査内容 | 結果 |\n|---:|---|---|---|\n";
        for (std::size_t index = 0; index < results.size(); ++index)
        {
            const auto& result = results[index];
            output << "| " << index + 1 << " | " << SuiteName(result.Suite) << " | "
                   << EscapeMarkdown(ContextDescription(result.Name)) << " | " << result.Status << " |\n";
        }
        output << "\n## NG箇所の分析\n\n";
        if (ng == 0)
        {
            output << "NGとなった検査項目はありません。\n";
            return;
        }
        output << "| 分類 | 検査内容 | 原因分類 | 発生箇所 | メッセージ |\n|---|---|---|---|---|\n";
        for (const auto& result : results)
        {
            if (result.Status == "NG")
            {
                output << "| " << SuiteName(result.Suite) << " | " << EscapeMarkdown(ContextDescription(result.Name))
                       << " | " << CategoryName(result.Category) << " | " << EscapeMarkdown(result.File) << ':' << result.Line
                       << " | " << EscapeMarkdown(result.Message) << " |\n";
            }
        }
    }
}

void RecordTestResult(const TestResult& result)
{
    Results().push_back(result);
    WriteReport();
}
