#include "Resource.h"
#include <Windows.h>
#include <CommCtrl.h>
#include <string>
#include <vector>
#pragma comment(lib, "comctl32.lib")


#define ID_LISTBOX 113
#define WM_TRAYICON (WM_USER + 2)
#define ID_TRAYICON 114

HWND g_hMainWnd = NULL;
HWND g_hListView = NULL;
BOOL g_IgnoreNextCopy = FALSE;

void CopyToClipboard(HWND hWnd, std::wstring text) {
	if (!OpenClipboard(hWnd)) return;
	size_t size = (text.size() + 1) * (sizeof(wchar_t));
	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, size);
}