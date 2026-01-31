#ifndef UNICODE
#define UNICODE
#endif 


#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#pragma comment(lib, "urlmon.lib")
#pragma comment(lib, "dwmapi.lib") 

#include <windows.h>
#include <string>
#include <commctrl.h>
#include <shobjidl.h>
#include <urlmon.h>
#include <dwmapi.h>
#include <algorithm>
#include <vector>
#include <fstream>
#include <sstream>
#include <ctime>

#pragma message("resource.h IDI_ICON1 = " STRINGIZE(IDI_ICON1))
#include "resource.h"

#pragma comment(lib, "comctl32.lib")

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif



// CONTROLS IDs 
#define ID_BUTTON_LAUNCH 1
#define ID_INPUT_BOX 2 
#define ID_BUTTON_BROWSE 4
#define ID_PROGRESS_BAR 5
#define ID_STATUS_TEXT 6
#define ID_BUTTON_HISTORY 9
#define ID_LIST_HISTORY 10
#define ID_BUTTON_CLEAR 11
#define ID_DOWNLOAD_TIMER 100

HWND hProgressBar = NULL;
HWND hStatusText = NULL;

// --- GLOBAL VARIABLES ---
HFONT hFont = NULL; 
UINT_PTR downloadTimerID = 0;
int dotCount = 0;

// sruct to hold history items
struct HistoryItem {
    std::wstring timestamp;
    std::wstring path;
};

//  Create a Modern Font 
static HFONT CreateModernFont(int size) {
    return CreateFont(
        size, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
}

static bool OpenFolderDialog(HWND owner, wchar_t* buffer, int maxLen)
{
	IFileDialog* pFileDialog = NULL;
	HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFileDialog));
	if (SUCCEEDED(hr))
	{
		DWORD options;
		pFileDialog->GetOptions(&options);
		pFileDialog->SetOptions(options | FOS_PICKFOLDERS);
		hr = pFileDialog->Show(owner);
		if (SUCCEEDED(hr))
		{
			IShellItem* pItem;
			hr = pFileDialog->GetResult(&pItem);
			if (SUCCEEDED(hr))
			{
				PWSTR pszFilePath = NULL;
				hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
				if (SUCCEEDED(hr))
				{
					wcsncpy_s(buffer, maxLen, pszFilePath, _TRUNCATE);
					CoTaskMemFree(pszFilePath);
					pItem->Release();
					pFileDialog->Release();
					return true;
				}
				pItem->Release();
			}
		}
		pFileDialog->Release();
	}
	return false;
}

class DownloadCallback : public IBindStatusCallback
{
public:
    STDMETHOD(OnProgress)(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText)
    {
        if (ulProgressMax > 0 && hProgressBar)
        {
            int percent = (int)((ulProgress * 100) / ulProgressMax);
            SendMessage(hProgressBar, PBM_SETPOS, percent, 0);

            // Update status message based on progress
            if (percent < 30)
                SetWindowText(hStatusText, L"Cofee is brewing...");
            else if (percent < 70)
                SetWindowText(hStatusText, L"Almost ready...");
            else if (percent < 100)
                SetWindowText(hStatusText, L"Your cofee is ready. You're good to go!");
        }
        return S_OK;
    }

    // Required IBindStatusCallback methods
    STDMETHOD(OnStartBinding)(DWORD, IBinding*) { return S_OK; }
    STDMETHOD(GetBindInfo)(DWORD*, BINDINFO*) { return S_OK; }
    STDMETHOD(OnDataAvailable)(DWORD, DWORD, FORMATETC*, STGMEDIUM*) { return S_OK; }
    STDMETHOD(OnObjectAvailable)(REFIID, IUnknown*) { return S_OK; }
    STDMETHOD(GetPriority)(LONG*) { return S_OK; }
    STDMETHOD(OnLowResource)(DWORD) { return S_OK; }
    STDMETHOD(OnStopBinding)(HRESULT, LPCWSTR) { return S_OK; }
    STDMETHOD_(ULONG, AddRef)() { return 1; }
    STDMETHOD_(ULONG, Release)() { return 1; }
    STDMETHOD(QueryInterface)(REFIID riid, void** ppv)
    {
        if (riid == IID_IUnknown || riid == IID_IBindStatusCallback)
        {
            *ppv = this;
            return S_OK;
        }
        *ppv = NULL;
        return E_NOINTERFACE;
    }
};

class HistoryManager {
   private:
    const std::wstring dbFile = L"glass_history.db";

    std::wstring GetCurrentTimeStr() {
        time_t now = time(0);
        tm ltm;
        localtime_s(&ltm, &now);
        wchar_t buffer[32];
        wcsftime(buffer, 32, L"%Y-%m-%d %H:%M:%S", &ltm);
        return std::wstring(buffer);
    }

   public:
       void AddEntry(const std::wstring& path) {
           std::wofstream file(dbFile, std::ios::app);
           file << GetCurrentTimeStr() << L"|" << path << L"\n";
       }

       std::vector<HistoryItem> GetHistory(int limit) {
           std::vector<HistoryItem> items;
           std::wifstream file(dbFile);

           if (!file.is_open()) return items;

           std::wstring line;
           std::vector<std::wstring> allLines;

           while (std::getline(file, line)) {
               if (!line.empty()) allLines.push_back(line);
           }

           // Reverse
           std::reverse(allLines.begin(), allLines.end());

           // Parse up to 'limit'
           int count = 0;
           for (const auto& rawLine : allLines) {
               if (count >= limit && limit != -1) break;

               size_t delimPos = rawLine.find(L'|');
               if (delimPos != std::wstring::npos) {
                   HistoryItem item;
                   item.timestamp = rawLine.substr(0, delimPos);
                   item.path = rawLine.substr(delimPos + 1);
                   items.push_back(item);
                   count++;
               }
           }
           return items;
       }

       void Clear() {
           std::wofstream file(dbFile, std::ios::trunc); // Truncate (Wipe) file
       }
};

HistoryManager g_History;

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
    {

        hFont = CreateModernFont(22);

        HWND hEdit = CreateWindow(
            L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_CENTER, 
            390, 250, 350, 35,
            hwnd, (HMENU)ID_INPUT_BOX, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL
        );
        SendMessage(hEdit, WM_SETFONT, (WPARAM)hFont, TRUE);

        SendMessage(hEdit, EM_SETCUEBANNER, TRUE, (LPARAM)L"Enter command or path to scan...");

        // The Browse Button 
        HWND hBrowse = CreateWindow(
            L"BUTTON", L"...", 
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_FLAT,
            800, 250, 50, 35, 
            hwnd, (HMENU)ID_BUTTON_BROWSE, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL
        );
        SendMessage(hBrowse, WM_SETFONT, (WPARAM)hFont, TRUE);

        // the launch button
        HWND hButton = CreateWindow(
            L"BUTTON", L"LAUNCH COFEE",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON | BS_FLAT,
            500, 310, 280, 50,
            hwnd, (HMENU)ID_BUTTON_LAUNCH, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL
        );
        SendMessage(hButton, WM_SETFONT, (WPARAM)hFont, TRUE);

        // histry toggle button
        HWND hHistoryBtn = CreateWindow(
            L"BUTTON", L"⏱",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_FLAT,
            750, 250, 40, 35,
            hwnd, (HMENU)ID_BUTTON_HISTORY, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL
        );
        SendMessage(hHistoryBtn, WM_SETFONT, (WPARAM)hFont, TRUE);

        // the list button
        HWND hList = CreateWindow(
            L"LISTBOX", NULL,
            WS_CHILD | WS_BORDER | LBS_NOTIFY | WS_VSCROLL,
            390, 295, 460, 180, // below the input box here
            hwnd, (HMENU)ID_LIST_HISTORY, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL
        );
        SendMessage(hList, WM_SETFONT, (WPARAM)CreateModernFont(18), TRUE);

        hProgressBar = CreateWindowEx(
            0, PROGRESS_CLASS, NULL,
            WS_CHILD | PBS_SMOOTH,
            440, 400, 400, 30,
            hwnd, (HMENU)ID_PROGRESS_BAR,
            (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL
        );
        SendMessage(hProgressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));

        hStatusText = CreateWindow(
            L"STATIC", L"",
            WS_CHILD | SS_CENTER,
            440, 440, 400, 30,
            hwnd, (HMENU)ID_STATUS_TEXT,
            (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL
        );
        SendMessage(hStatusText, WM_SETFONT, (WPARAM)hFont, TRUE);
    }
    return 0;

    case WM_LBUTTONDOWN:
    {
        ReleaseCapture();
        SendMessage(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
    }
    return 0;

    case WM_SIZE:
    {
        int width = LOWORD(lParam);
        int height = HIWORD(lParam);

        if (width == 0 || height == 0) return 0;

        int btnW = 50;
        int btnH = 40;

        int centerX = width / 2;
        int centerY = height / 2;

        HWND hEdit = GetDlgItem(hwnd, ID_INPUT_BOX);
        MoveWindow(hEdit, centerX - 230, centerY - 50, 350, 35, TRUE);

        HWND hBrowse = GetDlgItem(hwnd, ID_BUTTON_BROWSE);
        MoveWindow(hBrowse, centerX + 180, centerY - 50, 50, 35, TRUE);

        HWND hLaunch = GetDlgItem(hwnd, ID_BUTTON_LAUNCH);
        MoveWindow(hLaunch, centerX - 140, centerY + 10, 280, 50, TRUE);

        HWND hHistoryBtn = GetDlgItem(hwnd, ID_BUTTON_HISTORY);
        MoveWindow(hHistoryBtn, centerX + 130, centerY - 50, 40, 35, TRUE);

        HWND hList = GetDlgItem(hwnd, ID_LIST_HISTORY);
        MoveWindow(hList, centerX - 230, centerY + 105, 460, 180, TRUE);

        if (hProgressBar) MoveWindow(hProgressBar, centerX - 200, centerY + 80, 400, 30, TRUE);
        if (hStatusText) MoveWindow(hStatusText, centerX - 200, centerY + 120, 400, 30, TRUE);

        InvalidateRect(hwnd, NULL, TRUE);
    }
    return 0;

    case WM_COMMAND:
    {
        int id = LOWORD(wParam);

        // browse button
        if (id == ID_BUTTON_BROWSE)
        {
            wchar_t folderPath[MAX_PATH];
            if (OpenFolderDialog(hwnd, folderPath, MAX_PATH))
            {
                SetDlgItemText(hwnd, ID_INPUT_BOX, folderPath);
            }
        }


        // launch button
        if (id == ID_BUTTON_LAUNCH)
        {
            wchar_t pathBuffer[512];
            GetDlgItemText(hwnd, ID_INPUT_BOX, pathBuffer, 512);

            // save to hostory
            if (wcslen(pathBuffer) > 0) {
                g_History.AddEntry(pathBuffer);
            }

            std::wstring command = L"cofee.exe ";
            if (wcslen(pathBuffer) > 0) {
                command += L"\""; command += pathBuffer; command += L"\"";
            }
            else { command += L"."; }
            command += L" -r -v";

            HANDLE hReadPipe, hWritePipe;
            SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
            CreatePipe(&hReadPipe, &hWritePipe, &sa, 0);

            STARTUPINFO si = { sizeof(STARTUPINFO) };
            si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
            si.hStdOutput = hWritePipe; si.hStdError = hWritePipe;
            si.wShowWindow = SW_HIDE;

            PROCESS_INFORMATION pi;

            // launch Cofee
            if (CreateProcess(NULL, &command[0], NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
            {
                CloseHandle(hWritePipe);
                char buffer[4096]; DWORD bytesRead; std::string output;
                while (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
                    buffer[bytesRead] = '\0'; output += buffer;
                }
                CloseHandle(hReadPipe);
                WaitForSingleObject(pi.hProcess, INFINITE);
                CloseHandle(pi.hProcess); CloseHandle(pi.hThread);

                int size = MultiByteToWideChar(CP_UTF8, 0, output.c_str(), -1, NULL, 0);
                wchar_t* wideOutput = new wchar_t[size];
                MultiByteToWideChar(CP_UTF8, 0, output.c_str(), -1, wideOutput, size);
                MessageBox(hwnd, wideOutput, L"COFEE Report", MB_OK);
                delete[] wideOutput;
            }
            else {
                DWORD error = GetLastError();

                CloseHandle(hReadPipe);
                CloseHandle(hWritePipe);

                

                if (error == ERROR_FILE_NOT_FOUND)
                {
                    // Build path to cofee.exe in the same directory as this .exe
                    wchar_t exePath[MAX_PATH];
                    GetModuleFileName(NULL, exePath, MAX_PATH);
                    wchar_t* lastSlash = wcsrchr(exePath, L'\\');
                    if (lastSlash) *(lastSlash + 1) = L'\0';

                    wchar_t cofeePath[MAX_PATH];
                    wcscpy_s(cofeePath, exePath);
                    wcscat_s(cofeePath, L"cofee.exe");

                    // Check if cofee.exe already exists
                    DWORD fileAttr = GetFileAttributes(cofeePath);
                    bool cofeeExists = (fileAttr != INVALID_FILE_ATTRIBUTES &&
                        !(fileAttr & FILE_ATTRIBUTE_DIRECTORY));

                    if (!cofeeExists)
                    {
                        // Ask User
                        int result = MessageBox(
                            hwnd,
                            L"Cofee is missing!\n\n"
                            L"We detected a Vault Server.\n"
                            L"Attempt to download from server?",
                            L"Dependency Check",
                            MB_YESNO | MB_ICONQUESTION
                        );

                        if (result == IDYES)
                        {
                            // Get exe directory
                            wchar_t exePath[MAX_PATH];
                            GetModuleFileName(NULL, exePath, MAX_PATH);
                            wchar_t* lastSlash = wcsrchr(exePath, L'\\');
                            if (lastSlash) *(lastSlash + 1) = L'\0';

                            wchar_t cofeePath[MAX_PATH];
                            wcscpy_s(cofeePath, exePath);
                            wcscat_s(cofeePath, L"cofee.exe");

                            ShowWindow(hProgressBar, SW_SHOW);
                            ShowWindow(hStatusText, SW_SHOW);

                            // START THE ANIMATION
                            dotCount = 0;
                            downloadTimerID = SetTimer(hwnd, ID_DOWNLOAD_TIMER, 500, NULL);
                            SetWindowText(hStatusText, L"☕ Starting download");

                            DownloadCallback callback;
                            HRESULT hr = URLDownloadToFile(
                                NULL,
                                L"https://operation-vault.onrender.com/download/cofee.exe",
                                cofeePath,
                                0,
                                &callback
                            );

                            // STOP THE ANIMATION
                            if (downloadTimerID) {
                                KillTimer(hwnd, ID_DOWNLOAD_TIMER);
                                downloadTimerID = 0;
                            }

                            ShowWindow(hProgressBar, SW_HIDE);
                            ShowWindow(hStatusText, SW_HIDE);
                            SendMessage(hProgressBar, PBM_SETPOS, 0, 0);

                            if (hr == S_OK)
                            {
                                MessageBox(hwnd, L"Download Complete!\nClick Launch again.", L"Success", MB_OK);
                            }
                            else
                            {
                                MessageBox(
                                    hwnd,
                                    L"Download Failed.\nBut don't fret. Give it another try.",
                                    L"Connection Error",
                                    MB_OK | MB_ICONERROR
                                );
                            }
                        }
                    }
                    else
                    {
                        MessageBox(hwnd,
                            L"Cofee.exe exists but failed to launch.\nCheck file permissions or antivirus.",
                            L"Launch Error",
                            MB_OK | MB_ICONERROR);
                    }
                }
                else
                {
					wchar_t errorMsg[256];
					wsprintf(errorMsg, L"Failed to launch Cofee.exe. Error code: %lu", error);
					MessageBox(hwnd, errorMsg, L"Error", MB_ICONERROR);
                }
            }
        }

		// history button
        if (id == ID_BUTTON_HISTORY)
        {
            HWND hList = GetDlgItem(hwnd, ID_LIST_HISTORY);

            if (IsWindowVisible(hList)) {
                ShowWindow(hList, SW_HIDE);
            }
            else {

                // clear current list
                SendMessage(hList, LB_RESETCONTENT, 0, 0);

                // fetch from DB
                std::vector<HistoryItem> history = g_History.GetHistory(20); // Last 20

                for (const auto& item : history)
                {
                    std::wstring display = item.timestamp + L" | " + item.path;
                    SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)display.c_str());
                }
                SetWindowPos(hList, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
            }
            
        }

        //history selection logic
        if (id == ID_LIST_HISTORY && HIWORD(wParam) == LBN_SELCHANGE)
        {
            HWND hList = GetDlgItem(hwnd, ID_LIST_HISTORY);

            int index = SendMessage(hList, LB_GETCURSEL, 0, 0);

            if (index != LB_ERR)
            {
                int len = SendMessage(hList, LB_GETTEXTLEN, index, 0);
                wchar_t* buffer = new wchar_t[len + 1];

                SendMessage(hList, LB_GETTEXT, index, (LPARAM)buffer);

                std::wstring fullText(buffer);
                size_t split = fullText.find(L'|');
                if (split != std::wstring::npos)
                {
                    std::wstring cleanPath = fullText.substr(split + 2); // +2 to skip "| "
                    SetDlgItemText(hwnd, ID_INPUT_BOX, cleanPath.c_str());

                    ShowWindow(hList, SW_HIDE);
                }

                delete[] buffer;
            }
        }
    }
    return 0;

  

   

    case WM_TIMER:
    {
        if (wParam == ID_DOWNLOAD_TIMER)
        {
            // cycle through 0, 1, 2, 3 dots
            dotCount = (dotCount + 1) % 4;

            wchar_t dots[5] = L"";
            for (int i = 0; i < dotCount; i++) {
                wcscat_s(dots, L".");
            }

            wchar_t statusMsg[100];
            wsprintf(statusMsg, L"☕ Starting download%s", dots);
            SetWindowText(hStatusText, statusMsg);
        }
    }
    return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT clientRect;
        GetClientRect(hwnd, &clientRect);

        HBRUSH brush = CreateSolidBrush(RGB(25, 25, 25));
        FillRect(hdc, &clientRect, brush);
        DeleteObject(brush);

        HFONT hTitleFont = CreateModernFont(36); 
        SelectObject(hdc, hTitleFont);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(240, 240, 240));

        RECT textRect = { 0, 100, 1280, 200 };
        DrawText(hdc, L"PROJECT GLASS", -1, &textRect, DT_CENTER | DT_SINGLELINE);

        DeleteObject(hTitleFont);
        EndPaint(hwnd, &ps);
    }
    return 0;

    case WM_CTLCOLORSTATIC:
    {
        HDC hdcStatic = (HDC)wParam;
        SetTextColor(hdcStatic, RGB(240, 240, 240));
        SetBkColor(hdcStatic, RGB(25, 25, 25));
        return (INT_PTR)CreateSolidBrush(RGB(25, 25, 25));
    }

    case WM_CTLCOLORLISTBOX:
    {
        HDC hdcList = (HDC)wParam;
        SetTextColor(hdcList, RGB(240, 240, 240)); 
        SetBkColor(hdcList, RGB(45, 45, 48)); 
        return (INT_PTR)CreateSolidBrush(RGB(45, 45, 48));
    }

    case WM_NCACTIVATE:
    {
        BOOL useDarkMode = TRUE;
        DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDarkMode, sizeof(useDarkMode));
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    case WM_DESTROY:
        DeleteObject(hFont); 
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    const wchar_t CLASS_NAME[] = L"GlassLauncherClass";
    

    WNDCLASS wc = { };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(25, 25, 25));
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0, CLASS_NAME, L"Project Glass",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1280, 720,
        NULL, NULL, hInstance, NULL
    );

    if (hwnd == NULL) return 0;

    BOOL useDarkMode = TRUE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDarkMode, sizeof(useDarkMode));

    HICON hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
    if (hIcon) {
        SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
        SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
    }

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    SetWindowPos(hwnd, NULL, (screenW - 1280) / 2, (screenH - 720) / 2, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

    ShowWindow(hwnd, nCmdShow);

    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    CoUninitialize();
    return 0;
}
