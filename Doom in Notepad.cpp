#include <opencv2/opencv.hpp>
#include <windows.h>
#include <iostream>
#include <vector>
#include <thread>
#include "proc.h"  // Ensure this header contains the FindProcessIdByName function

using namespace std;

// ASCII art dimensions
const int ASCII_WIDTH = 850;  // Output width in characters
const int ASCII_HEIGHT = 250; // Output height in characters
const int FPS = 30;           // Frames per second

// Convert pixel brightness to an ASCII character
wchar_t pixelToChar(int brightness) {
    if (brightness < 26) return L'@';   // Darkest
    if (brightness < 51) return L'#';
    if (brightness < 77) return L'%';
    if (brightness < 102) return L'&';
    if (brightness < 128) return L'+';
    if (brightness < 153) return L'*';
    if (brightness < 179) return L'-';
    if (brightness < 204) return L':';
    if (brightness < 230) return L'.';
    return L' ';                        // Brightest
}


// Capture a window's content as a cv::Mat
cv::Mat captureWindow(HWND hwnd) {
    RECT windowRect;
    GetWindowRect(hwnd, &windowRect);
    int windowWidth = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;

    HDC hWindowDC = GetDC(hwnd);
    HDC hCompatibleDC = CreateCompatibleDC(hWindowDC);
    HBITMAP hBitmap = CreateCompatibleBitmap(hWindowDC, windowWidth, windowHeight);
    SelectObject(hCompatibleDC, hBitmap);

    BitBlt(hCompatibleDC, 0, 0, windowWidth, windowHeight, hWindowDC, 0, 0, SRCCOPY);
    cv::Mat src(windowHeight, windowWidth, CV_8UC4);
    BITMAPINFOHEADER bi = { sizeof(BITMAPINFOHEADER), windowWidth, -windowHeight, 1, 32, BI_RGB, 0, 0, 0, 0, 0 };

    GetDIBits(hCompatibleDC, hBitmap, 0, windowHeight, src.data, (BITMAPINFO*)&bi, DIB_RGB_COLORS);

    DeleteObject(hBitmap);
    DeleteDC(hCompatibleDC);
    ReleaseDC(hwnd, hWindowDC);

    cv::Mat frame;
    cv::cvtColor(src, frame, cv::COLOR_BGRA2BGR); // Convert to BGR format
    return frame;
}

// Generate ASCII content from a resized grayscale frame
std::wstring generateASCIIContent(const cv::Mat& grayFrame) {
    std::wstring content;
    for (int y = 0; y < grayFrame.rows; ++y) {
        for (int x = 0; x < grayFrame.cols; ++x) {
            uchar intensity = grayFrame.at<uchar>(y, x);
            content += pixelToChar(intensity);
        }
        content += L'\n'; // Add a newline after each row
    }
    return content;
}

// Find the main window associated with a PID
HWND findMainWindow(DWORD pid) {
    HWND targetWindow = nullptr;

    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
        DWORD windowPid;
        GetWindowThreadProcessId(hwnd, &windowPid);

        if (windowPid == *(DWORD*)lParam && IsWindowVisible(hwnd)) {
            *(HWND*)lParam = hwnd;
            return FALSE; // Stop enumeration once found
        }
        return TRUE; // Continue enumeration
        }, (LPARAM)&pid);

    return targetWindow;
}


struct WindowInfo {
    DWORD pid;
    std::wstring title;
};

std::vector<WindowInfo> listAllWindows() {
    std::vector<WindowInfo> windows;

    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
        WCHAR windowTitle[256];
        DWORD pid;

        // Get the window title
        if (GetWindowTextW(hwnd, windowTitle, sizeof(windowTitle) / sizeof(WCHAR)) > 0) {
            // Get the process ID
            GetWindowThreadProcessId(hwnd, &pid);
            // Filter only visible windows with a title
            if (IsWindowVisible(hwnd)) {
                auto* windows = reinterpret_cast<std::vector<WindowInfo>*>(lParam);
                windows->push_back({ pid, windowTitle });
            }
        }
        return TRUE;
        }, reinterpret_cast<LPARAM>(&windows));

    return windows;
}

int main() {
    // Find the Notepad window and its edit control
    HWND notepadWindow = FindWindow(TEXT("Notepad"), nullptr);
    if (!notepadWindow) {
        std::cerr << "Error: No Notepad window found.\n";
        return 1;
    }
    HWND editControl = FindWindowEx(notepadWindow, nullptr, TEXT("Edit"), nullptr);
    if (!editControl) {
        std::cerr << "Error: Failed to find the edit control in Notepad.\n";
        return 1;
    }

    // Target application details 
    DWORD pid;
    cout << "Enter Process ID: ";
    cin >> pid;
    if (!pid) {
        cerr << "Error: Process not found.\n";
        return -1;
    }
    cout << "Process ID: " << pid << endl;
    LPCWSTR windowTitle = L"The Ultimate Doom - Chocolate Doom 3.1.0";

    HWND hwnd = FindWindowW(NULL, windowTitle);
    if (!hwnd)
    {
        cerr << "Error: Window not found." << endl;
        return -1;
    }

    // Main loop to capture and display ASCII art
    const int delay = 1000 / FPS; // Calculate delay for desired FPS
    while (true) {
        cv::Mat frame = captureWindow(hwnd);
        if (frame.empty()) {
            cerr << "Error: Failed to capture window content.\n";
            break;
        }

        // Resize and convert to grayscale
        cv::Mat resizedFrame, grayFrame;
        cv::resize(frame, resizedFrame, cv::Size(ASCII_WIDTH, ASCII_HEIGHT));
        cv::cvtColor(resizedFrame, grayFrame, cv::COLOR_BGR2GRAY);

        // Generate ASCII content and update Notepad
        std::wstring content = generateASCIIContent(grayFrame);
        SendMessageW(editControl, WM_SETTEXT, 0, (LPARAM)content.c_str());

        // Maintain FPS
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    }

    return 0;
}
