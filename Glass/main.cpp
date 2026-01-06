// Enable Unicode support for Windows API
#ifndef UNICODE
#define UNICODE
#endif 

#include <windows.h>  // Core Windows API functions
#include <string>     // For std::string (text manipulation)

// Button identifier - used to detect which button was clicked
#define ID_BUTTON_LAUNCH 1

// Main window event handler
// Windows calls this function whenever something happens to our window
// (clicks, painting, closing, etc.)
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
        // Window is being created - this is where we add our UI controls
    {
        // Create the main launch button
        CreateWindow(
            L"BUTTON",                      // Control type
            L"Launch Cofee ☕",              // Button text with coffee emoji
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            500, 300,                       // Position (X, Y)
            280, 80,                        // Size (Width, Height)
            hwnd,                           // Parent window
            (HMENU)ID_BUTTON_LAUNCH,        // Button ID for identification
            (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
            NULL
        );
    }
    return 0;

    case WM_COMMAND:
        // A button or menu item was clicked
    {
        if (LOWORD(wParam) == ID_BUTTON_LAUNCH)
        {
            // Setup pipes to capture COFEE's console output
            HANDLE hReadPipe, hWritePipe;
            SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
            CreatePipe(&hReadPipe, &hWritePipe, &sa, 0);

            // Configure the process to run hidden (no console window)
            STARTUPINFO si = { sizeof(STARTUPINFO) };
            si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
            si.hStdOutput = hWritePipe;  // Redirect output to our pipe
            si.hStdError = hWritePipe;   // Redirect errors to our pipe
            si.wShowWindow = SW_HIDE;    // Don't show console window

            PROCESS_INFORMATION pi;
            // Command to execute: run COFEE on current directory with report and verbose flags
            wchar_t cmdLine[] = L"cofee.exe . -r -v";

            // Launch COFEE as a child process
            if (CreateProcess(NULL, cmdLine, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
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
        }
    }
    return 0;

    case WM_DESTROY:
        // User closed the window - exit the application
        PostQuitMessage(0);
        return 0;

    case WM_PAINT:
        // Windows needs us to redraw the window
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        // Fill window with dark gray background
        HBRUSH brush = CreateSolidBrush(RGB(30, 30, 30));
        FillRect(hdc, &ps.rcPaint, brush);
        DeleteObject(brush);  // Clean up to avoid memory leak

        // Draw title text in white
        SetBkMode(hdc, TRANSPARENT);           // No background behind text
        SetTextColor(hdc, RGB(255, 255, 255)); // White text color

        RECT textRect = { 100, 100, 1180, 200 };
        DrawText(hdc, L"PROJECT GLASS LAUNCHER", -1, &textRect,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        EndPaint(hwnd, &ps);
    }
    return 0;
    }

    // Let Windows handle any messages we don't care about
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// Application entry point
// This is where the program starts executing
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    PWSTR pCmdLine, int nCmdShow)
{
    // Register our window class (the blueprint for our window)
    const wchar_t CLASS_NAME[] = L"GlassLauncherClass";

    WNDCLASS wc = { };
    wc.lpfnWndProc = WindowProc;      // Connect our event handler
    wc.hInstance = hInstance;         // This application instance
    wc.lpszClassName = CLASS_NAME;    // Window class name
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);  // Use standard arrow cursor

    RegisterClass(&wc);

    // Create the main window
    HWND hwnd = CreateWindowEx(
        0,                            // No extended styles
        CLASS_NAME,                   // Use our registered class
        L"Project Glass - Launcher",  // Window title
        WS_OVERLAPPEDWINDOW,          // Standard window style
        CW_USEDEFAULT, CW_USEDEFAULT, // Let Windows choose position
        1280, 720,                    // Window size
        NULL, NULL, hInstance, NULL
    );

    // Exit if window creation failed
    if (hwnd == NULL) return 0;

    // Make the window visible
    ShowWindow(hwnd, nCmdShow);

    // Message loop - keeps the application running
    // Processes all events (clicks, keyboard, etc.)
    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);  // Translate keyboard input
        DispatchMessage(&msg);   // Send message to WindowProc
    }

    return 0;
}