#ifndef UNICODE
#define UNICODE
#endif 


#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#pragma comment(lib, "urlmon.lib")

#include <windows.h>
#include <string>
#include <commctrl.h>
#include <shobjidl.h>
#include <urlmon.h>

#pragma comment(lib, "comctl32.lib")

// --- CONTROLS IDs ---
#define ID_BUTTON_LAUNCH 1
#define ID_INPUT_BOX 2
#define ID_BUTTON_CLOSE 3  
#define ID_BUTTON_BROWSE 4

// --- GLOBAL VARIABLES ---
HFONT hFont = NULL; 

//  Create a Modern Font 
HFONT CreateModernFont(int size) {
    return CreateFont(
        size, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
}

bool OpenFolderDialog(HWND owner, wchar_t* buffer, int maxLen)
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

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
    {

        hFont = CreateModernFont(22);

        HWND hEdit = CreateWindow(
            L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_CENTER, 
            390, 250, 400, 35, 
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

        // The Launch Button
        HWND hButton = CreateWindow(
            L"BUTTON", L"LAUNCH COFEE",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON | BS_FLAT,
            500, 310, 280, 50,
            hwnd, (HMENU)ID_BUTTON_LAUNCH, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL
        );
        SendMessage(hButton, WM_SETFONT, (WPARAM)hFont, TRUE);

        HWND hClose = CreateWindow(
            L"BUTTON", L"X",
            WS_VISIBLE | WS_CHILD | BS_FLAT,
            1230, 0, 50, 40, 
            hwnd, (HMENU)ID_BUTTON_CLOSE, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL
        );
        SendMessage(hClose, WM_SETFONT, (WPARAM)hFont, TRUE);
    }
    return 0;

    case WM_LBUTTONDOWN:
    {
        ReleaseCapture();
        SendMessage(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
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

        // close button
        if (id == ID_BUTTON_CLOSE) {
            PostQuitMessage(0);
        }

        // launch button
        if (id == ID_BUTTON_LAUNCH)
        {
            wchar_t pathBuffer[512];
            GetDlgItemText(hwnd, ID_INPUT_BOX, pathBuffer, 512);

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
                    // Ask User
                    int result = MessageBox(
                        hwnd,
                        L"Cofee.exe is missing!\n\n"
                        L"We detected a local Vault Server.\n"
                        L"Attempt to download from http://localhost:3000?",
                        L"Dependency Check",
                        MB_YESNO | MB_ICONQUESTION
                    );

                    if (result == IDYES)
                    {
                        // the Native Download
                        // HRESULT URLDownloadToFile(caller, url, filename, reserved, callback)
                        HRESULT hr = URLDownloadToFile(
                            NULL,
                            L"https://operation-vault.onrender.com/download/cofee.exe",
                            L"cofee.exe",                                 
                            0,
                            NULL
                        );

                        if (hr == S_OK)
                        {
                            MessageBox(hwnd, L"Download Complete!\nClick Launch again.", L"Success", MB_OK);
                        }
                        else
                        {
                            MessageBox(
                                hwnd,
                                L"Download Failed.\nIs 'Operation-Vault' running on Port 3000?",
                                L"Connection Error",
                                MB_OK | MB_ICONERROR
                            );
                        }
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
    }
    return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        HBRUSH brush = CreateSolidBrush(RGB(25, 25, 25));
        FillRect(hdc, &ps.rcPaint, brush);
        DeleteObject(brush);


        RECT headerRect = { 0, 0, 1280, 40 };
        HBRUSH headerBrush = CreateSolidBrush(RGB(45, 45, 48)); 
        FillRect(hdc, &headerRect, headerBrush);
        DeleteObject(headerBrush);

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
    // Dark background brush to prevent flickering
    wc.hbrBackground = CreateSolidBrush(RGB(25, 25, 25));

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0, CLASS_NAME, L"Project Glass",
        WS_POPUP | WS_VISIBLE, 
        CW_USEDEFAULT, CW_USEDEFAULT,
        1280, 720,
        NULL, NULL, hInstance, NULL
    );

    if (hwnd == NULL) return 0;

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