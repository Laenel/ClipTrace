#define UNICODE
#define _UNICODE

#include "Resource.h"
#include <Windows.h>
#include <CommCtrl.h>
#include <Pdh.h>
#include <string>
#include <vector>
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "pdh.lib")


#define ID_LISTVIEW 113
#define WM_TRAYICON (WM_USER + 2)
#define WM_PASSCHECK (WM_USER + 3)
#define WM_CONFSEL (WM_USER + 4)
#define ID_TRAYICON 114
#define ID_TITLE 115
#define ID_HK_V 116

HWND g_hMainWnd = NULL;
HWND g_hListView = NULL;
HWND g_h1Text = NULL;
HFONT g_h1Font;
HFONT g_listFont;
BOOL g_IgnoreNextCopy = FALSE;
std::vector<std::wstring> g_hListofCT;
const int lvStart = 35;

HICON iconLg = (HICON)LoadImage(
	GetModuleHandle(NULL),
	MAKEINTRESOURCE(IDI_CLIPMANAGER),
	IMAGE_ICON,
	GetSystemMetrics(SM_CXICON),
	GetSystemMetrics(SM_CYICON),
	LR_DEFAULTCOLOR
);
HICON iconSm = (HICON)LoadImage(
	GetModuleHandle(NULL),
	MAKEINTRESOURCE(IDI_SMALL),
	IMAGE_ICON,
	GetSystemMetrics(SM_CXSMICON),
	GetSystemMetrics(SM_CYSMICON),
	LR_DEFAULTCOLOR
);

void CopyToClipboard(HWND hWnd, std::wstring text) {
	if (!OpenClipboard(hWnd)) return;
	EmptyClipboard();
	size_t size = (text.size() + 1) * (sizeof(wchar_t));
	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, size);
	if (hMem) {
		memcpy(GlobalLock(hMem), text.c_str(), size);
		GlobalUnlock(hMem);
		g_IgnoreNextCopy = TRUE;
		SetClipboardData(CF_UNICODETEXT, hMem);
	}
	CloseClipboard();
}

std::wstring GetClipboardText(HWND hWnd) {
	std::wstring text = L"";
	if (!OpenClipboard(hWnd)) return text;
	HANDLE hData = GetClipboardData(CF_UNICODETEXT);
	if (hData) {
		wchar_t* pszText = static_cast<wchar_t*>(GlobalLock(hData));
		if (pszText) {
			text = pszText;
		}
		GlobalUnlock(hData);
	}
	CloseClipboard();
	return text;
}

void SetupTrayIcon(HWND hWnd) {
	NOTIFYICONDATA nid = {};
	nid.hWnd = hWnd;
	nid.hIcon = iconSm;
	nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
	nid.uID = ID_TRAYICON;
	nid.uCallbackMessage = WM_TRAYICON;
	lstrcpy(nid.szTip, L"ClipTrace");
	Shell_NotifyIcon(NIM_ADD, &nid);
}

DWORD WINAPI CountThread(LPVOID lpParam) {
	HWND hWnd = (HWND)lpParam;
	return 0;

}

DWORD WINAPI ConfirmationWorker(LPVOID lpParam) {
	int selected = (int)lpParam;
	if (selected >= 0) {
		wchar_t buff[60];
		std::wstring bcktxt = L"";
		LV_ITEM item = {};
		item.mask = LVIF_TEXT;
		item.iItem = selected;
		item.pszText = buff;
		item.iSubItem = 0;
		item.cchTextMax = 60;
		ListView_GetItem(g_hListView, &item);
		bcktxt = buff;
		item.pszText = (LPWSTR)L"Copied \u2714";
		ListView_SetItem(g_hListView, &item);
		Sleep(1500);
		item.pszText = (LPWSTR)bcktxt.c_str();
		ListView_SetItem(g_hListView, &item);
	}
	return 0;
}

void AddRow(HWND hWnd, std::wstring text) {
	LVITEM item = {};
	item.mask = LVIF_TEXT;
	item.iItem = 0;
	item.pszText = const_cast<LPWSTR>(text.c_str());
	ListView_InsertItem(hWnd, &item);
}

int PointerSizeFont(HWND hWnd, int ps) {
	UINT dpi = 96;
	HDC hdc = GetDC(hWnd);
	if (hdc) {
		dpi = GetDeviceCaps(hdc, LOGPIXELSY);
		ReleaseDC(hWnd, hdc);
	}
	return -MulDiv(ps, dpi, 72);
}

HFONT CreateSizeFont(HWND hWnd, int ps, int weight, BOOL italic, LPCWSTR fontFamily) {
	int height = PointerSizeFont(hWnd, ps);
	return CreateFont(
		height, 0, 0, 0, weight, italic, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEVICE_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, fontFamily
	);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg)
	{
	case WM_CREATE: {
		RECT rt, rtlv;
		GetClientRect(hWnd, &rt);
		SetupTrayIcon(hWnd);
		AddClipboardFormatListener(hWnd);
		g_h1Text = CreateWindowEx(
			0, L"STATIC", L"ClipTrace", WS_CHILD | WS_VISIBLE | SS_CENTER, 0, 0, rt.right, lvStart + 3, hWnd, (HMENU)ID_TITLE, GetModuleHandle(NULL), NULL
		);

		g_hListView = CreateWindowEx(
			0, WC_LISTVIEW, NULL, WS_CHILD | LVS_REPORT | LVS_SINGLESEL | LVS_NOCOLUMNHEADER | WS_VISIBLE, 0, lvStart, rt.right, max(0, rt.bottom - lvStart), hWnd, (HMENU)ID_LISTVIEW, GetModuleHandle(NULL), NULL
		);
		g_h1Font = CreateSizeFont(hWnd, 22, FW_SEMIBOLD, FALSE, L"Griffy");
		g_listFont = CreateSizeFont(hWnd, 11, FW_LIGHT, FALSE, L"Playwrite NZ Basic Regular");
		SendMessage(g_h1Text, WM_SETFONT, (WPARAM)g_h1Font, TRUE);
		SendMessage(g_hListView, WM_SETFONT, (WPARAM)g_listFont, TRUE);
		GetClientRect(g_hListView, &rtlv);
		CreateThread(
			NULL, 0, CountThread, hWnd, 0, NULL
		);
		ListView_SetExtendedListViewStyle(g_hListView, LVS_EX_TWOCLICKACTIVATE | LVS_EX_FULLROWSELECT | LVS_EX_JUSTIFYCOLUMNS);
		LV_COLUMN col = {};
		col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
		col.cx = rtlv.right;
		col.fmt = LVCFMT_CENTER;
		col.pszText = (LPWSTR)L"";
		ListView_InsertColumn(g_hListView, 0, &col);
		break;

	}
	case WM_TRAYICON: {
		if (lParam == WM_LBUTTONDBLCLK) {
			ShowWindow(hWnd, IsWindowVisible(hWnd) ? SW_HIDE : (SW_SHOW && SetForegroundWindow(hWnd)));
		}
		break;
	}
	case WM_ACTIVATE: {
		if (LOWORD(wParam) == WA_INACTIVE) {
			ShowWindow(hWnd, SW_HIDE);
		}
		break;
	}
	case WM_PASSCHECK: {
		if (lParam == 0) {
			double count = (double)wParam;
			AddRow(g_hListView, std::to_wstring(wParam));
		}
		break;
	}
	case WM_CLIPBOARDUPDATE: {
		if (g_IgnoreNextCopy == TRUE) {
			g_IgnoreNextCopy = FALSE;
			break;
		}
		std::wstring copiedText = GetClipboardText(hWnd);
		if (!copiedText.empty()) {
			if (g_hListofCT.empty() || g_hListofCT.front() != copiedText) {
				std::wstring displayText = copiedText.substr(0, 40);
				if (copiedText.size() > 40) displayText += L"...";
				AddRow(g_hListView, displayText);
				g_hListofCT.insert(g_hListofCT.begin(), copiedText);
			}
		}
		break;
	}
	case WM_NOTIFY: {
		LPNMHDR hdr = (LPNMHDR)lParam;
		if (hdr->idFrom == ID_LISTVIEW && hdr->code == NM_DBLCLK) {
			int sel = ListView_GetNextItem(g_hListView, -1, LVNI_SELECTED);
			if (sel >= 0 && sel < (int)g_hListofCT.size()) {
				CopyToClipboard(hWnd, g_hListofCT[sel].c_str());
				MessageBeep(MB_OK);
				SendMessage(hWnd, WM_CONFSEL, (WPARAM)sel, 0x006725);
			}
		}
		break;
	}
	case WM_HOTKEY: {
		if (wParam == ID_HK_V) {
			ShowWindow(hWnd, SW_SHOW);
			SetForegroundWindow(hWnd);
			SetFocus(hWnd);
		}
		break;
	}
	case WM_CONFSEL: {
		if (lParam == 0x006725) {
			CreateThread(NULL, 0, ConfirmationWorker, (LPVOID)wParam, NULL, 0);
		}
		break;
	}
	case WM_CLOSE: {
		ShowWindow(hWnd, SW_HIDE);
		return 0;
	}
	case WM_DESTROY: {
		ShowWindow(hWnd, SW_HIDE);
		RemoveClipboardFormatListener(hWnd);
		UnregisterHotKey(hWnd, ID_HK_V);
		DeleteObject(g_h1Font);
		DeleteObject(g_listFont);
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
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = L"ClipTraceClass";
	wc.hIcon = iconLg;
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 2);
	INITCOMMONCONTROLSEX icex = { sizeof(icex), ICC_LISTVIEW_CLASSES };
	InitCommonControlsEx(&icex);
	RegisterClass(&wc);
	g_hMainWnd = CreateWindowEx(
		0, L"ClipTraceClass", L"ClipTrace", WS_POPUP & WS_BORDER, CW_USEDEFAULT, CW_USEDEFAULT, 250, 250, NULL, NULL, hInstance, NULL
	);
	ShowWindow(g_hMainWnd, SW_SHOW);
	if (!RegisterHotKey(g_hMainWnd, ID_HK_V, MOD_CONTROL | MOD_ALT, 'V')) {
		DWORD err = GetLastError();
	}
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}