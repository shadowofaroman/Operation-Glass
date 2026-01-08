
#ifndef UNICODE
#define UNICODE
#endif 

#include <windows.h>  
#include <string>     

// Button identifiers
#define ID_BUTTON_LAUNCH 1
#define ID_INPUT_BOX 2


LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
    {

        CreateWindow(
            L"EDIT",                      
            L"",      
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 
            400, 250,    
            480, 30,        
            hwnd,
            (HMENU)ID_INPUT_BOX,       
            (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
            NULL
        );
        CreateWindow(
            L"BUTTON",                      
            L"Launch Cofee ☕",     
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            500, 300,                       // Position (X, Y)
            280, 80,                        // Size (Width, Height)
            hwnd,                           // Parent window
            (HMENU)ID_BUTTON_LAUNCH,        // Button ID 
            (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
            NULL
        );
    }
    return 0;

    case WM_COMMAND:
    {
        if (LOWORD(wParam) == ID_BUTTON_LAUNCH)
        {

            wchar_t pathBuffer[512];
            GetDlgItemText(hwnd, ID_INPUT_BOX, pathBuffer, 512);

            std::wstring command = L"cofee.exe ";

            if (wcslen(pathBuffer) > 0)
            {
				command += L"\"";
				command += pathBuffer;
				command += L"\" ";
            }
            else
            {
                // Default to current directory if box is empty
                command += L".";
            }

            command += L" -r -v";
            // Setup pipes to capture COFEE's console output
            HANDLE hReadPipe, hWritePipe;
            SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
            CreatePipe(&hReadPipe, &hWritePipe, &sa, 0);

            STARTUPINFO si = { sizeof(STARTUPINFO) };
            si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
            si.hStdOutput = hWritePipe;  // Redirect output to our pipe
            si.hStdError = hWritePipe;   // Redirect errors to our pipe
            si.wShowWindow = SW_HIDE;

            PROCESS_INFORMATION pi;
            // Command to execute: run COFEE on current directory with report and verbose flags
            wchar_t cmdLine[] = L"cofee.exe . -r -v";

            // Launch COFEE as a child process
            if (CreateProcess(NULL, &command[0], NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
            {
                CloseHandle(hWritePipe);

                // Read all output from COFEE
                char buffer[4096];
                DWORD bytesRead;
                std::string output;

                // Keep reading until the pipe is empty
                while (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0)
                {
                    buffer[bytesRead] = '\0';
                    output += buffer;
                }

                // Clean up handles
                CloseHandle(hReadPipe);
                WaitForSingleObject(pi.hProcess, INFINITE);  // Wait for COFEE to finish
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);

                // Convert output from UTF-8 to Unicode for display
                int size = MultiByteToWideChar(CP_UTF8, 0, output.c_str(), -1, NULL, 0);
                wchar_t* wideOutput = new wchar_t[size];
                MultiByteToWideChar(CP_UTF8, 0, output.c_str(), -1, wideOutput, size);

                // Show results in a message box
                MessageBox(hwnd, wideOutput, L"COFEE Results", MB_OK);
                delete[] wideOutput;
            }
            else
            {
                MessageBox(hwnd, L"Failed to start Cofee.exe! Make sure it is in the PATH or same folder.", L"Error", MB_ICONERROR);
            }

        }
    }
    return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        HBRUSH brush = CreateSolidBrush(RGB(30, 30, 30));
        FillRect(hdc, &ps.rcPaint, brush);
        DeleteObject(brush); 

        SetBkMode(hdc, TRANSPARENT);  
        SetTextColor(hdc, RGB(255, 255, 255)); 

        RECT textRect = { 100, 100, 1180, 200 };
        DrawText(hdc, L"GLASS LAUNCHER", -1, &textRect,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        EndPaint(hwnd, &ps);
    }
    return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// Application entry point
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    PWSTR pCmdLine, int nCmdShow)
{
    const wchar_t CLASS_NAME[] = L"GlassLauncherClass";

    WNDCLASS wc = { };
    wc.lpfnWndProc = WindowProc; 
    wc.hInstance = hInstance;  
    wc.lpszClassName = CLASS_NAME;   
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);  

    RegisterClass(&wc);

    // Create the main window
    HWND hwnd = CreateWindowEx(
        0,                            
        CLASS_NAME,                  
        L"Glass. The Cofee Launcher",  
        WS_OVERLAPPEDWINDOW,        
        CW_USEDEFAULT, CW_USEDEFAULT, 
        1280, 720,                  
        NULL, NULL, hInstance, NULL
    );

    // Exit if window creation failed
    if (hwnd == NULL) return 0;

    // Make the window visible
    ShowWindow(hwnd, nCmdShow);

    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg); 
        DispatchMessage(&msg);   // Send message to WindowProc
    }

    return 0;
}