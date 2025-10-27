// SnapKey 1.2.9
// github.com/cafali/SnapKey

#include <windows.h>
#include <shellapi.h>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <regex>
#include <vector>
#include <filesystem>
#include <thread>
#include <chrono>

using namespace std;
namespace fs = std::filesystem;
using namespace chrono_literals;

#define ID_TRAY_APP_ICON                1001
#define ID_TRAY_EXIT_CONTEXT_MENU_ITEM  3000
#define ID_TRAY_VERSION_INFO            3001
#define ID_TRAY_REBIND_KEYS             3002
#define ID_TRAY_LOCK_FUNCTION           3003
#define ID_TRAY_RESTART_SNAPKEY         3004
#define ID_TRAY_HELP                    3005
#define ID_TRAY_CHECKUPDATE             3006
#define ID_TRAY_LAYOUTS                 3007
#define WM_TRAYICON                     (WM_USER + 1)
#define ID_LAYOUT_BASE                  4000 // 1.2.9

struct KeyState {
    bool registered = false;
    bool keyDown = false;
    int group;
    bool simulated = false;
};

struct GroupState {
    int previousKey;
    int activeKey;
};

unordered_map<int, GroupState> GroupInfo;
unordered_map<int, KeyState> KeyInfo;

HHOOK hHook = NULL;
HANDLE hMutex = NULL;
NOTIFYICONDATA nid;
bool isLocked = false;

// Forward declarations
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void InitNotifyIconData(HWND hwnd);
bool LoadConfig(const std::wstring& filename);
void CreateDefaultConfig(const std::wstring& filename);
void RestoreConfigFromBackup(const std::wstring& backupFilename, const std::wstring& destinationFilename);
std::wstring GetVersionInfo();
void SendKey(int target, bool keyDown);

// select layout via context menu v 1.2.9
vector<string> ListLayouts() {
    vector<string> layouts;
    string path = "meta\\profiles";
    if (!fs::exists(path)) return layouts;

    for (auto& entry : fs::directory_iterator(path)) {
        if (entry.is_regular_file()) {
            auto ext = entry.path().extension().string();
            if (ext == ".cfg") {
                layouts.push_back(entry.path().stem().string()); // ignore file extension
            }
        }
    }
    return layouts;
}

// apply layout (replace config.cfg content) v 1.2.9
void ApplyLayout(const string& layoutName) {
    string sourcePath = "meta\\profiles\\" + layoutName + ".cfg";
    string destPath = "config.cfg";

    ifstream src(sourcePath, ios::binary);
    ofstream dst(destPath, ios::binary | ios::trunc);

    if (!src.is_open() || !dst.is_open()) {
        MessageBox(NULL, L"Failed to apply layout. Please check the layout file.",
                   L"SnapKey Error", MB_ICONERROR | MB_OK);
        return;
    }

    dst << src.rdbuf(); // copy file contents
}

// restart
void RestartSnapKey() {
    TCHAR szExeFileName[MAX_PATH];
    GetModuleFileName(NULL, szExeFileName, MAX_PATH);
    ShellExecute(NULL, NULL, szExeFileName, NULL, NULL, SW_SHOWNORMAL);
    PostQuitMessage(0);
}

// Main entry
int main() {
    if (!LoadConfig(L"config.cfg")) {
        return 1;
    }

    hMutex = CreateMutex(NULL, TRUE, TEXT("SnapKeyMutex"));
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        MessageBox(NULL, L"ثوك تاب يعمل بالخلفية", L"Thock Tap", MB_ICONINFORMATION | MB_OK);
        return 1;
    }

    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = TEXT("SnapKeyClass");

    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, L"Window Registration Failed!", L"Error", MB_ICONEXCLAMATION | MB_OK);
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
        return 1;
    }

    HWND hwnd = CreateWindowEx(0, wc.lpszClassName, TEXT("Thock Tap"), WS_OVERLAPPEDWINDOW,
                               CW_USEDEFAULT, CW_USEDEFAULT, 240, 120,
                               NULL, NULL, wc.hInstance, NULL);

    if (hwnd == NULL) {
        MessageBox(NULL, L"Window Creation Failed!", L"Error", MB_ICONEXCLAMATION | MB_OK);
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
        return 1;
    }

    InitNotifyIconData(hwnd);

    hHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);
    if (hHook == NULL) {
        MessageBox(NULL, L"Failed to install hook!", L"Error", MB_ICONEXCLAMATION | MB_OK);
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
        return 1;
    }

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(hHook);
    Shell_NotifyIcon(NIM_DELETE, &nid);
    ReleaseMutex(hMutex);
    CloseHandle(hMutex);

    return 0;
}

// Key handling
void handleKeyDown(int keyCode) {
    KeyState& currentKeyInfo = KeyInfo[keyCode];
    GroupState& currentGroupInfo = GroupInfo[currentKeyInfo.group];
    if (!currentKeyInfo.keyDown) {
        currentKeyInfo.keyDown = true;
        SendKey(keyCode, true);
        if (currentGroupInfo.activeKey == 0 || currentGroupInfo.activeKey == keyCode) {
            currentGroupInfo.activeKey = keyCode;
        } else {
            currentGroupInfo.previousKey = currentGroupInfo.activeKey;
            currentGroupInfo.activeKey = keyCode;
            SendKey(currentGroupInfo.previousKey, false);
        }
    }
}

void handleKeyUp(int keyCode) {
    KeyState& currentKeyInfo = KeyInfo[keyCode];
    GroupState& currentGroupInfo = GroupInfo[currentKeyInfo.group];
    if (currentGroupInfo.previousKey == keyCode && !currentKeyInfo.keyDown) {
        currentGroupInfo.previousKey = 0;
    }
    if (currentKeyInfo.keyDown) {
        currentKeyInfo.keyDown = false;
        if (currentGroupInfo.activeKey == keyCode && currentGroupInfo.previousKey != 0) {
            SendKey(keyCode, false);
            currentGroupInfo.activeKey = currentGroupInfo.previousKey;
            currentGroupInfo.previousKey = 0;
            SendKey(currentGroupInfo.activeKey, true);
        } else {
            currentGroupInfo.previousKey = 0;
            if (currentGroupInfo.activeKey == keyCode) currentGroupInfo.activeKey = 0;
            SendKey(keyCode, false);
        }
    }
}

bool isSimulatedKeyEvent(DWORD flags) { return flags & 0x10; }

void SendKey(int targetKey, bool keyDown) {
    INPUT input = {0};
    input.ki.wVk = targetKey;
    input.ki.wScan = MapVirtualKey(targetKey, 0);
    input.type = INPUT_KEYBOARD;

    DWORD flags = KEYEVENTF_SCANCODE;
    input.ki.dwFlags = keyDown ? flags : flags | KEYEVENTF_KEYUP;
    int randNum = rand()%(7-1 + 1) + 1;
    std::this_thread::sleep_for(std::chrono::milliseconds(randNum));
    SendInput(1, &input, sizeof(INPUT));
}

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (!isLocked && nCode >= 0) {
        KBDLLHOOKSTRUCT *pKeyBoard = (KBDLLHOOKSTRUCT *)lParam;
        if (!isSimulatedKeyEvent(pKeyBoard->flags)) {
            if (KeyInfo[pKeyBoard->vkCode].registered) {
                if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) handleKeyDown(pKeyBoard->vkCode);
                if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) handleKeyUp(pKeyBoard->vkCode);
                return 1;
            }
        }
    }
    return CallNextHookEx(hHook, nCode, wParam, lParam);
}

void InitNotifyIconData(HWND hwnd) {
    memset(&nid, 0, sizeof(NOTIFYICONDATA));
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = ID_TRAY_APP_ICON;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;

    HICON hIcon = (HICON)LoadImage(NULL, TEXT("icon.ico"), IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
    nid.hIcon = hIcon ? hIcon : LoadIcon(NULL, IDI_APPLICATION);
    lstrcpy(nid.szTip, L"ثوك تاب");
    Shell_NotifyIcon(NIM_ADD, &nid);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_TRAYICON:
        if (lParam == WM_RBUTTONDOWN) {
            POINT curPoint;
            GetCursorPos(&curPoint);
            SetForegroundWindow(hwnd);

            HMENU hMenu = CreatePopupMenu();
            // AppendMenu(hMenu, MF_STRING, ID_TRAY_REBIND_KEYS, TEXT("Rebind Keys"));

            // submenu layouts
            HMENU hSubMenu = CreatePopupMenu();
            vector<string> layouts = ListLayouts();
            if (!layouts.empty()) {
                int id = 0;
                for (auto& layout : layouts) {
                    AppendMenuA(hSubMenu, MF_STRING, ID_LAYOUT_BASE + id, layout.c_str());
                    id++;
                }
            } else {
                AppendMenu(hSubMenu, MF_GRAYED, 0, TEXT("No layouts found"));
            }
            AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hSubMenu, L"اختيار البروفال");

            AppendMenu(hMenu, MF_STRING, ID_TRAY_RESTART_SNAPKEY, L"اعادة تشغيل ثوك تاب");
            AppendMenu(hMenu, MF_STRING, ID_TRAY_LOCK_FUNCTION, isLocked ? L"تفعيل ثوك تاب" : L"تعطيل ثوك تاب");
            // AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
            // AppendMenu(hMenu, MF_STRING, ID_TRAY_HELP, L"المساعدة");
            AppendMenu(hMenu, MF_STRING, ID_TRAY_CHECKUPDATE, L"متجر ثوك");
            // AppendMenu(hMenu, MF_STRING, ID_TRAY_VERSION_INFO, L"Version Info (1.2.9)");
            AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
            AppendMenu(hMenu, MF_STRING, ID_TRAY_EXIT_CONTEXT_MENU_ITEM, L"خروج");

            TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, curPoint.x, curPoint.y, 0, hwnd, NULL);
            DestroyMenu(hMenu);
        }
        else if (lParam == WM_LBUTTONDBLCLK) {
            isLocked = !isLocked;
            HICON hIcon = isLocked
                ? (HICON)LoadImage(NULL, TEXT("icon_off.ico"), IMAGE_ICON, 0, 0, LR_LOADFROMFILE)
                : (HICON)LoadImage(NULL, TEXT("icon.ico"), IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
            if (hIcon) {
                nid.hIcon = hIcon;
                Shell_NotifyIcon(NIM_MODIFY, &nid);
                DestroyIcon(hIcon);
            }
        }
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) >= ID_LAYOUT_BASE) {
            int layoutIndex = LOWORD(wParam) - ID_LAYOUT_BASE;
            vector<string> layouts = ListLayouts();
            if (layoutIndex >= 0 && layoutIndex < (int)layouts.size()) {
                ApplyLayout(layouts[layoutIndex]);
                RestartSnapKey(); // restart after applying layout
            }
        }
        else {
            switch (LOWORD(wParam)) {
            case ID_TRAY_EXIT_CONTEXT_MENU_ITEM:
                PostQuitMessage(0);
                break;
            case ID_TRAY_VERSION_INFO:
                MessageBox(hwnd, GetVersionInfo().c_str(), L"SnapKey Version Info", MB_OK);
                break;
            case ID_TRAY_REBIND_KEYS:
                ShellExecute(NULL, L"open", L"config.cfg", NULL, NULL, SW_SHOWNORMAL);
                break;
            case ID_TRAY_HELP:
                ShellExecute(NULL, L"open", L"README.pdf", NULL, NULL, SW_SHOWNORMAL);
                break;
            case ID_TRAY_CHECKUPDATE:
                if (MessageBox(NULL,
                               L"متجر ثوك متخصص في صناعة و بيع افخم الكبيوردات. المتابعة للمتجر؟",
                               L"Thock",
                               MB_YESNO | MB_ICONQUESTION) == IDYES) {
                    ShellExecute(NULL, L"open", L"https://thock.sa", NULL, NULL, SW_SHOWNORMAL);
                }
                break;
            case ID_TRAY_RESTART_SNAPKEY:
                RestartSnapKey();
                break;
            case ID_TRAY_LOCK_FUNCTION:
                isLocked = !isLocked;
                {
                    HICON hIcon = isLocked
                        ? (HICON)LoadImage(NULL, TEXT("icon_off.ico"), IMAGE_ICON, 0, 0, LR_LOADFROMFILE)
                        : (HICON)LoadImage(NULL, TEXT("icon.ico"), IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
                    if (hIcon) {
                        nid.hIcon = hIcon;
                        Shell_NotifyIcon(NIM_MODIFY, &nid);
                        DestroyIcon(hIcon);
                    }
                }
                break;
            }
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

std::wstring GetVersionInfo() {
    return L"SnapKey v1.2.9 (R18)\n"
           L"Version Date: August 8, 2025\n"
           L"Repository: github.com/cafali/SnapKey\n"
           L"License: MIT License\n";
}

void RestoreConfigFromBackup(const std::wstring& backupFilename, const std::wstring& destinationFilename) {
    std::wstring sourcePath = L"meta\\" + backupFilename;
    std::wstring destinationPath = destinationFilename;

    if (CopyFile(sourcePath.c_str(), destinationPath.c_str(), FALSE)) {
        MessageBox(NULL, L"Default config restored from backup successfully.", L"Thock tap", MB_ICONINFORMATION | MB_OK);
    } else {
        MessageBox(NULL, L"Failed to restore config from backup.", L"Thock tap Error", MB_ICONERROR | MB_OK);
    }
}

void CreateDefaultConfig(const std::wstring& filename) {
    RestoreConfigFromBackup(L"backup.thocktap", filename);
}

bool LoadConfig(const std::wstring& filename) {
    std::ifstream configFile(filename);
    if (!configFile.is_open()) {
        CreateDefaultConfig(filename);
        return false;
    }

    string line;
    int id = 0;
    while (getline(configFile, line)) {
        istringstream iss(line);
        string key;
        int value;
        regex secPat(R"(\s*\[Group\]\s*)");
        if (regex_match(line, secPat)) {
            id++;
        } else if (getline(iss, key, '=') && (iss >> value)) {
            if (key.find("key") != string::npos) {
                if (!KeyInfo[value].registered) {
                    KeyInfo[value].registered = true;
                    KeyInfo[value].group = id;
                } else {
                    MessageBox(NULL,
                               L"The config file contains duplicate keys. Please review the setup.",
                               L"Thock tap Error", MB_ICONEXCLAMATION | MB_OK);
                    return false;
                }
            }
        }
    }
    return true;
}
