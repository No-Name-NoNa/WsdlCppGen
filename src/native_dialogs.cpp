#include "native_dialogs.hpp"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <sstream>
#include <string>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <shlobj.h>
#include <windows.h>
#endif

namespace WsdlCppGen
{
namespace
{
#if defined(_WIN32)
std::string ToUtf8(const wchar_t* value)
{
    if (!value || !*value)
        return {};
    int count = WideCharToMultiByte(CP_UTF8, 0, value, -1, nullptr, 0, nullptr, nullptr);
    if (count <= 0)
        return {};
    std::string result(static_cast<size_t>(count - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value, -1, result.data(), count, nullptr, nullptr);
    return result;
}

DialogResult SelectWithIFileDialog(bool folder)
{
    HRESULT init = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    const bool should_uninitialize = SUCCEEDED(init);
    if (FAILED(init) && init != RPC_E_CHANGED_MODE)
        return {std::nullopt, "CoInitializeEx failed"};

    IFileDialog* dialog = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog));
    if (FAILED(hr) || !dialog)
    {
        if (should_uninitialize)
            CoUninitialize();
        return {std::nullopt, "Failed to create Windows file dialog"};
    }

    DWORD options = 0;
    dialog->GetOptions(&options);
    options |= FOS_FORCEFILESYSTEM;
    if (folder)
        options |= FOS_PICKFOLDERS;
    dialog->SetOptions(options);
    dialog->SetTitle(folder ? L"Select output directory" : L"Select WSDL file");

    COMDLG_FILTERSPEC filters[] = {
        {L"WSDL files", L"*.wsdl"},
        {L"All files", L"*.*"},
    };
    if (!folder)
    {
        dialog->SetFileTypes(2, filters);
        dialog->SetFileTypeIndex(1);
    }

    hr = dialog->Show(nullptr);
    if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED))
    {
        dialog->Release();
        if (should_uninitialize)
            CoUninitialize();
        return {std::nullopt, {}};
    }
    if (FAILED(hr))
    {
        dialog->Release();
        if (should_uninitialize)
            CoUninitialize();
        return {std::nullopt, "Windows file dialog failed"};
    }

    IShellItem* item = nullptr;
    hr = dialog->GetResult(&item);
    if (FAILED(hr) || !item)
    {
        dialog->Release();
        if (should_uninitialize)
            CoUninitialize();
        return {std::nullopt, "Windows file dialog returned no selection"};
    }

    PWSTR wide_path = nullptr;
    hr = item->GetDisplayName(SIGDN_FILESYSPATH, &wide_path);
    std::string path = SUCCEEDED(hr) ? ToUtf8(wide_path) : std::string();
    if (wide_path)
        CoTaskMemFree(wide_path);
    item->Release();
    dialog->Release();
    if (should_uninitialize)
        CoUninitialize();

    if (path.empty())
        return {std::nullopt, "Selected path could not be read"};
    return {path, {}};
}
#else
std::string ShellQuote(const std::string& value)
{
    std::string result = "'";
    for (char ch : value)
    {
        if (ch == '\'')
            result += "'\\''";
        else
            result += ch;
    }
    result += "'";
    return result;
}

DialogResult RunSelectionCommand(const std::string& command)
{
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe)
        return {std::nullopt, "Failed to start system file picker"};

    std::array<char, 512> buffer{};
    std::string output;
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe))
        output += buffer.data();

    int code = pclose(pipe);
    if (code != 0)
        return {std::nullopt, {}};

    while (!output.empty() && (output.back() == '\n' || output.back() == '\r'))
        output.pop_back();
    if (output.empty())
        return {std::nullopt, {}};
    return {output, {}};
}
#endif
}

DialogResult SelectWsdlFile()
{
#if defined(_WIN32)
    return SelectWithIFileDialog(false);
#elif defined(__APPLE__)
    return RunSelectionCommand("osascript -e 'POSIX path of (choose file of type {\"wsdl\"} with prompt \"Select WSDL file\")'");
#else
    return RunSelectionCommand("zenity --file-selection --title='Select WSDL file' --file-filter='WSDL files | *.wsdl' --file-filter='All files | *'");
#endif
}

DialogResult SelectOutputDirectory()
{
#if defined(_WIN32)
    return SelectWithIFileDialog(true);
#elif defined(__APPLE__)
    return RunSelectionCommand("osascript -e 'POSIX path of (choose folder with prompt \"Select output directory\")'");
#else
    return RunSelectionCommand("zenity --file-selection --directory --title='Select output directory'");
#endif
}

DialogResult OpenDirectoryInShell(const std::string& path)
{
    if (path.empty())
        return {std::nullopt, "Output directory is empty"};
    if (!std::filesystem::exists(path))
        return {std::nullopt, "Output directory does not exist"};

#if defined(_WIN32)
    std::wstring wide_path;
    int count = MultiByteToWideChar(CP_UTF8, 0, path.data(), static_cast<int>(path.size()), nullptr, 0);
    if (count > 0)
    {
        wide_path.resize(static_cast<size_t>(count));
        MultiByteToWideChar(CP_UTF8, 0, path.data(), static_cast<int>(path.size()), wide_path.data(), count);
    }
    HINSTANCE result = ShellExecuteW(nullptr, L"open", wide_path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    if (reinterpret_cast<intptr_t>(result) <= 32)
        return {std::nullopt, "Failed to open output directory"};
    return {std::nullopt, {}};
#elif defined(__APPLE__)
    std::string command = "open " + ShellQuote(path);
    return {std::nullopt, std::system(command.c_str()) == 0 ? std::string() : "Failed to open output directory"};
#else
    std::string command = "xdg-open " + ShellQuote(path) + " >/dev/null 2>&1 &";
    return {std::nullopt, std::system(command.c_str()) == 0 ? std::string() : "Failed to open output directory"};
#endif
}
}
