#include "musiccom/musiccom.h"
#include "resource.h"
#include <Windows.h>
#include <array>
#include <boost/lexical_cast.hpp>
#include <iterator>
#include <kmp_pi.h>
#include <shlwapi.h>

#pragma comment(lib, "Shlwapi.lib")

using namespace std;
using namespace boost;

HMODULE hDllModule = nullptr;

#define KMPPLUGIN_VERSION 1

template<typename T>
T GetSetting(LPCWSTR fileName, LPCWSTR key, T defaultValue)
{
    try
    {
        std::array<wchar_t, 256> buf{};
        GetPrivateProfileStringW(L"KbAsciiMml", key, nullptr, buf.data(), static_cast<DWORD>(buf.size()), fileName);
        return boost::lexical_cast<int>(buf.data());
    }
    catch (const boost::bad_lexical_cast&)
    {
        return defaultValue;
    }
}

class KbAsciiMml
{
public:
    KbAsciiMml();
    BOOL Open(const char* cszFileName, SOUNDINFO* pInfo);
    DWORD Render(BYTE* pBuffer, DWORD dwSize);
    DWORD SetPosition(DWORD dwPos);
    ~KbAsciiMml();

private:
    MusicCom::MusicCom musicCom;
    uint bytespersample;
    SOUNDINFO info;
};

KbAsciiMml::KbAsciiMml()
    : bytespersample(0),
      info{}
{
    std::array<wchar_t, MAX_PATH> iniName{};
    GetModuleFileNameW(hDllModule, iniName.data(), static_cast<DWORD>(iniName.size()));
    PathRemoveExtensionW(iniName.data());
    PathAddExtensionW(iniName.data(), L".ini");

    int fmvol = GetSetting(iniName.data(), L"FMVolume", 0);
    int psgvol = GetSetting(iniName.data(), L"PSGVolume", 0);
    int soundtempo = GetSetting(iniName.data(), L"SoundTempo", MusicCom::MusicCom::SOUND_EFFECT_DEFAULT_TEMPO);

    musicCom.SetFMVolume(fmvol);
    musicCom.SetPSGVolume(psgvol);
    musicCom.SetSoundTempo(soundtempo);
}

KbAsciiMml::~KbAsciiMml() = default;

BOOL KbAsciiMml::Open(const char* cszFileName, SOUNDINFO* pInfo)
{
    if (pInfo == nullptr)
    {
        return FALSE;
    }

    if (pInfo->dwSamplesPerSec == 0)
    {
        pInfo->dwSamplesPerSec = 55466;
    }
    pInfo->dwBitsPerSample = 16;
    pInfo->dwLength = 0xFFFFFFFF;
    pInfo->dwChannels = 2;
    pInfo->dwSeekable = 0;
    pInfo->dwUnitRender = 0;
    pInfo->dwReserved1 = 0xFFFFFFFF;
    pInfo->dwReserved2 = 0;

    try
    {
        if (!musicCom.Load(cszFileName))
        {
            return FALSE;
        }
    }
    catch (const std::exception& e)
    {
        MessageBoxA(nullptr, e.what(), "エラー", MB_OK);
        return FALSE;
    }

    if (!musicCom.PrepareMix(pInfo->dwSamplesPerSec))
        return FALSE;

    bytespersample = pInfo->dwChannels * pInfo->dwBitsPerSample / 8;
    info = *pInfo;
    return TRUE;
}

DWORD KbAsciiMml::Render(BYTE* Buffer, DWORD dwSize)
{
    musicCom.Mix(reinterpret_cast<int16*>(Buffer), dwSize / bytespersample);
    return dwSize;
}

DWORD KbAsciiMml::SetPosition(DWORD dwPos)
{
    musicCom.PrepareMix(info.dwSamplesPerSec);
    return 0;
}

static HKMP WINAPI kmp_Open(const char* cszFileName, SOUNDINFO* pInfo)
{
    OutputDebugStringA(cszFileName);

    KbAsciiMml* pKbAsciiMml = new KbAsciiMml;
    if (pKbAsciiMml->Open(cszFileName, pInfo))
    {
        return static_cast<HKMP>(pKbAsciiMml);
    }
    else
    {
        delete pKbAsciiMml;
        return nullptr;
    }
}

static void WINAPI kmp_Close(HKMP hKMP)
{
    KbAsciiMml* pKbAsciiMml = static_cast<KbAsciiMml*>(hKMP);
    delete pKbAsciiMml;
}

static DWORD WINAPI kmp_Render(HKMP hKMP, BYTE* Buffer, DWORD dwSize)
{
    KbAsciiMml* pKbAsciiMml = static_cast<KbAsciiMml*>(hKMP);
    if (pKbAsciiMml)
    {
        return pKbAsciiMml->Render(Buffer, dwSize);
    }
    else
    {
        return 0;
    }
}

static DWORD WINAPI kmp_SetPosition(HKMP hKMP, DWORD dwPos)
{
    KbAsciiMml* pKbAsciiMml = static_cast<KbAsciiMml*>(hKMP);
    if (pKbAsciiMml)
    {
        return pKbAsciiMml->SetPosition(dwPos);
    }
    else
    {
        return 0;
    }
}

KMPMODULE* WINAPI kmp_GetTestModule()
{
    static std::array<const char*, 2> ppszSupportExts = {".mml", nullptr};

    static KMPMODULE Mod =
        {
            KMPMODULE_VERSION, // dwVersion
            KMPPLUGIN_VERSION, // dwPluginVersion
            VER_LEGAL_COPYRIGHT, // pszCopyright
            VER_FILE_DESCRIPTION, // pszDescription
            ppszSupportExts.data(), // ppszSupportExts
            1, // dwReentrant
            nullptr, // Init
            nullptr, // Deinit
            kmp_Open, // Open
            nullptr, // OpenFromBuffer
            kmp_Close, // Close
            kmp_Render, // Render
            kmp_SetPosition // SetPosition
        };

    return &Mod;
}

BOOL APIENTRY DllMain(
    HMODULE hModule,
    DWORD ul_reason_for_call,
    LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        hDllModule = hModule;
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
