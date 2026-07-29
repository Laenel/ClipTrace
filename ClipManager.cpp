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
#define ID_TRAYICON 114

HWND g_hMainWnd = NULL;
HWND g_hListView = NULL;
BOOL g_IgnoreNextCopy = FALSE;
std::vector<std::wstring> g_hListofCT;

HICON iconLg = (HICON)LoadImage(
	GetModuleHandle(NULL),
	MAKEINTRESOURCE(IDI_FAVICON),
	IMAGE_ICON,
	GetSystemMetrics(SM_CXICON),
	GetSystemMetrics(SM_CYICON),
	LR_DEFAULTCOLOR
);
HICON iconSm = (HICON)LoadImage(
	GetModuleHandle(NULL),
	MAKEINTRESOURCE(IDI_FAVICON),
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

void AddRow(HWND hWnd, std::wstring text) {
	LVITEM item = {};
	item.mask = LVIF_TEXT;
	item.iItem = 0;
	item.pszText = const_cast<LPWSTR>(text.c_str());
	ListView_InsertItem(hWnd, &item);
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg)
	{
	case WM_CREATE: {
		RECT rt, rtlv;
		GetClientRect(hWnd, &rt);
		GetWindowRect(g_hListView, &rtlv);
		SetupTrayIcon(hWnd);
		AddClipboardFormatListener(hWnd);
		const int lvStart = 15;
		g_hListView = CreateWindowEx(
			0, WC_LISTVIEW, NULL, WS_CHILD | WS_BORDER | LVS_REPORT | LVS_SINGLESEL | WS_VISIBLE, 0, lvStart, rt.right, max(0, rt.bottom - lvStart), hWnd, (HMENU)ID_LISTVIEW, GetModuleHandle(NULL), NULL
		);
		GetWindowRect(g_hListView, &rtlv);
		CreateThread(
			NULL, 0, CountThread, hWnd, 0, NULL
		);
		ListView_SetExtendedListViewStyle(g_hListView, LVS_EX_TWOCLICKACTIVATE | LVS_EX_FULLROWSELECT | LVS_EX_JUSTIFYCOLUMNS);
		LV_COLUMN col = {};
		col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
		col.cx = rtlv.right;
		col.fmt = LVCFMT_CENTER;
		col.pszText = (LPWSTR)L"Closed";
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
			}
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
		0, L"ClipTraceClass", L"ClipTrace", WS_POPUP & WS_BORDER, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, NULL, NULL, hInstance, NULL
	);
	ShowWindow(g_hMainWnd, SW_SHOW);
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}