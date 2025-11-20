#pragma once

// Standard Windows constant (if not defined)
#ifndef IDC_STATIC
#define IDC_STATIC               -1
#endif

// Icons
#define IDI_AWAKEFOX             101
#define IDI_AWAKEFOX_ACTIVE      102

// Dialog
#define IDD_MAIN                 103

// Controls
#define IDC_BTN_STARTSTOP        1001
#define IDC_SLIDER_INTERVAL      1002
#define IDC_STATIC_INTERVAL      1003
#define IDC_STATIC_STATUS        1004
#define IDC_STATIC_RUNTIME       1005
#define IDC_STATIC_MOVES         1006
#define IDC_CHECK_MINIMIZE       1007
#define IDC_CHECK_AUTOSTART      1008

// Menu (system tray)
#define IDM_TRAY_SHOW            2001
#define IDM_TRAY_STARTSTOP       2002
#define IDM_TRAY_EXIT            2003

// Timer
#define IDT_MOUSE_MOVE           3001
#define IDT_UPDATE_UI            3002

// Tray icon
#define WM_TRAYICON              (WM_USER + 1)

// Version info
#define VER_MAJOR                1
#define VER_MINOR                0
#define VER_BUILD                0
