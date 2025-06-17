export module ApplicationLayers;

import ImGui;
import BasicContext;
import Application;
import vulkan_hpp;
import Atomic;

import <windows.h>;
import "vendor/glfwpp/native.h";

std::string exec(const std::wstring &command) { // get the powershell command as input:
    std::string output_string;
    // first we create pipe which can share memory -> used for communication
    HANDLE readPipe = nullptr; // note PHANDLE means HANDLE pointer not pipe's handle
    HANDLE writePipe = nullptr; // do not declare handle here, since I need to use later

    SECURITY_ATTRIBUTES sa; // security attribute struct needed for create pipe function's parameter
    sa.nLength = sizeof(SECURITY_ATTRIBUTES); // doc
    sa.bInheritHandle = TRUE;
    // we need this to inherit from the pipe otherwise the app will be isolated and communication fails
    sa.lpSecurityDescriptor = nullptr; // default security with access token
    if (CreatePipe(&readPipe, &writePipe, &sa, 0) == 0) {
        throw std::runtime_error("Unable to create a pipe!");
    }

    STARTUPINFOW siw;
    ZeroMemory(&siw, sizeof(STARTUPINFOW));
    siw.cb = sizeof(STARTUPINFOW);
    siw.lpReserved = nullptr;
    siw.dwFlags |= STARTF_USESTDHANDLES; // using or since all others are now 0
    siw.hStdInput = nullptr; // the read is later we can read the result from it
    siw.hStdOutput = writePipe; // the powershell write output to pipe
    siw.hStdError = writePipe; // the powershell also should write error to pipe

    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
    std::vector command_buffer(command.begin(), command.end()); // copy the command to buffer (the command may be const)
    command_buffer.push_back(L'\0'); // copy did not contain the \0 so add \0 manually
    if (!CreateProcessW(nullptr, command_buffer.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr,
                        &siw, &pi)) {
        CloseHandle(readPipe);
        CloseHandle(writePipe);
        throw std::runtime_error("CreateProcess failed");
    }

    CloseHandle(writePipe); // we have done writing

    // read the output from the pipe
    // note that in windows, readFile can read handle infomation
    std::vector<char> buffer(4096);
    DWORD bytesRead;

    while (ReadFile(readPipe, buffer.data(), buffer.size(), &bytesRead, nullptr) && bytesRead != 0) {
        output_string.append(buffer.data(), bytesRead);
    }

    // clean up: close pipe
    CloseHandle(readPipe);
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    return output_string;
}

export class AppUiLayer : public IUpdatableLayer {
public:
    void OnUpdate() override {
        ImGui::Begin("Application UI");
        ImGui::Text("AppLayer");
        if (ImGui::Button("Rebuild Swap Chain")) {
            g_BasicContext->RecreateSwapChain();
        }
        if (ImGui::Button("Hello")) {
            std::cout << "Hello, App!" << std::endl;
        }

        // Button to get the handle of the game process
        {
            ImGui::Text("Game Name: ");
            ImGui::SameLine();
            size_t availableWidth = ImGui::GetContentRegionAvail().x;
            ImGui::SetNextItemWidth(availableWidth - ImGui::GetStyle().ItemSpacing.x - ImGui::CalcTextSize("Get Handle").x - 10);

            {
                auto gameNameProxy = m_GameName.GetProxy();
                ImGui::InputText("##GameName", &gameNameProxy.Get());
            }
            ImGui::SameLine();
            if (ImGui::Button("Get Handle")) {
                std::thread([this] {
                    OnGetHandleClicked();
                }).detach();
            }
        }


        ImGui::End();
    }

    void OnSubmitCommandBuffer(vk::CommandBuffer commandBuffer, std::vector<std::any> &dependentContexts) override {}

    bool OnEvent(const Event *event) override {
        return false;
    }

    void OnGetHandleClicked() {
        // HWND is a handle to a window
        const auto myHandle = glfw::native::getWin32Window(g_BasicContext->GetWindow()); // HWND for handle
        // we are running in cmd condition, so we call powershell and use command to find the process and return the window name
        // get the name string first
        std::wstring gameName;
        {
            auto proxy = m_GameName.GetProxy();
            gameName = std::wstring(proxy->begin(), proxy->end());
        }
        const std::wstring command =
                L"powershell -noprofile -command \"Get-Process | Where-Object { $_.MainWindowTitle -Like '*" + gameName
                + L"*' } | Select-Object -ExpandProperty MainWindowTitle\"";
        std::string resultTarget = exec(command);
        // std::cout << "result is:" << resultTarget << "<-" << std::endl;
        // remove all the \n\r\t which might influence the search for window
        size_t endStrings = resultTarget.find_last_not_of(" \n\r\t"); // find_last_not_of has internal loop and find the first one not included in the bracket NOLINT
        if (std::string::npos != endStrings) {
            resultTarget = resultTarget.substr(0, endStrings + 1);
        }

        std::cout << resultTarget << std::endl;

        // casting to LPCSTR according to doc
        const LPCSTR gameProcess = resultTarget.c_str();
        HWND target = FindWindowA(NULL, gameProcess); // NOLINT
        if (target == nullptr) {
            // QWidget::setWindowTitle("Easy Reverse : Error"); // NOLINT
            MessageBoxW(myHandle, L"Error! Could not find the process handle!", L"Error", MB_OK | MB_ICONERROR);
            return;
        }

        // when we got the handle, we can open process:
        // note the DWORD is a 32-bit unsigned integer used to store the process ID
        DWORD processID;
        GetWindowThreadProcessId(target, &processID);
        // check valid:
        if (processID == 0) {
            MessageBoxW(myHandle, L"Error! Invalid process ID!", L"Error", MB_OK | MB_ICONERROR);
            return;
        }

        // now we can have handle to kernel objects:
        HANDLE gameKernelProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processID); // NOLINT
        // check:
        if (gameKernelProcess == nullptr) {
            MessageBoxW(myHandle, L"Error! Invalid handle to kernel objects!", L"Error", MB_OK | MB_ICONERROR);
            return;
        }
        // if we can reach here, it means we get the correct process, update window for info
        // QString updateName = QString::fromStdString(resultTarget);
        // QWidget::setWindowTitle("Selected: "+ updateName); // NOLINT
        // store all necessary things in global varibles:
        gameWindowHandle = target;
        gameProcessID = processID;
        gameHandle = gameKernelProcess;
    }

public:
    Atomic<std::string> m_GameName;

    // global variables
    Atomic<HWND> gameWindowHandle = nullptr;
    Atomic<DWORD> gameProcessID = 0;
    Atomic<HANDLE> gameHandle = nullptr;

    // last read value as global variable
    Atomic<int> readValueInt = 0;
    Atomic<float> readValueFloat = 0.0;
    Atomic<double> readValueDouble = 0.0;
    Atomic<int> readValuePtr = 0; // this will be base 16
};
