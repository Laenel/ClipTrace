#define UNICODE
#define _UNICODE
#include <Windows.h>
#include "Resource.h"
#include <string>
#include <vector>

HWND g_hMainWindow = NULL;
HWND g_hTitle = NULL;
HWND g_hListBox = NULL;
BOOL g_ignoreNextCopy = FALSE;
std::vector<std::wstring> g_ListofCT;

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

void CopyToClipboard(HWND hWnd, std::wstring& text) {
	if (!OpenClipboard(hWnd)) return;
	EmptyClipboard();
	size_t size = (text.size() + 1) * (sizeof(wchar_t));
	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, size);
	if (hMem) {
		memcpy(GlobalLock(hMem), text.c_str(), size);
		GlobalUnlock(hMem);
		g_ignoreNextCopy = TRUE;
		SetClipboardData(CF_UNICODETEXT, hMem);
	}
	CloseClipboard();
}

std::wstring GetClipboardText(HWND hWnd) {
	std::wstring text = L"";
	if (!OpenClipboard(hWnd)) return text;
	HANDLE hData = GetClipboardData(CF_UNICODETEXT);
	if (hData) {
		wchar_t* psz_text = static_cast<wchar_t*>(GlobalLock(hData));
		if (psz_text) {
			text = psz_text;
			GlobalUnlock(hData);
		}
		CloseClipboard();
	}
	return text;
}

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
		AddClipboardFormatListener(hWnd);
		SendMessage(g_hTitle, WM_SETFONT, (WPARAM)h1Font, TRUE);
		SetFocus(hWnd);
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
	case WM_CLIPBOARDUPDATE: {
		if (g_ignoreNextCopy == TRUE) {
			g_ignoreNextCopy = FALSE;
			break;
		}
		std::wstring copiedText = GetClipboardText(hWnd);
		if (!copiedText.empty()) {
			if (g_ListofCT.empty() || g_ListofCT.front() != copiedText) {
				g_ListofCT.insert(g_ListofCT.begin(), copiedText);
				std::wstring displayText = copiedText.substr(0, 40);
				if (copiedText.size() > 40) displayText += L".....";
				SendMessage(g_hListBox, LB_INSERTSTRING, 0, (LPARAM)displayText.c_str());
			}
		}
		break;
	}
	case WM_COMMAND: {
		if (LOWORD(wParam) == ID_LISTBOX && HIWORD(wParam) == LBN_SELCHANGE) {
			int sel = (int)SendMessage(g_hListBox, LB_GETCURSEL, 0, 0);
			if (sel >= 0 && sel < (int)g_ListofCT.size()) {
				std::wstring textClicked = g_ListofCT[sel];
				CopyToClipboard(hWnd, textClicked);
				MessageBeep(MB_OK);
				std::wstring textIndex = L"Text at index" + std::to_wstring(sel) + L" copied.";
				MessageBox(hWnd, textIndex.c_str(), L"Text Copied", MB_OK);
			}
		}
		break;
	}
	case WM_ACTIVATE: {
		if (LOWORD(wParam) == WA_INACTIVE) {
			ShowWindow(hWnd, SW_HIDE);
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
			//MessageBox(NULL, L"V was clicked", L"Virtual key V was pressed", MB_OKCANCEL);
		}
		break;
	}
	case WM_DESTROY: {
		ShowWindow(hWnd, SW_HIDE);
		UnregisterHotKey(hWnd, ID_HOTKEY);
		RemoveClipboardFormatListener(hWnd);
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
		0, L"ClipManagerClass", L"Clip Manager", WS_POPUP & WS_BORDER & WS_VISIBLE & ~WS_SIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, 400, 400, NULL, NULL, hInstance, NULL
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