// AwakeFox - Keep your computer awake
// Copyright (C) 2025 JustFox
//
// A lightweight Windows utility that prevents your computer from going to sleep
// by periodically moving the mouse cursor by a minimal amount.

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <commctrl.h>
#include <string>
#include <chrono>
#include <cstdio>

#include "resource.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// Global variables
HINSTANCE g_hInstance = nullptr;
HWND g_hMainDlg = nullptr;
NOTIFYICONDATA g_nid = {};
bool g_isRunning = false;
bool g_minimizeToTray = true;
int g_intervalSeconds = 60;
int g_moveCount = 0;
std::chrono::steady_clock::time_point g_startTime;

// Registry key for settings
const wchar_t* REG_KEY = L"Software\\JustFox\\AwakeFox";

// Forward declarations
INT_PTR CALLBACK MainDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
void StartMovement(HWND hDlg);
void StopMovement(HWND hDlg);
void MoveMouse();
void UpdateUI(HWND hDlg);
void AddTrayIcon(HWND hDlg);
void RemoveTrayIcon();
void UpdateTrayIcon(bool active);
void ShowTrayMenu(HWND hDlg);
void MinimizeToTray(HWND hDlg);
void RestoreFromTray(HWND hDlg);
void LoadSettings();
void SaveSettings();
std::wstring FormatDuration(int totalSeconds);

// Entry point
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                      _In_opt_ HINSTANCE hPrevInstance,
                      _In_ LPWSTR lpCmdLine,
                      _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    g_hInstance = hInstance;

    // Initialize common controls
    INITCOMMONCONTROLSEX icc = {};
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icc);

    // Load settings
    LoadSettings();

    // Create main dialog
    DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAIN), nullptr, MainDlgProc);

    return 0;
}

// Main dialog procedure
INT_PTR CALLBACK MainDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
    {
        g_hMainDlg = hDlg;

        // Set icon
        HICON hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_AWAKEFOX));
        SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
        SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

        // Initialize slider (10 to 300 seconds)
        HWND hSlider = GetDlgItem(hDlg, IDC_SLIDER_INTERVAL);
        SendMessage(hSlider, TBM_SETRANGE, TRUE, MAKELPARAM(10, 300));
        SendMessage(hSlider, TBM_SETTICFREQ, 30, 0);
        SendMessage(hSlider, TBM_SETPOS, TRUE, g_intervalSeconds);

        // Update interval label
        wchar_t buf[64];
        swprintf_s(buf, L"Interval: %d seconds", g_intervalSeconds);
        SetDlgItemText(hDlg, IDC_STATIC_INTERVAL, buf);

        // Set checkbox
        CheckDlgButton(hDlg, IDC_CHECK_MINIMIZE, g_minimizeToTray ? BST_CHECKED : BST_UNCHECKED);

        // Add tray icon
        AddTrayIcon(hDlg);

        // Start UI update timer (every second)
        SetTimer(hDlg, IDT_UPDATE_UI, 1000, nullptr);

        return TRUE;
    }

    case WM_HSCROLL:
    {
        if ((HWND)lParam == GetDlgItem(hDlg, IDC_SLIDER_INTERVAL))
        {
            g_intervalSeconds = (int)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);

            wchar_t buf[64];
            swprintf_s(buf, L"Interval: %d seconds", g_intervalSeconds);
            SetDlgItemText(hDlg, IDC_STATIC_INTERVAL, buf);

            // Update timer if running
            if (g_isRunning)
            {
                KillTimer(hDlg, IDT_MOUSE_MOVE);
                SetTimer(hDlg, IDT_MOUSE_MOVE, g_intervalSeconds * 1000, nullptr);
            }

            SaveSettings();
        }
        return TRUE;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_BTN_STARTSTOP:
            if (g_isRunning)
                StopMovement(hDlg);
            else
                StartMovement(hDlg);
            return TRUE;

        case IDC_CHECK_MINIMIZE:
            g_minimizeToTray = IsDlgButtonChecked(hDlg, IDC_CHECK_MINIMIZE) == BST_CHECKED;
            SaveSettings();
            return TRUE;

        case IDM_TRAY_SHOW:
            RestoreFromTray(hDlg);
            return TRUE;

        case IDM_TRAY_STARTSTOP:
            if (g_isRunning)
                StopMovement(hDlg);
            else
                StartMovement(hDlg);
            return TRUE;

        case IDM_TRAY_EXIT:
            StopMovement(hDlg);
            RemoveTrayIcon();
            EndDialog(hDlg, 0);
            return TRUE;

        case IDCANCEL:
            if (g_minimizeToTray && g_isRunning)
            {
                MinimizeToTray(hDlg);
            }
            else
            {
                StopMovement(hDlg);
                RemoveTrayIcon();
                EndDialog(hDlg, 0);
            }
            return TRUE;
        }
        break;

    case WM_TIMER:
        switch (wParam)
        {
        case IDT_MOUSE_MOVE:
            MoveMouse();
            g_moveCount++;
            UpdateUI(hDlg);
            return TRUE;

        case IDT_UPDATE_UI:
            if (g_isRunning)
                UpdateUI(hDlg);
            return TRUE;
        }
        break;

    case WM_TRAYICON:
        switch (lParam)
        {
        case WM_LBUTTONDBLCLK:
            RestoreFromTray(hDlg);
            return TRUE;

        case WM_RBUTTONUP:
            ShowTrayMenu(hDlg);
            return TRUE;
        }
        break;

    case WM_SYSCOMMAND:
        if (wParam == SC_MINIMIZE && g_minimizeToTray)
        {
            MinimizeToTray(hDlg);
            return TRUE;
        }
        break;

    case WM_CLOSE:
        if (g_minimizeToTray && g_isRunning)
        {
            MinimizeToTray(hDlg);
            return TRUE;
        }
        else
        {
            StopMovement(hDlg);
            RemoveTrayIcon();
            EndDialog(hDlg, 0);
            return TRUE;
        }
        break;

    case WM_DESTROY:
        KillTimer(hDlg, IDT_UPDATE_UI);
        KillTimer(hDlg, IDT_MOUSE_MOVE);
        RemoveTrayIcon();
        break;
    }

    return FALSE;
}

void StartMovement(HWND hDlg)
{
    g_isRunning = true;
    g_moveCount = 0;
    g_startTime = std::chrono::steady_clock::now();

    // Start mouse movement timer
    SetTimer(hDlg, IDT_MOUSE_MOVE, g_intervalSeconds * 1000, nullptr);

    // Update UI
    SetDlgItemText(hDlg, IDC_BTN_STARTSTOP, L"Stop");
    SetDlgItemText(hDlg, IDC_STATIC_STATUS, L"Running - Keeping computer awake");

    // Update tray icon
    UpdateTrayIcon(true);

    // Do first movement immediately
    MoveMouse();
    g_moveCount++;
    UpdateUI(hDlg);
}

void StopMovement(HWND hDlg)
{
    g_isRunning = false;

    // Stop timer
    KillTimer(hDlg, IDT_MOUSE_MOVE);

    // Update UI
    SetDlgItemText(hDlg, IDC_BTN_STARTSTOP, L"Start");
    SetDlgItemText(hDlg, IDC_STATIC_STATUS, L"Stopped");

    // Update tray icon
    UpdateTrayIcon(false);
}

void MoveMouse()
{
    // Move mouse right/down by 1 pixel
    mouse_event(MOUSEEVENTF_MOVE, 1, 1, 0, 0);
    Sleep(50);
    // Move back
    mouse_event(MOUSEEVENTF_MOVE, (DWORD)-1, (DWORD)-1, 0, 0);
}

void UpdateUI(HWND hDlg)
{
    if (!g_isRunning) return;

    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - g_startTime).count();

    std::wstring runtimeStr = L"Runtime: " + FormatDuration((int)duration);
    SetDlgItemText(hDlg, IDC_STATIC_RUNTIME, runtimeStr.c_str());

    wchar_t movesStr[64];
    swprintf_s(movesStr, L"Moves: %d", g_moveCount);
    SetDlgItemText(hDlg, IDC_STATIC_MOVES, movesStr);
}

void AddTrayIcon(HWND hDlg)
{
    g_nid.cbSize = sizeof(NOTIFYICONDATA);
    g_nid.hWnd = hDlg;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_SHOWTIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_AWAKEFOX));
    wcscpy_s(g_nid.szTip, L"AwakeFox - Stopped");
    g_nid.uVersion = NOTIFYICON_VERSION_4;

    Shell_NotifyIcon(NIM_ADD, &g_nid);
    Shell_NotifyIcon(NIM_SETVERSION, &g_nid);
}

void RemoveTrayIcon()
{
    Shell_NotifyIcon(NIM_DELETE, &g_nid);
}

void UpdateTrayIcon(bool active)
{
    if (active)
    {
        wcscpy_s(g_nid.szTip, L"AwakeFox - Running");
    }
    else
    {
        wcscpy_s(g_nid.szTip, L"AwakeFox - Stopped");
    }
    Shell_NotifyIcon(NIM_MODIFY, &g_nid);
}

void ShowTrayMenu(HWND hDlg)
{
    POINT pt;
    GetCursorPos(&pt);

    HMENU hMenu = CreatePopupMenu();
    AppendMenu(hMenu, MF_STRING, IDM_TRAY_SHOW, L"Show Window");
    AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenu(hMenu, MF_STRING, IDM_TRAY_STARTSTOP, g_isRunning ? L"Stop" : L"Start");
    AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenu(hMenu, MF_STRING, IDM_TRAY_EXIT, L"Exit");

    SetForegroundWindow(hDlg);
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hDlg, nullptr);
    DestroyMenu(hMenu);
}

void MinimizeToTray(HWND hDlg)
{
    ShowWindow(hDlg, SW_HIDE);
}

void RestoreFromTray(HWND hDlg)
{
    ShowWindow(hDlg, SW_SHOW);
    SetForegroundWindow(hDlg);
}

void LoadSettings()
{
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, REG_KEY, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        DWORD size = sizeof(DWORD);
        DWORD value;

        if (RegQueryValueEx(hKey, L"Interval", nullptr, nullptr, (LPBYTE)&value, &size) == ERROR_SUCCESS)
        {
            g_intervalSeconds = value;
            if (g_intervalSeconds < 10) g_intervalSeconds = 10;
            if (g_intervalSeconds > 300) g_intervalSeconds = 300;
        }

        if (RegQueryValueEx(hKey, L"MinimizeToTray", nullptr, nullptr, (LPBYTE)&value, &size) == ERROR_SUCCESS)
        {
            g_minimizeToTray = value != 0;
        }

        RegCloseKey(hKey);
    }
}

void SaveSettings()
{
    HKEY hKey;
    if (RegCreateKeyEx(HKEY_CURRENT_USER, REG_KEY, 0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr) == ERROR_SUCCESS)
    {
        DWORD value = g_intervalSeconds;
        RegSetValueEx(hKey, L"Interval", 0, REG_DWORD, (LPBYTE)&value, sizeof(DWORD));

        value = g_minimizeToTray ? 1 : 0;
        RegSetValueEx(hKey, L"MinimizeToTray", 0, REG_DWORD, (LPBYTE)&value, sizeof(DWORD));

        RegCloseKey(hKey);
    }
}

std::wstring FormatDuration(int totalSeconds)
{
    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int seconds = totalSeconds % 60;

    wchar_t buf[32];
    swprintf_s(buf, L"%02d:%02d:%02d", hours, minutes, seconds);
    return buf;
}
