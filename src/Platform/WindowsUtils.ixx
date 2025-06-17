export module Platform.WindowsUtils;

export import <Windows.h>;
import <shlobj.h>;
import <direct.h>;
import std;

namespace Windows {
    // Helper: UTF-8 <-> UTF-16 conversion
    inline std::wstring Utf8ToUtf16(std::string_view utf8) {
        int len = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), (int) utf8.size(), nullptr, 0);
        std::wstring wstr(len, 0);
        MultiByteToWideChar(CP_UTF8, 0, utf8.data(), (int) utf8.size(), wstr.data(), len);
        return wstr;
    }

    inline std::string Utf16ToUtf8(const std::wstring &wstr) {
        int len = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int) wstr.size(), nullptr, 0, nullptr, nullptr);
        std::string str(len, 0);
        WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int) wstr.size(), str.data(), len, nullptr, nullptr);
        return str;
    }

    export std::optional<std::string> OpenFileDialog(std::wstring_view filter = L"All Files\0*.*\0") {
        wchar_t originalDir[MAX_PATH];
        _wgetcwd(originalDir, MAX_PATH);

        wchar_t filename[MAX_PATH] = L"";

        OPENFILENAMEW ofn;
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = NULL;
        ofn.lpstrFilter = filter.data();
        ofn.lpstrFile = filename;
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

        std::optional<std::string> result;
        if (GetOpenFileNameW(&ofn)) {
            result = Utf16ToUtf8(filename);
        }

        _wchdir(originalDir);
        return result;
    }

    export std::optional<std::string> OpenDirectoryDialog() {
        wchar_t originalDir[MAX_PATH];
        _wgetcwd(originalDir, MAX_PATH);
        wchar_t path[MAX_PATH] = L"";

        BROWSEINFOW bi = {0};
        bi.lpszTitle = L"Select a folder";
        bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
        LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);

        if (pidl && SHGetPathFromIDListW(pidl, path)) {
            CoTaskMemFree(pidl);
            _wchdir(originalDir);
            return Utf16ToUtf8(path);
        }
        if (pidl) CoTaskMemFree(pidl);
        _wchdir(originalDir);
        return {};
    }

    export struct ProcessOutput {
        std::string stdout_str;
        std::string stderr_str;
    };

    // ShowErrorMessage: UTF-8 support
    export void ShowErrorMessage(const char *message, const char *title = "Error") {
        auto wmsg = Utf8ToUtf16(message);
        auto wtitle = Utf8ToUtf16(title);
        MessageBoxW(NULL, wmsg.c_str(), wtitle.c_str(), MB_OK | MB_ICONERROR);
    }

    // RunProcessWithOutput: UTF-8 support
    export std::optional<ProcessOutput> RunProcessWithOutput(const std::string &cmd) {
        HANDLE outRead, outWrite, errRead, errWrite;
        SECURITY_ATTRIBUTES sa{sizeof(sa), NULL, TRUE};

        if (!CreatePipe(&outRead, &outWrite, &sa, 0)) return std::nullopt;
        if (!CreatePipe(&errRead, &errWrite, &sa, 0)) {
            CloseHandle(outRead);
            CloseHandle(outWrite);
            return std::nullopt;
        }

        STARTUPINFOW si = {sizeof(si)};
        si.dwFlags = STARTF_USESTDHANDLES;
        si.hStdOutput = outWrite;
        si.hStdError = errWrite;
        si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

        PROCESS_INFORMATION pi{};
        // Convert UTF-8 command to UTF-16
        std::wstring wcmd = Utf8ToUtf16(cmd);
        // CreateProcessW requires non-const buffer
        std::vector<wchar_t> cmdBuf(wcmd.begin(), wcmd.end());
        cmdBuf.push_back(L'\0');

        BOOL success = CreateProcessW(
            NULL, cmdBuf.data(), NULL, NULL, TRUE,
            CREATE_NO_WINDOW, NULL, NULL, &si, &pi
        );

        CloseHandle(outWrite);
        CloseHandle(errWrite);

        if (!success) {
            CloseHandle(outRead);
            CloseHandle(errRead);
            ShowErrorMessage("Failed to create process. Please check if all required files are present and the command is correct.");
            return std::nullopt;
        }

        std::string outStr, errStr;
        char buf[4096];
        DWORD read;

        while (ReadFile(outRead, buf, sizeof(buf), &read, NULL) && read)
            outStr.append(buf, read);

        while (ReadFile(errRead, buf, sizeof(buf), &read, NULL) && read)
            errStr.append(buf, read);

        CloseHandle(outRead);
        CloseHandle(errRead);

        WaitForSingleObject(pi.hProcess, INFINITE);
        DWORD exitCode = 0;
        GetExitCodeProcess(pi.hProcess, &exitCode);

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        if (exitCode != 0) {
            ShowErrorMessage(
                ("Process exited abnormally. Exit code: " + std::to_string(exitCode) +
                 ". Please report this issue to the compiler vendor with the source it compiled.").c_str());
        }

        return ProcessOutput{std::move(outStr), std::move(errStr)};
    }
}
