// Tablacus Darkmode (C)2021 Gaku
// MIT Lisence
// Visual Studio Express 2017 for Windows Desktop
// Visual Studio 2017 (v141)

#include "pch.h"
#include <windowsx.h>
#include <CommCtrl.h>
#include <Shlwapi.h>
#include <Uxtheme.h>
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
std::unordered_map<HWND, HWND> g_umSetTheme;
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

extern "C" void CALLBACK GetDarkMode(HWND hWnd, HINSTANCE hInstance, PBOOL bDarkMode, int nCmdShow)
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

extern "C" LRESULT CALLBACK ListViewProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
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

extern "C" LRESULT CALLBACK TabCtrlProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
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

extern "C" void CALLBACK FixChildren(HWND hwnd, HINSTANCE hInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	DLLEXPORT;

	HWND hwnd1 = NULL;
	while (hwnd1 = ::FindWindowEx(hwnd, hwnd1, NULL, NULL)) {
		if (::GetWindowTheme(hwnd)) {
			break;
		}
		CHAR pszClassA[MAX_CLASS_NAME];
		::GetClassNameA(hwnd1, pszClassA, MAX_CLASS_NAME);
		if (lstrcmpiA(pszClassA, WC_BUTTONA) == 0) {
			::SetWindowTheme(hwnd1, g_bDarkMode ? L"darkmode_explorer" : L"explorer", NULL);
			DWORD dwStyle = GetWindowLong(hwnd1, GWL_STYLE);
			if ((dwStyle & BS_TYPEMASK) > BS_DEFPUSHBUTTON) {
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
								::DeleteObject(hBM);
								::SysFreeString(bs);
								auto itr = g_umDlgProc.find(hwnd1);
								if (itr == g_umDlgProc.end()) {
									g_umDlgProc[hwnd1] = hwnd;
								}
							}
						}
					}
				} else {
					auto itr = g_umDlgProc.find(hwnd1);
					if (itr != g_umDlgProc.end()) {
						::SendMessage(hwnd1, BM_SETIMAGE, IMAGE_BITMAP, NULL);
						::SetWindowLong(hwnd1, GWL_STYLE, (dwStyle & ~BS_BITMAP));
						g_umDlgProc.erase(itr);
					}
				}
			}
		}
		if (lstrcmpiA(pszClassA, WC_COMBOBOXA) == 0) {
			::SetWindowTheme(hwnd1, g_bDarkMode ? L"darkmode_cfd" : L"cfd", NULL);
		}
		if (lstrcmpiA(pszClassA, WC_SCROLLBARA) == 0) {
			::SetWindowTheme(hwnd1, g_bDarkMode ? L"darkmode_explorer" : L"explorer", NULL);
		}
		if (lstrcmpiA(pszClassA, WC_TREEVIEWA) == 0) {
			SetWindowTheme(hwnd1, g_bDarkMode ? L"darkmode_explorer" : L"explorer", NULL);
			TreeView_SetTextColor(hwnd1, g_bDarkMode ? TECL_DARKTEXT : GetSysColor(COLOR_WINDOWTEXT));
			TreeView_SetBkColor(hwnd1, g_bDarkMode ? TECL_DARKBG : GetSysColor(COLOR_WINDOW));
		}
		if (lstrcmpiA(pszClassA, WC_LISTVIEWA) == 0) {
			SetWindowTheme(hwnd1, g_bDarkMode ? L"darkmode_itemsview" : L"explorer", NULL);
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
		}
		if (lstrcmpiA(pszClassA, WC_TABCONTROLA) == 0) {
			if (g_bOwnerDrawTC && g_bDarkMode && !(GetWindowLong(hwnd1, GWL_STYLE) & TCS_OWNERDRAWFIXED)) {
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

extern "C" LRESULT CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
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
				auto itr = g_umSetTheme.find((HWND)lParam);
				if (itr == g_umSetTheme.end()) {
					SetWindowTheme((HWND)lParam, g_bDarkMode ? L"darkmode_explorer" : L"explorer", NULL);
					g_umSetTheme[(HWND)lParam] = hwnd;
				}
			}
			break;
		case WM_CLOSE:
			for (auto itr = g_umSetTheme.begin(); itr != g_umSetTheme.end();) {
				if (hwnd == itr->second) {
					itr = g_umSetTheme.erase(itr);
				} else {
					++itr;
				}
			}
			for (auto itr = g_umDlgProc.begin(); itr != g_umDlgProc.end();) {
				if (hwnd == itr->second) {
					CHAR pszClassA[MAX_CLASS_NAME];
					GetClassNameA(itr->first, pszClassA, MAX_CLASS_NAME);
					if (lstrcmpiA(pszClassA, WC_LISTVIEWA) == 0) {
						RemoveWindowSubclass(itr->first, ListViewProc, (UINT_PTR)ListViewProc);
					} else if (lstrcmpiA(pszClassA, WC_TABCONTROLA) == 0) {
						RemoveWindowSubclass(itr->first, TabCtrlProc, (UINT_PTR)TabCtrlProc);
					}
					itr = g_umDlgProc.erase(itr);
				} else {
					++itr;
				}
			}
			break;
		}
		if (g_bDarkMode) {
			CHAR pszClassA[MAX_CLASS_NAME];
			switch (msg) {
			case WM_CTLCOLORSTATIC:
				SetTextColor((HDC)wParam, TECL_DARKTEXT);
				//SetBkColor((HDC)wParam, TECL_DARKBG);
				SetBkMode((HDC)wParam, TRANSPARENT);
				return (LRESULT)g_hbrDarkBackground;
			case WM_CTLCOLORLISTBOX://Combobox
			case WM_CTLCOLOREDIT:
				SetTextColor((HDC)wParam, TECL_DARKTEXT);
				SetBkMode((HDC)wParam, TRANSPARENT);
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
			case WM_NOTIFY:
				if (((LPNMHDR)lParam)->code == NM_CUSTOMDRAW) {
					if (g_bDarkMode) {
						LPNMCUSTOMDRAW pnmcd = (LPNMCUSTOMDRAW)lParam;
						if (pnmcd->dwDrawStage == CDDS_PREPAINT) {
							return CDRF_NOTIFYITEMDRAW;
						}
						if (pnmcd->dwDrawStage == CDDS_ITEMPREPAINT) {
							SetTextColor(pnmcd->hdc, TECL_DARKTEXT);
							return CDRF_DODEFAULT;
						}
						DefSubclassProc(hwnd, LVM_SETSELECTEDCOLUMN, -1, 0);
					}
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
			}
		}
	} catch (...) {}
	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

extern "C" void CALLBACK FixWindow(HWND hwnd, HINSTANCE hInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	DLLEXPORT;

	FixWindow1(hwnd);
	auto itr = g_umHook.find(hwnd);
	if (itr == g_umHook.end()) {
		g_umHook[hwnd] = 1;
		SetWindowSubclass(hwnd, DialogProc, (UINT_PTR)DialogProc, 0);
	}
}

extern "C" void CALLBACK UndoWindow(HWND hwnd, HINSTANCE hInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	DLLEXPORT;
	auto itr = g_umHook.find(hwnd);
	if (itr != g_umHook.end()) {
		if (itr->second == 1) {
			g_umHook.erase(itr);
			RemoveWindowSubclass(hwnd, DialogProc, (UINT_PTR)DialogProc);
		}
	}
}

extern "C" void CALLBACK SetAppMode(HWND hwnd, HINSTANCE hInstance, LPWSTR lpCmdLine, int nCmdShow)
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
		if (!lstrcmpA(pszClassA, "#32770")) {
			FixWindow((HWND)wParam, NULL, NULL, 0);
		}
		if (!lstrcmpA(pszClassA, TOOLTIPS_CLASSA)) {
			if (g_bTooltips) {
				SetWindowTheme(hwnd, g_bDarkMode ? L"darkmode_explorer" : L"explorer", NULL);
			}
		}
	} else if (nCode == HCBT_DESTROYWND) {
		UndoWindow((HWND)wParam, NULL, NULL, 0);
	} else if (nCode == HCBT_ACTIVATE) {
		if (g_bTooltips) {
			HWND hwnd = (HWND)wParam;
			GetClassNameA(hwnd, pszClassA, MAX_CLASS_NAME);
			if (!lstrcmpA(pszClassA, TOOLTIPS_CLASSA)) {
				SetWindowTheme(hwnd, g_bDarkMode ? L"darkmode_explorer" : L"explorer", NULL);
			}
		}
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
		{
			DWORD dwThreadId = GetCurrentThreadId();
			auto itr = g_umCBTHook.find(dwThreadId);
			if (itr == g_umCBTHook.end()) {
				g_umCBTHook[dwThreadId] = SetWindowsHookEx(WH_CBT, (HOOKPROC)CBTProc, NULL, dwThreadId);
			}
		}
		break;
	case DLL_THREAD_DETACH:
		{
			auto itr = g_umCBTHook.find(GetCurrentThreadId());
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
			if (lstrcmpiA(pszClassA, WC_LISTVIEWA) == 0) {
				RemoveWindowSubclass(itr->first, ListViewProc, (UINT_PTR)ListViewProc);
			} else if (lstrcmpiA(pszClassA, WC_TABCONTROLA) == 0) {
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
extern "C" int __stdcall GetPluginInfo(int infono, LPSTR buf, int buflen)
{
	DLLEXPORT;

	switch (infono){
	case 0:
		StrCpyNA(buf, "00IN", buflen);
		break;
	case 1:
		StrCpyNA(buf, "Tablacus dark", buflen);
		break;
	case 2:
		StrCpyNA(buf, "*.TDM", buflen);
		break;
	case 3:
		StrCpyNA(buf, "Tablacus dark", buflen);
		break;
	default:
		buf[0] = NULL;
		break;
	}
	return lstrlenA(buf);
}

extern "C" int __stdcall IsSupported(LPCSTR filename, void *dw)
{
	DLLEXPORT;

	return 0;
}

extern "C" int __stdcall GetPictureInfo(LPSTR buf, LONG_PTR len, unsigned int flag, void *lpInfo)
{
	DLLEXPORT;

	return -1;
}

extern "C" int __stdcall GetPicture(LPSTR buf, LONG_PTR len,unsigned int flag, HANDLE *pHBInfo, HANDLE *pHBm, FARPROC lpPrgressCallback, LONG_PTR lData)
{
	DLLEXPORT;

	return -1;
}

extern "C" int __stdcall GetPreview(LPSTR buf,LONG_PTR len,unsigned int flag, HANDLE *pHBInfo, HANDLE *pHBm, FARPROC lpPrgressCallback, LONG_PTR lData)
{
	DLLEXPORT;

	return -1;
}

extern "C" int __stdcall ConfigurationDlg(HWND parent, int fnc)
{
	DLLEXPORT;

	MessageBox(parent, L"Tablacus Dark", L"Talacus Dark", MB_OK);
	return 0;
}
