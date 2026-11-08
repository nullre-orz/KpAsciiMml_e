#define NOMINMAX
#include "test_support.h"
#include <windows.h>
#include <kmp_pi.h>
#include <array>
#include <filesystem>
#include <memory>

void RunPluginTests(TestRunner& runner, Fixtures& fixtures)
{
    runner.Run("plugin zero sample rate uses default", [&] {
        std::array<wchar_t, 32768> executable{};
        const auto length = GetModuleFileNameW(nullptr, executable.data(), static_cast<DWORD>(executable.size()));
        Check(length > 0 && length < executable.size(), "cannot locate test executable");
        const std::filesystem::path executable_path(executable.data());
        const auto configuration = executable_path.parent_path().filename();
        const auto platform = executable_path.parent_path().parent_path().filename();
        const auto dll = executable_path.parent_path().parent_path().parent_path().parent_path() / "KbAsciiMml" / platform / configuration / "KbAsciiMml.dll";
        const auto module = std::unique_ptr<std::remove_pointer_t<HMODULE>, decltype(&FreeLibrary)>(LoadLibraryW(dll.c_str()), &FreeLibrary);
        Check(module != nullptr, "cannot load KbAsciiMml.dll");
        const auto get_module = reinterpret_cast<pfnGetKMPModule>(GetProcAddress(module.get(), "kmp_GetTestModule"));
        Check(get_module != nullptr, "plugin entry point is missing");
        auto plugin = get_module();
        Check(plugin && plugin->Open && plugin->Close, "plugin callbacks are missing");
        const auto path = fixtures.Write("plugin.mml", "1:T120 O4 C64\n");
        SOUNDINFO info{};
        auto handle = plugin->Open(path.string().c_str(), &info);
        Check(handle != nullptr, "plugin did not open valid MML");
        const auto rate = info.dwSamplesPerSec;
        plugin->Close(handle);
        Check(rate == 55466, "zero sample rate did not use the default");
    });
}
