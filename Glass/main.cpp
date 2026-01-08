#ifndef UNICODE
#define UNICODE
#endif 

#include <windows.h>
#include <string>

// --- CONTROLS IDs ---
#define ID_BUTTON_LAUNCH 1
#define ID_INPUT_BOX 2
#define ID_BUTTON_CLOSE 3  

// --- GLOBAL VARIABLES ---
HFONT hFont = NULL; 

//  Create a Modern Font 
HFONT CreateModernFont(int size) {
    return CreateFont(
        size, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
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
            390, 250, 500, 35, 
            hwnd, (HMENU)ID_INPUT_BOX, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL
        );
        SendMessage(hEdit, WM_SETFONT, (WPARAM)hFont, TRUE);

        HWND hButton = CreateWindow(
            L"BUTTON", L"LAUNCH COMMAND",
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

        if (id == ID_BUTTON_CLOSE) {
            PostQuitMessage(0);
        }

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
                MessageBox(hwnd, L"Error: Could not run cofee.exe", L"Error", MB_ICONERROR);
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
    return 0;
}