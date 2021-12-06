// Tablacus Darkmode (C)2021 Gaku
// MIT Lisence
// Visual Studio Express 2017 for Windows Desktop
// Visual Studio 2017 (v141)

#include "pch.h"
#include "resource.h"
#include <windowsx.h>
#include <CommCtrl.h>
#include <Shlwapi.h>
#include <Uxtheme.h>
#include <tchar.h>
#include <unordered_map>
#include <vector>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "UxTheme.lib")

#define DLLEXPORT __pragma(comment(linker, "/EXPORT:" __FUNCTION__ "=" __FUNCDNAME__))

#define TECL_DARKTEXT 0xffffff
#define TECL_DARKTEXT2 0xe0e0e0
#define TECL_DARKBG 0x202020
#define TECL_DARKSEL 0x606060
#define TECL_DARKFRAME 0x909090

#ifndef PreferredAppMode
enum PreferredAppMode
{
	APPMODE_DEFAULT = 0,
	APPMODE_ALLOWDARK = 1,
	APPMODE_FORCEDARK = 2,
	APPMODE_FORCELIGHT = 3,
	APPMODE_MAX = 4
};
#endif
#ifndef WINCOMPATTRDATA
struct WINCOMPATTRDATA
{
	DWORD dwAttrib;
	PVOID pvData;
	SIZE_T cbData;
};
#endif

typedef PreferredAppMode (WINAPI* LPFNSetPreferredAppMode)(PreferredAppMode appMode);
typedef bool (WINAPI* LPFNAllowDarkModeForWindow)(HWND hwnd, BOOL allow);
typedef bool (WINAPI* LPFNShouldAppsUseDarkMode)();
typedef void (WINAPI* LPFNRefreshImmersiveColorPolicyState)();
typedef bool (WINAPI* LPFNIsDarkModeAllowedForWindow)(HWND hwnd);
typedef BOOL(WINAPI* LPFNSetWindowCompositionAttribute)(HWND hWnd, WINCOMPATTRDATA*);
typedef HRESULT (STDAPICALLTYPE * LPFNDwmSetWindowAttribute)(HWND hwnd, DWORD dwAttribute, LPCVOID pvAttribute, DWORD   cbAttribute);

LPFNSetPreferredAppMode _SetPreferredAppMode = NULL;
LPFNAllowDarkModeForWindow _AllowDarkModeForWindow = NULL;
LPFNShouldAppsUseDarkMode _ShouldAppsUseDarkMode = NULL;
LPFNRefreshImmersiveColorPolicyState _RefreshImmersiveColorPolicyState = NULL;
LPFNIsDarkModeAllowedForWindow _IsDarkModeAllowedForWindow = NULL;
LPFNSetWindowCompositionAttribute _SetWindowCompositionAttribute = NULL;
LPFNDwmSetWindowAttribute _DwmSetWindowAttribute = NULL;

std::unordered_map<HWND, int> g_umHook;
std::unordered_map<DWORD, HHOOK> g_umCBTHook;
std::unordered_map<HWND, HWND> g_umDlgProc;

HBRUSH	g_hbrDarkBackground;

BOOL	g_bDarkMode = FALSE;
BOOL	g_bTooltips = FALSE;
BOOL	g_bFixOwnerDrawCB = TRUE;
BOOL	g_bOwnerDrawTC = TRUE;

BOOL IsHighContrast()
{
	HIGHCONTRAST highContrast = { sizeof(HIGHCONTRAST) };
	return SystemParametersInfo(SPI_GETHIGHCONTRAST, sizeof(highContrast), &highContrast, FALSE) && (highContrast.dwFlags & HCF_HIGHCONTRASTON);
}

BOOL IsClan(HWND hwndRoot, HWND hwnd)
{
	while (hwnd != hwndRoot) {
		hwnd = GetParent(hwnd);
		if (!hwnd) {
			return FALSE;
		}
	}
	return TRUE;
}

VOID teFixGroup(LPNMLVCUSTOMDRAW lplvcd, COLORREF clrBk)
{
	if (lplvcd->dwItemType == LVCDI_GROUP) { //Fix groups in dark background
		if (lplvcd->nmcd.dwDrawStage == CDDS_PREPAINT) {
			FillRect(lplvcd->nmcd.hdc, &lplvcd->rcText, GetStockBrush(WHITE_BRUSH));
		} else if (lplvcd->nmcd.dwDrawStage == CDDS_POSTPAINT) {
			int w = lplvcd->rcText.right - lplvcd->rcText.left;
			int h = lplvcd->rcText.bottom - lplvcd->rcText.top;
			BYTE r0 = GetRValue(clrBk);
			BYTE g0 = GetGValue(clrBk);
			BYTE b0 = GetBValue(clrBk);
			BITMAPINFO bmi;
			RGBQUAD *pcl = NULL;
			::ZeroMemory(&bmi.bmiHeader, sizeof(BITMAPINFOHEADER));
			bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			bmi.bmiHeader.biWidth = w;
			bmi.bmiHeader.biHeight = -(LONG)h;
			bmi.bmiHeader.biPlanes = 1;
			bmi.bmiHeader.biBitCount = 32;
			HBITMAP hBM = CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, (void **)&pcl, NULL, 0);
			HDC hmdc = CreateCompatibleDC(lplvcd->nmcd.hdc);
			HGDIOBJ hOld = SelectObject(hmdc, hBM);
			BitBlt(hmdc, 0, 0, w, h, lplvcd->nmcd.hdc, lplvcd->rcText.left, lplvcd->rcText.top, NOTSRCCOPY);
			for (int i = w * h; --i >= 0; ++pcl) {
				if (pcl->rgbRed || pcl->rgbGreen || pcl->rgbBlue) {
					WORD cl = pcl->rgbRed > pcl->rgbGreen ? pcl->rgbRed : pcl->rgbGreen;
					if (cl < pcl->rgbBlue) {
						cl = pcl->rgbBlue;
					}
					cl += 48;
					pcl->rgbRed = pcl->rgbGreen = pcl->rgbBlue = cl > 0xff ? 0xff : cl;
				} else {
					pcl->rgbRed = r0;
					pcl->rgbGreen = g0;
					pcl->rgbBlue = b0;
				}
			}
			BitBlt(lplvcd->nmcd.hdc, lplvcd->rcText.left, lplvcd->rcText.top, w, h, hmdc, 0, 0, SRCCOPY);
			SelectObject(hmdc, hOld);
			DeleteDC(hmdc);
			DeleteObject(hBM);
		}
	}
}

#ifdef _DEBUG
#pragma comment(lib, "version.lib")
int GetComctl32Version()
{
	int nVer = 0;
	DWORD dwLen;
	DWORD dwHandle;
	CHAR pszPathA[] = "comctl32.dll";
	if (dwLen = ::GetFileVersionInfoSizeA(pszPathA, &dwHandle)) {
		char *lpData = new char[dwLen + 1];
		if (lpData) {
			if (::GetFileVersionInfoA(pszPathA, 0, dwLen, lpData)) {
				UINT nLen;
				VS_FIXEDFILEINFO* pv;
				if (::VerQueryValueA(lpData, "\\", (void **)&pv, &nLen)) {
					nVer = HIWORD(pv->dwFileVersionMS);
				}
			}
			delete [] lpData;
		}
	}
	return nVer;
}
#endif

EXTERN_C void CALLBACK GetDarkMode(HWND hWnd, HINSTANCE hInstance, PBOOL bDarkMode, int nCmdShow)
{
	DLLEXPORT;

	if (_ShouldAppsUseDarkMode) {
		g_bDarkMode = _ShouldAppsUseDarkMode() && IsAppThemed() && !IsHighContrast();
	}
	if (bDarkMode) {
		*bDarkMode = g_bDarkMode;
	}
}

void CALLBACK SetDarkMode(HWND hwnd, HINSTANCE hInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	DLLEXPORT;

	GetDarkMode(NULL, NULL, NULL, 0);
	if (_AllowDarkModeForWindow) {
		_AllowDarkModeForWindow(hwnd, g_bDarkMode);
	}
	if (nCmdShow & 1) {
		if (_SetWindowCompositionAttribute) {
			WINCOMPATTRDATA wcpad = { 26, &g_bDarkMode, sizeof(g_bDarkMode) };
			_SetWindowCompositionAttribute(hwnd, &wcpad);
		} else if (_DwmSetWindowAttribute) {
			_DwmSetWindowAttribute(hwnd, 19, &g_bDarkMode, sizeof(g_bDarkMode));
		}
		if (_RefreshImmersiveColorPolicyState) {
			_RefreshImmersiveColorPolicyState();
		}
	}
}

EXTERN_C LRESULT CALLBACK ListViewProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	DLLEXPORT;

	try {
		switch (msg) {
		case LVM_SETSELECTEDCOLUMN:
			if (g_bDarkMode) {
				wParam = -1;
			}
			break;
		case WM_NOTIFY:
			if (g_bDarkMode) {
				if (((LPNMHDR)lParam)->code == NM_CUSTOMDRAW) {
					LPNMCUSTOMDRAW pnmcd = (LPNMCUSTOMDRAW)lParam;
					if (pnmcd->dwDrawStage == CDDS_PREPAINT) {
						return CDRF_NOTIFYITEMDRAW;
					}
					if (pnmcd->dwDrawStage == CDDS_ITEMPREPAINT) {
						SetTextColor(pnmcd->hdc, TECL_DARKTEXT);
						return CDRF_DODEFAULT;
					}
				}
				DefSubclassProc(hwnd, LVM_SETSELECTEDCOLUMN, -1, 0);
			}
			break;
		}
	} catch (...) {
	}
	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

/*
LRESULT CALLBACK ButtonProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	try {
		if (g_bDarkMode && msg == WM_PAINT) {
			DWORD dwStyle = GetWindowLong(hwnd, GWL_STYLE);
			if ((dwStyle & (BS_TYPEMASK | BS_BITMAP)) == BS_GROUPBOX) {
				PAINTSTRUCT ps;
				HDC hdc = BeginPaint(hwnd, &ps);
				int nLen = ::GetWindowTextLength(hwnd);
				if (nLen) {
					BSTR bs = ::SysAllocStringLen(NULL, nLen + 1);
					::GetWindowText(hwnd, bs, nLen + 1);
					HGDIOBJ hFont = (HGDIOBJ)::SendMessage(hwnd, WM_GETFONT, 0, 0);
					::SelectObject(ps.hdc, hFont);
					SetDCPenColor(ps.hdc, TECL_DARKFRAME);
					SelectObject(ps.hdc, GetStockPen(DC_PEN));
					SelectObject(ps.hdc, g_hbrDarkBackground);
					RECT rc = { 0, 0, 32767, 255 };
					::DrawText(ps.hdc, bs, nLen + 1, &rc, DT_HIDEPREFIX | DT_CALCRECT);
					MoveToEx(ps.hdc, ps.rcPaint.left, ps.rcPaint.top + rc.bottom / 2, NULL);
					LineTo(ps.hdc, ps.rcPaint.left, ps.rcPaint.bottom);
					LineTo(ps.hdc, ps.rcPaint.right, ps.rcPaint.bottom);
					LineTo(ps.hdc, ps.rcPaint.right, ps.rcPaint.top + rc.bottom / 2);
					LineTo(ps.hdc, ps.rcPaint.left, ps.rcPaint.top + rc.bottom / 2);
					SetTextColor(ps.hdc, TECL_DARKTEXT);
					SetBkColor(ps.hdc, TECL_DARKBG);
					ps.rcPaint.left += rc.bottom / 2;
					::DrawText(ps.hdc, bs, nLen + 1, &ps.rcPaint, DT_HIDEPREFIX);
				}
				EndPaint(hwnd, &ps);
				return 0;
			}
		}
	} catch (...) {
	}
	return DefSubclassProc(hwnd, msg, wParam, lParam);
}
*/

EXTERN_C LRESULT CALLBACK TabCtrlProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	DLLEXPORT;

	try {
		if (g_bDarkMode && msg == WM_PAINT) {
			PAINTSTRUCT ps;
			RECT rc;
			WCHAR label[MAX_PATH];
			HDC hdc = BeginPaint(hwnd, &ps);
			DWORD dwStyle = GetWindowLong(hwnd, GWL_STYLE);
			SetBkMode(ps.hdc, TRANSPARENT);
			SetDCPenColor(ps.hdc, TECL_DARKSEL);
			SelectObject(ps.hdc, GetStockPen(DC_PEN));
			SelectObject(ps.hdc, g_hbrDarkBackground);
			::FillRect(ps.hdc, &ps.rcPaint, g_hbrDarkBackground);
			HGDIOBJ hFont = (HGDIOBJ)::SendMessage(hwnd, WM_GETFONT, 0, 0);
			::SelectObject(hdc, hFont);
			int nSelected = TabCtrl_GetCurSel(hwnd);
			HIMAGELIST himl = TabCtrl_GetImageList(hwnd);
			int cx, cy;
			ImageList_GetIconSize(himl, &cx, &cy);
			for (int i = TabCtrl_GetItemCount(hwnd); i-- > 0;) {
				TabCtrl_GetItemRect(hwnd, i, &rc);
				if (!(dwStyle & (TCS_BUTTONS | TCS_FLATBUTTONS))) {
					++rc.right;
					--rc.top;
					rc.bottom += 2;
				}
				TC_ITEM tci;
				tci.mask = TCIF_TEXT | TCIF_IMAGE;
				tci.pszText = label;
				tci.cchTextMax = _countof(label) - 1;
				TabCtrl_GetItem(hwnd, i, &tci);
				if (i == nSelected) {
					SetDCBrushColor(ps.hdc, TECL_DARKSEL);
					::FillRect(ps.hdc, &rc, GetStockBrush(DC_BRUSH));
					SetTextColor(ps.hdc, TECL_DARKTEXT);
				} else {
					if (!(dwStyle & (TCS_BUTTONS | TCS_FLATBUTTONS))) {
						if (dwStyle & TCS_BOTTOM) {
							rc.bottom -= 2;
						} else {
							rc.top += 2;
						}
					}
					if (!(dwStyle & TCS_FLATBUTTONS)) {
						Rectangle(ps.hdc, rc.left, rc.top, rc.right, rc.bottom);
					}
					SetTextColor(ps.hdc, TECL_DARKTEXT2);
				}
				if (tci.iImage >= 0) {
					int x = rc.left + 6;
					int y = rc.top + ((rc.bottom - rc.top) - cy) / 2;
					ImageList_Draw(himl, tci.iImage, ps.hdc, x, y, ILD_TRANSPARENT);
					rc.left += cx + 3;
				}
				::DrawText(ps.hdc, label, -1, &rc, DT_HIDEPREFIX | DT_SINGLELINE | DT_VCENTER | DT_PATH_ELLIPSIS | DT_CENTER);
			}
			EndPaint(hwnd, &ps);
			return 0;
		}
	} catch (...) {
	}
	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

EXTERN_C void CALLBACK FixChildren(HWND hwnd, HINSTANCE hInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	DLLEXPORT;

	HWND hwnd1 = NULL;
	while (hwnd1 = ::FindWindowEx(hwnd, hwnd1, NULL, NULL)) {
		if (::GetWindowTheme(hwnd)) {
			break;
		}
		CHAR pszClassA[MAX_CLASS_NAME];
		::GetClassNameA(hwnd1, pszClassA, MAX_CLASS_NAME);
		if (::PathMatchSpecA(pszClassA, TWC_BUTTON)) {
			::SetWindowTheme(hwnd1, g_bDarkMode ? L"darkmode_explorer" : L"explorer", NULL);
			DWORD dwStyle = GetWindowLong(hwnd1, GWL_STYLE);
			DWORD dwButton = dwStyle & BS_TYPEMASK;
			if (dwButton > BS_DEFPUSHBUTTON && dwButton < BS_OWNERDRAW) {
				if (g_bDarkMode) {
					if (!(dwStyle & BS_BITMAP)) {
						int nLen = ::GetWindowTextLength(hwnd1);
						if (nLen) {
							BSTR bs = ::SysAllocStringLen(NULL, nLen + 1);
							::GetWindowText(hwnd1, bs, nLen + 1);
							HDC hdc = ::GetDC(hwnd);
							if (hdc) {
								HGDIOBJ hFont = (HGDIOBJ)::SendMessage(hwnd1, WM_GETFONT, 0, 0);
								HGDIOBJ hFontOld = ::SelectObject(hdc, hFont);
								RECT rc;
								::GetClientRect(hwnd1, &rc);
								::DrawText(hdc, bs, nLen + 1, &rc, DT_HIDEPREFIX | DT_CALCRECT);
								HBITMAP hBM = ::CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
								HDC hmdc = ::CreateCompatibleDC(hdc);
								HGDIOBJ hOld = ::SelectObject(hmdc, hBM);
								HGDIOBJ hFontOld2 = ::SelectObject(hmdc, hFont);
								::SetTextColor(hmdc, TECL_DARKTEXT);
								::SetBkColor(hmdc, TECL_DARKBG);
								::DrawText(hmdc, bs, nLen + 1, &rc, DT_HIDEPREFIX);
								::SelectObject(hmdc, hFontOld2);
								::SelectObject(hmdc, hOld);
								::DeleteDC(hmdc);
								::SelectObject(hdc, hFontOld);
								::ReleaseDC(hwnd1, hdc);
								::SetWindowLong(hwnd1, GWL_STYLE, (dwStyle | BS_BITMAP));
								::SendMessage(hwnd1, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBM);
								::SysFreeString(bs);
								g_umDlgProc.try_emplace(hwnd1, hwnd);
							}
						}
					}
				} else {
					auto itr = g_umDlgProc.find(hwnd1);
					if (itr != g_umDlgProc.end()) {
						HBITMAP hBM = (HBITMAP)::SendMessage(hwnd1, BM_SETIMAGE, IMAGE_BITMAP, NULL);
						if (hBM) {
							::DeleteObject(hBM);
						}
						::SetWindowLong(hwnd1, GWL_STYLE, (dwStyle & ~BS_BITMAP));
						g_umDlgProc.erase(itr);
					}
				}
			}
		} else if (::PathMatchSpecA(pszClassA, TWC_EDIT ";" TWC_COMBOBOX)) {
			::SetWindowTheme(hwnd1, g_bDarkMode ? L"darkmode_cfd" : L"cfd", NULL);
		} else if (::PathMatchSpecA(pszClassA, WC_SCROLLBARA)) {
			::SetWindowTheme(hwnd1, g_bDarkMode ? L"darkmode_explorer" : L"explorer", NULL);
		} else if (::PathMatchSpecA(pszClassA, TWC_TREEVIEW)) {
			SetWindowTheme(hwnd1, g_bDarkMode ? L"darkmode_explorer" : L"explorer", NULL);
			TreeView_SetTextColor(hwnd1, g_bDarkMode ? TECL_DARKTEXT : GetSysColor(COLOR_WINDOWTEXT));
			TreeView_SetBkColor(hwnd1, g_bDarkMode ? TECL_DARKBG : GetSysColor(COLOR_WINDOW));
		} else if (::PathMatchSpecA(pszClassA, TWC_LISTVIEW)) {
//			SetWindowTheme(hwnd1, g_bDarkMode ? L"darkmode_itemsview" : L"explorer", NULL);
			if (_AllowDarkModeForWindow) {
				_AllowDarkModeForWindow(hwnd1, g_bDarkMode);
				SetWindowTheme(hwnd1, L"explorer", NULL);
			}
			ListView_SetTextColor(hwnd1, g_bDarkMode ? TECL_DARKTEXT : GetSysColor(COLOR_WINDOWTEXT));
			ListView_SetTextBkColor(hwnd1, g_bDarkMode ? TECL_DARKBG : GetSysColor(COLOR_WINDOW));
			ListView_SetBkColor(hwnd1, g_bDarkMode ? TECL_DARKBG : GetSysColor(COLOR_WINDOW));
			HWND hHeader = ListView_GetHeader(hwnd1);
			if (hHeader) {
				SetWindowTheme(hHeader, g_bDarkMode ? L"darkmode_itemsview" : L"explorer", NULL);
			}
			if (g_bDarkMode) {
				auto itr = g_umDlgProc.find(hwnd1);
				if (itr == g_umDlgProc.end()) {
					SetWindowSubclass(hwnd1, ListViewProc, (UINT_PTR)ListViewProc, 0);
					g_umDlgProc[hwnd1] = hwnd;
				}
				ListView_SetSelectedColumn(hwnd1, -1);
			}
		} else if (::PathMatchSpecA(pszClassA, TWC_TABCONTROL)) {
			if (g_bOwnerDrawTC && g_bDarkMode) {
				::SetClassLongPtr(hwnd1, GCLP_HBRBACKGROUND, (LONG_PTR)g_hbrDarkBackground);
				auto itr = g_umDlgProc.find(hwnd1);
				if (itr == g_umDlgProc.end()) {
					SetWindowSubclass(hwnd1, TabCtrlProc, (UINT_PTR)TabCtrlProc, 0);
					g_umDlgProc[hwnd1] = hwnd;
				}
			}
		}
		FixChildren(hwnd1, hInstance, lpCmdLine, nCmdShow);
	}
}

VOID FixWindow1(HWND hwnd) {
	SetDarkMode(hwnd, NULL, NULL, 1);
	FixChildren(hwnd, NULL, NULL, 0);
	::RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN);
}

EXTERN_C LRESULT CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	DLLEXPORT;

	try {
		switch (msg) {
		case WM_INITDIALOG:
		case WM_SETTINGCHANGE:
			FixWindow1(hwnd);
			break;
		case WM_NCPAINT:
			FixChildren(hwnd, NULL, NULL, 0);
			break;
		case WM_CTLCOLORLISTBOX:
			if (_AllowDarkModeForWindow) {
				auto itr = g_umDlgProc.find((HWND)lParam);
				if (itr == g_umDlgProc.end()) {
					SetWindowTheme((HWND)lParam, g_bDarkMode ? L"darkmode_explorer" : L"explorer", NULL);
					g_umDlgProc[(HWND)lParam] = hwnd;
				}
			}
			break;
		}
		if (g_bDarkMode) {
			CHAR pszClassA[MAX_CLASS_NAME];
			switch (msg) {
			case WM_CTLCOLORBTN:
			case WM_CTLCOLORSTATIC:
				SetTextColor((HDC)wParam, TECL_DARKTEXT);
				SetBkColor((HDC)wParam, TECL_DARKBG);
//				SetBkMode((HDC)wParam, TRANSPARENT);
				return (LRESULT)g_hbrDarkBackground;
			case WM_CTLCOLORLISTBOX://Combobox
			case WM_CTLCOLOREDIT:
				SetTextColor((HDC)wParam, TECL_DARKTEXT);
				SetBkColor((HDC)wParam, TECL_DARKBG);
				return (LRESULT)GetStockObject(BLACK_BRUSH);
			case WM_ERASEBKGND:
				RECT rc;
				GetClientRect(hwnd, &rc);
				FillRect((HDC)wParam, &rc, g_hbrDarkBackground);
				return 1;
			case WM_PAINT:
				HWND hwndChild;
				hwndChild = NULL;
				BOOL bHandle;
				bHandle = TRUE;
				while (hwndChild = FindWindowEx(hwnd, hwndChild, NULL, NULL)) {
					GetClassNameA(hwndChild, pszClassA, MAX_CLASS_NAME);
					if (bHandle && !PathMatchSpecA(pszClassA, WC_STATICA ";" WC_BUTTONA ";" WC_COMBOBOXA)) {
						bHandle = FALSE;
					}
				}
				if (bHandle) {
					PAINTSTRUCT ps;
					HDC hdc = BeginPaint(hwnd, &ps);
					EndPaint(hwnd, &ps);
				}
				break;
			case WM_DRAWITEM:
				if (g_bFixOwnerDrawCB) {
					PDRAWITEMSTRUCT pdis;
					pdis = (PDRAWITEMSTRUCT)lParam;
					if (pdis->CtlType == ODT_COMBOBOX) {
						if (pdis->itemState & ODS_SELECTED) {
							break;
						}
						int w = pdis->rcItem.right - pdis->rcItem.left;
						int h = pdis->rcItem.bottom - pdis->rcItem.top;
						BITMAPINFO bmi;
						COLORREF *pcl = NULL, *pcl2 = NULL;
						::ZeroMemory(&bmi.bmiHeader, sizeof(BITMAPINFOHEADER));
						bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
						bmi.bmiHeader.biWidth = w;
						bmi.bmiHeader.biHeight = -(LONG)h;
						bmi.bmiHeader.biPlanes = 1;
						bmi.bmiHeader.biBitCount = 32;
						HGDIOBJ hFont = (HGDIOBJ)::SendMessage(pdis->hwndItem, WM_GETFONT, 0, 0);
						HBITMAP hBM = CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, (void **)&pcl, NULL, 0);
						HDC hmdc = CreateCompatibleDC(pdis->hDC);
						HGDIOBJ hOld = SelectObject(hmdc, hBM);
						HDC hdc1 = pdis->hDC;
						pdis->hDC = hmdc;
						CopyRect(&rc, &pdis->rcItem);
						pdis->rcItem.left = 0;
						pdis->rcItem.top = 0;
						pdis->rcItem.right = w;
						pdis->rcItem.bottom = h;
						HGDIOBJ hFontOld = ::SelectObject(hmdc, hFont);
						LRESULT lResult = DefSubclassProc(hwnd, msg, wParam, lParam);
						::SelectObject(hmdc, hFontOld);
						/* COLORREF              RGBQUAD
						rgbRed   =  0x000000FF = rgbBlue
						rgbGreen =  0x0000FF00 = rgbGreen
						rgbBlue  =  0x00FF0000 = rgbRed */
						DWORD cl0 = *pcl;
						if (299 * GetBValue(cl0) + 587 * GetGValue(cl0) + 114 * GetRValue(cl0) >= 127500) {
							HBITMAP hBM2 = CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, (void **)&pcl2, NULL, 0);
							HDC hmdc2 = CreateCompatibleDC(pdis->hDC);
							HGDIOBJ hOld2 = SelectObject(hmdc2, hBM2);
							pdis->hDC = hmdc2;
							pdis->itemState = ODS_SELECTED;
							HGDIOBJ hFontOld2 = ::SelectObject(hmdc2, hFont);
							::DefSubclassProc(hwnd, msg, wParam, lParam);
							::SelectObject(hmdc2, hFontOld2);
							if (*pcl != *pcl2) {
								for (int i = w * h; --i >= 0; ++pcl) {
									if (*pcl != *pcl2) {
										*pcl ^= 0xffffff;
									}
									++pcl2;
								}
							}
							SelectObject(hmdc2, hOld2);
							DeleteDC(hmdc2);
							DeleteObject(hBM2);
						}
						pdis->hDC = hdc1;
						CopyRect(&pdis->rcItem, &rc);
						BitBlt(pdis->hDC, pdis->rcItem.left, pdis->rcItem.top, w, h, hmdc, 0, 0, SRCCOPY);
						SelectObject(hmdc, hOld);
						DeleteDC(hmdc);
						DeleteObject(hBM);
						return lResult;
					}
				}
				break;
			case WM_NOTIFY:
				LPNMHDR lpnmhdr;
				lpnmhdr = (LPNMHDR)lParam;
				if (lpnmhdr->code == NM_CUSTOMDRAW) {
					LPNMLVCUSTOMDRAW lplvcd = (LPNMLVCUSTOMDRAW)lParam;
					teFixGroup(lplvcd, TECL_DARKBG);
					if (lplvcd->nmcd.dwDrawStage == CDDS_PREPAINT) {
						return CDRF_NOTIFYITEMDRAW;
					}
					if (lplvcd->dwItemType != LVCDI_GROUP) {
						if (lplvcd->nmcd.dwDrawStage == CDDS_ITEMPREPAINT) {
							DWORD uState = ListView_GetItemState(lpnmhdr->hwndFrom, lplvcd->nmcd.dwItemSpec, LVIS_SELECTED);
							if (uState & LVIS_SELECTED || lplvcd->nmcd.uItemState & CDIS_HOT) {
								RECT rc;
								ListView_GetItemRect(lpnmhdr->hwndFrom, lplvcd->nmcd.dwItemSpec, &rc, LVIR_SELECTBOUNDS);
								::SetDCBrushColor(lplvcd->nmcd.hdc, TECL_DARKSEL);
								::FillRect(lplvcd->nmcd.hdc, &rc, GetStockBrush(DC_BRUSH));
							}
							return CDRF_DODEFAULT;
						}
					}
				}
				break;
			}
		}
	} catch (...) {}
	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

EXTERN_C void CALLBACK FixWindow(HWND hwnd, HINSTANCE hInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	DLLEXPORT;

	FixWindow1(hwnd);
	auto itr = g_umHook.find(hwnd);
	if (itr == g_umHook.end()) {
		g_umHook[hwnd] = 1;
		SetWindowSubclass(hwnd, DialogProc, (UINT_PTR)DialogProc, 0);
	}
}

EXTERN_C void CALLBACK UndoWindow(HWND hwnd, HINSTANCE hInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	DLLEXPORT;

	for (auto itr = g_umDlgProc.begin(); itr != g_umDlgProc.end();) {
		if (IsClan(hwnd, itr->second)) {
			CHAR pszClassA[MAX_CLASS_NAME];
			GetClassNameA(itr->first, pszClassA, MAX_CLASS_NAME);
			if (::PathMatchSpecA(pszClassA, TWC_LISTVIEW)) {
				RemoveWindowSubclass(itr->first, ListViewProc, (UINT_PTR)ListViewProc);
			} else if (::PathMatchSpecA(pszClassA, TWC_TABCONTROL)) {
				RemoveWindowSubclass(itr->first, TabCtrlProc, (UINT_PTR)TabCtrlProc);
			} else if (::PathMatchSpecA(pszClassA, TWC_BUTTON)) {
				HBITMAP hBM = (HBITMAP)::SendMessage(itr->first, BM_SETIMAGE, IMAGE_BITMAP, NULL);
				if (hBM) {
					::DeleteObject(hBM);
				}
			}

			itr = g_umDlgProc.erase(itr);
		} else {
			++itr;
		}
	}
	auto itr = g_umHook.find(hwnd);
	if (itr != g_umHook.end()) {
		if (itr->second == 1) {
			g_umHook.erase(itr);
			RemoveWindowSubclass(hwnd, DialogProc, (UINT_PTR)DialogProc);
		}
	}
}

EXTERN_C void CALLBACK SetAppMode(HWND hwnd, HINSTANCE hInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	DLLEXPORT;

	GetDarkMode(NULL, NULL, NULL, 0);
	if (nCmdShow & 1) {
		if (_SetPreferredAppMode) {
			_SetPreferredAppMode(g_bDarkMode ? APPMODE_ALLOWDARK : APPMODE_DEFAULT);
		}
		if (_RefreshImmersiveColorPolicyState) {
			_RefreshImmersiveColorPolicyState();
		}
	}
	g_bTooltips = nCmdShow & 2;
	g_bFixOwnerDrawCB = nCmdShow & 4;
	g_bOwnerDrawTC = nCmdShow & 0x10;
}

LRESULT CALLBACK CBTProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	CHAR pszClassA[MAX_CLASS_NAME];
	if (nCode == HCBT_CREATEWND) {
		HWND hwnd = (HWND)wParam;
		GetClassNameA(hwnd, pszClassA, MAX_CLASS_NAME);
		if (PathMatchSpecA(pszClassA, "#32770")) {
			FixWindow(hwnd, NULL, NULL, 0);
		} else if (!lstrcmpA(pszClassA, TOOLTIPS_CLASSA)) {
			if (g_bTooltips) {
				SetWindowTheme(hwnd, g_bDarkMode ? L"darkmode_explorer" : L"explorer", NULL);
			}
		}
	} else if (nCode == HCBT_DESTROYWND) {
		UndoWindow((HWND)wParam, NULL, NULL, 0);
	} else if (nCode == HCBT_ACTIVATE) {
		HWND hwnd = (HWND)wParam;
		GetClassNameA(hwnd, pszClassA, MAX_CLASS_NAME);
		if (!lstrcmpA(pszClassA, TOOLTIPS_CLASSA)) {
			if (g_bTooltips) {
				SetWindowTheme(hwnd, g_bDarkMode ? L"darkmode_explorer" : L"explorer", NULL);
			}
		}
#ifdef USE_VCL
		else if (PathMatchSpecA(pszClassA, "T*Form")) {//VCL
			auto itr = g_umHook.find(hwnd);
			if (itr == g_umHook.end()) {
				g_umHook[hwnd] = 1;
				FixWindow1(hwnd);
				SetWindowSubclass(hwnd, DialogProc, (UINT_PTR)DialogProc, 0);
			}
		}
#endif
	}
	auto itr = g_umCBTHook.find(GetCurrentThreadId());
	return itr != g_umCBTHook.end() ? CallNextHookEx(itr->second, nCode, wParam, lParam) : 0;
}

BOOL APIENTRY DllMain( HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	HMODULE hDll;
	DWORD dwThreadId = GetCurrentThreadId();
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		if (hDll = LoadLibraryA("uxtheme.dll")) {
			*(FARPROC *)&_ShouldAppsUseDarkMode = GetProcAddress(hDll, MAKEINTRESOURCEA(132));
			*(FARPROC *)&_AllowDarkModeForWindow = GetProcAddress(hDll, MAKEINTRESOURCEA(133));
			*(FARPROC *)&_SetPreferredAppMode = GetProcAddress(hDll, MAKEINTRESOURCEA(135));
			*(FARPROC *)&_RefreshImmersiveColorPolicyState = GetProcAddress(hDll, MAKEINTRESOURCEA(104));
			*(FARPROC *)&_IsDarkModeAllowedForWindow = GetProcAddress(hDll, MAKEINTRESOURCEA(137));
			FreeLibrary(hDll);
		}
		if (hDll = LoadLibraryA("user32.dll")) {
			*(FARPROC *)&_SetWindowCompositionAttribute = GetProcAddress(hDll, "SetWindowCompositionAttribute");
			FreeLibrary(hDll);
		}
		if (!_SetWindowCompositionAttribute) {
			if (hDll = LoadLibraryA("dwmapi.dll")) {
				*(FARPROC *)&_DwmSetWindowAttribute = GetProcAddress(hDll, "DwmSetWindowAttribute");
				FreeLibrary(hDll);
			}
		}
		if (_ShouldAppsUseDarkMode && _AllowDarkModeForWindow) {
			GetDarkMode(NULL, NULL, NULL, 0);
			g_hbrDarkBackground = CreateSolidBrush(TECL_DARKBG);
		}
	case DLL_THREAD_ATTACH:
		g_umCBTHook.try_emplace(dwThreadId, SetWindowsHookEx(WH_CBT, (HOOKPROC)CBTProc, NULL, dwThreadId));
		break;
	case DLL_THREAD_DETACH:
		{
			auto itr = g_umCBTHook.find(dwThreadId);
			if (itr != g_umCBTHook.end()) {
				UnhookWindowsHookEx(itr->second);
				g_umCBTHook.erase(itr);
			}
		}
		break;
	case DLL_PROCESS_DETACH:
		for (auto itr = g_umDlgProc.begin(); itr != g_umDlgProc.end(); ++itr) {
			CHAR pszClassA[MAX_CLASS_NAME];
			GetClassNameA(itr->first, pszClassA, MAX_CLASS_NAME);
			if (::PathMatchSpecA(pszClassA, TWC_LISTVIEW)) {
				RemoveWindowSubclass(itr->first, ListViewProc, (UINT_PTR)ListViewProc);
			} else if (::PathMatchSpecA(pszClassA, TWC_TABCONTROL)) {
				RemoveWindowSubclass(itr->first, TabCtrlProc, (UINT_PTR)TabCtrlProc);
			}
		}
		g_umDlgProc.clear();
		for (auto itr = g_umCBTHook.begin(); itr != g_umCBTHook.end(); ++itr) {
			UnhookWindowsHookEx(itr->second);
		}
		g_umCBTHook.clear();
		for (auto itr = g_umHook.begin(); itr != g_umHook.end(); itr++) {
			if (itr->second == 1) {
				RemoveWindowSubclass(itr->first, DialogProc, (UINT_PTR)DialogProc);
			}
		}
		g_umHook.clear();
		if (g_hbrDarkBackground) {
			DeleteObject(g_hbrDarkBackground);
		}
		break;
	}
	return TRUE;
}

//Susie Plug-in
EXTERN_C int __stdcall GetPluginInfo(int infono, LPSTR buf, int buflen)
{
	DLLEXPORT;

	switch (infono){
	case 0:
		StrCpyNA(buf, "00IN", buflen);
		break;
	case 1:
		StrCpyNA(buf, "Tablacus Dark", buflen);
		break;
	case 2:
		StrCpyNA(buf, "*.TDM", buflen);
		break;
	case 3:
		StrCpyNA(buf, "Tablacus Dark", buflen);
		break;
	default:
		buf[0] = NULL;
		break;
	}
	return lstrlenA(buf);
}

EXTERN_C int __stdcall IsSupported(LPCSTR filename, void *dw)
{
	DLLEXPORT;

	return 0;
}

EXTERN_C int __stdcall GetPictureInfo(LPSTR buf, LONG_PTR len, unsigned int flag, void *lpInfo)
{
	DLLEXPORT;

	return -1;
}

EXTERN_C int __stdcall GetPicture(LPSTR buf, LONG_PTR len,unsigned int flag, HANDLE *pHBInfo, HANDLE *pHBm, FARPROC lpPrgressCallback, LONG_PTR lData)
{
	DLLEXPORT;

	return -1;
}

EXTERN_C int __stdcall GetPreview(LPSTR buf,LONG_PTR len,unsigned int flag, HANDLE *pHBInfo, HANDLE *pHBm, FARPROC lpPrgressCallback, LONG_PTR lData)
{
	DLLEXPORT;

	return -1;
}

EXTERN_C int __stdcall ConfigurationDlg(HWND parent, int fnc)
{
	DLLEXPORT;

	MessageBox(parent, _T(PRODUCTNAME), _T(PRODUCTNAME), MB_OK);
	return 0;
}

//Mery plug-in

EXTERN_C void WINAPI OnCommand(HWND hwnd)
{
	DLLEXPORT;
}

EXTERN_C BOOL WINAPI QueryStatus(HWND hwnd, LPBOOL pbChecked)
{
	DLLEXPORT;

	return true;
}

EXTERN_C UINT WINAPI GetMenuTextID() {
	DLLEXPORT;

	return IDS_TEXT;
}

EXTERN_C UINT WINAPI GetStatusMessageID()
{
	DLLEXPORT;

	return IDS_TEXT;
}

EXTERN_C UINT WINAPI GetIconID()
{
	DLLEXPORT;

	return 0;
}

EXTERN_C void WINAPI OnEvents(HWND hwnd, UINT nEvent, LPARAM lParam)
{
	DLLEXPORT;

}

EXTERN_C LRESULT WINAPI PluginProc(HWND hwnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	DLLEXPORT;

	if (lParam) {
		switch (nMsg) {
		case WM_USER + 0x0500 + 2:
			lstrcpynW(LPWSTR(lParam), L"Tablacus Dark", (int)wParam);
			return lstrlen(LPWSTR(lParam));
		case WM_USER + 0x0500 + 3:
			swprintf_s(LPWSTR(lParam), wParam, L"%d.%d.%d", VER_Y, VER_M, VER_D);
			return lstrlen(LPWSTR(lParam));
		}
	}
	return	0;
}
