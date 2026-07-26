#define UNICODE
#define _UNICODE
#include <Windows.h>
#include "Resource.h"
#include <string>

HWND g_hMainWindow = NULL;
HWND g_hTitle = NULL;
HWND g_hListBox = NULL;

HICON hIconLg = (HICON)LoadImage(
	GetModuleHandle(NULL),
	MAKEINTRESOURCE(IDI_FAVICON),
	IMAGE_ICON,
	GetSystemMetrics(SM_CXICON),
	GetSystemMetrics(SM_CYICON),
	LR_DEFAULTCOLOR
);
HICON hIconSm = (HICON)LoadImage(
	GetModuleHandle(NULL),
	MAKEINTRESOURCE(IDI_FAVICON),
	IMAGE_ICON,
	GetSystemMetrics(SM_CXSMICON),
	GetSystemMetrics(SM_CYSMICON),
	LR_DEFAULTCOLOR
);

HFONT h1Font = CreateFont(24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, L"Eater");

#define ID_LISTBOX 6267
#define WM_TRAYICON (WM_USER + 2)
#define ID_TRAYICON 1006
#define ID_TITLE 1002
#define ID_UPDATETITLETEXT (WM_USER + 5)
#define ID_HOTKEY 2

void CreateTrayIcon(HWND hWnd) {
	NOTIFYICONDATA nid = { sizeof(nid) };
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid.hWnd = hWnd;
	nid.uID = ID_TRAYICON;
	nid.uCallbackMessage = WM_TRAYICON;
	nid.hIcon = hIconSm;
	lstrcpy(nid.szTip, L"ClipManager");
	Shell_NotifyIcon(NIM_ADD, &nid);
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg)
	{
	case WM_CREATE: {
		RECT rt;
		GetClientRect(hWnd, &rt);
		const int titleLimit = 30;
		g_hTitle = CreateWindowEx(
			0, L"STATIC", L"Clipboard Manager",
			WS_CHILD | WS_VISIBLE | SS_CENTER,
			0, 0, rt.right, titleLimit,
			hWnd, (HMENU)ID_TITLE, GetModuleHandle(NULL), NULL
			);
		g_hListBox = CreateWindow(
			L"LISTBOX", NULL,
			WS_CHILD | WS_VISIBLE | LBS_NOTIFY | LBS_HASSTRINGS,
			0, titleLimit, rt.right, max(0, rt.bottom - titleLimit),
			hWnd, (HMENU)ID_LISTBOX, GetModuleHandle(NULL), NULL
		);
		CreateTrayIcon(hWnd);

		SendMessage(g_hTitle, WM_SETFONT, (WPARAM)h1Font, TRUE);
		SetFocus(hWnd);
		break;
	}
	case WM_SIZE: {
		int cx = LOWORD(lParam);
		int cy = HIWORD(lParam);
		const int titleLimit = 30;
		if (g_hListBox && g_hTitle) {
			MoveWindow(g_hTitle, 0, 0, cx, titleLimit, TRUE);
			MoveWindow(g_hListBox, 0, titleLimit, cx, max(0, cy - titleLimit), TRUE);
		}
		break;
	}
	case WM_TRAYICON: {
		if (lParam == WM_LBUTTONDBLCLK)
		{
			ShowWindow(hWnd, IsWindowVisible(hWnd) ? SW_HIDE : SW_SHOW);
			SetForegroundWindow(hWnd);
			SetFocus(hWnd);
		}
		break;
	}
	case WM_CLOSE: {
		ShowWindow(hWnd, SW_HIDE);
		return 0;
	}
	case WM_KEYDOWN: {
		if (wParam == 'V') {
			SetFocus(hWnd);
			SendMessage(hWnd, ID_UPDATETITLETEXT, (WPARAM)"V clicked", (LPARAM)0);
		}
		break;
	}
	case WM_HOTKEY: {
		if (wParam == ID_HOTKEY) {
			ShowWindow(hWnd, SW_SHOW);
			SetForegroundWindow(hWnd);
			SetFocus(hWnd);
			SendMessage(hWnd, ID_UPDATETITLETEXT, (WPARAM)"V clicked", 0);
		}
		break;
	}
	case ID_UPDATETITLETEXT: {
		if (lParam == 0 && wParam == (WPARAM)"V clicked") {
			SendMessage(g_hTitle, WM_SETTEXT, 0, (LPARAM)L"V was clicked!!");
			MessageBox(NULL, L"V was clicked", L"It was a long journey but V was finally clicked", MB_OKCANCEL);
		}
		break;
	}
	case WM_DESTROY: {
		ShowWindow(hWnd, SW_HIDE);
		UnregisterHotKey(hWnd, ID_HOTKEY);
		PostQuitMessage(0);
		break;
	}
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrev, _In_ LPSTR lpCmd, _In_ int nCmdShow) {
	WNDCLASS wc = { 0 };
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 2);
	wc.lpszClassName = L"ClipManagerClass";
	wc.hIcon = hIconLg;
	RegisterClass(&wc);
	g_hMainWindow = CreateWindowEx(
		0, L"ClipManagerClass", L"Clip Manager", WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_SIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, NULL, NULL, hInstance, NULL
	);
	ShowWindow(g_hMainWindow, SW_SHOW);
	if (!RegisterHotKey(g_hMainWindow, ID_HOTKEY, MOD_CONTROL | MOD_ALT, 'V')) {
		DWORD err = GetLastError();
	}
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}