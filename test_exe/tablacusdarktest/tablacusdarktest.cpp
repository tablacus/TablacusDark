// tablacusdarktest.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "tablacusdarktest.h"
#include <CommCtrl.h>
#include <Shlobj.h>
#include <Shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

#define MAX_LOADSTRING 100

typedef void (__stdcall* LPFNEntryPointW)(HWND hwnd, HINSTANCE hinst, LPWSTR lpszCmdLine, int nCmdShow);
typedef VOID (WINAPI * LPFNSHRunDialog)(HWND hwnd, HICON hIcon, LPWSTR pszPath, LPWSTR pszTitle, LPWSTR pszPrompt, DWORD dwFlags);

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

LPFNEntryPointW _GetDarkMode = NULL;
LPFNEntryPointW _SetDarkMode = NULL;
LPFNEntryPointW _SetAppMode = NULL;
LPFNEntryPointW _FixWindow = NULL;
LPFNEntryPointW _FixChildren = NULL;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_TABLACUSDARKTEST, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_TABLACUSDARKTEST));
	WCHAR pszPath[MAX_PATH];
	::GetModuleFileName(NULL, pszPath, MAX_PATH);
	::PathRemoveFileSpec(pszPath);
#ifdef _WIN64
	::PathAppend(pszPath, L"tablacusdark64.dll");
#else
	::PathAppend(pszPath, L"tablacusdark32.dll");
#endif
	HMODULE hDark = ::LoadLibrary(pszPath);

	*(FARPROC *)&_GetDarkMode = GetProcAddress(hDark, "GetDarkMode");
	*(FARPROC *)&_SetDarkMode = GetProcAddress(hDark, "SetDarkMode");
	*(FARPROC *)&_SetAppMode = GetProcAddress(hDark, "SetAppMode");
	*(FARPROC *)&_FixWindow = GetProcAddress(hDark, "FixWindow");
	*(FARPROC *)&_FixChildren = GetProcAddress(hDark, "FixChildren");

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

	if (hDark) {
		FreeLibrary(hDark);
	}
    return (int) msg.wParam;
}
//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TABLACUSDARKTEST));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_TABLACUSDARKTEST);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }
   int xs = 40;
   int x = xs;
   int y = 20;
   int xd = 240;
   int bw = 200, bh = 30;

   CreateWindow(WC_STATIC, L"Dialog", SS_SIMPLE | WS_CHILD | WS_VISIBLE,
	   x, y, bw, bh, hWnd, (HMENU)IDM_MESSAGEBOX, hInstance, NULL
   );
   y += 40;

   CreateWindow(WC_BUTTON, L"MessageBox", BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE,
	   x, y, bw, bh, hWnd, (HMENU)IDM_MESSAGEBOX, hInstance, NULL
   );
   x += xd;
   CreateWindow(WC_BUTTON, L"ChooseFont", BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE,
	   x, y, bw, bh, hWnd, (HMENU)IDM_FONT, hInstance, NULL
   );
   x += xd;
   CreateWindow(WC_BUTTON, L"ChooseColor", BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE,
	   x, y, bw, bh, hWnd, (HMENU)IDM_COLOR, hInstance, NULL
   );

   y += 40;
   x = xs;
   CreateWindow(WC_BUTTON, L"SHBrowseForFolder", BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE,
	   x, y, bw, bh, hWnd, (HMENU)IDM_BROWSE, hInstance, NULL
   );
   x += xd;
   CreateWindow(WC_BUTTON, L"SHRunDialog", BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE,
	   x, y, bw, bh, hWnd, (HMENU)IDM_RUNDIALOG, hInstance, NULL
   );
   x += xd;
   CreateWindow(WC_BUTTON, L"ShellAbout", BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE,
	   x, y, bw, bh, hWnd, (HMENU)IDM_SHELLABOUT, hInstance, NULL
   );
   y += 40;

   x = xs;
   CreateWindow(WC_BUTTON, L"Property", BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE,
	   x, y, bw, bh, hWnd, (HMENU)IDM_PROPERTY, hInstance, NULL
   );
   y += 80;

   x = xs;
   CreateWindow(WC_STATIC, L"Main Window", SS_SIMPLE | WS_CHILD | WS_VISIBLE,
	   x, y, bw, bh, hWnd, (HMENU)IDM_MESSAGEBOX, hInstance, NULL
   );
   y += 40;
   x = xs;
   CreateWindow(WC_BUTTON, L"Title bar", BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE,
	   x, y, bw, bh, hWnd, (HMENU)IDM_TITLEBAR, hInstance, NULL
   );
   x += xd;
   CreateWindow(WC_BUTTON, L"Menu", BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE,
	   x, y, bw, bh, hWnd, (HMENU)IDM_MENU, hInstance, NULL
   );
   x += xd;
   CreateWindow(WC_BUTTON, L"Window (Hook)", BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE,
	   x, y, bw, bh, hWnd, (HMENU)IDM_HOOK, hInstance, NULL
   );
   
   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	LPITEMIDLIST pidl;
	WCHAR pszPath[MAX_PATH];
	HMENU hMenu;

	switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:

			switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
			case IDM_MESSAGEBOX:
				MessageBox(hWnd, L"Message Box", L"Tablacus Dark", MB_ICONINFORMATION | MB_CANCELTRYCONTINUE);
				break;
			case IDM_FONT:
				LOGFONT lf;
				CHOOSEFONT cf;
				memset(&cf, 0, sizeof(CHOOSEFONT));
				cf.lStructSize = sizeof(CHOOSEFONT);
				cf.hwndOwner = hWnd;
				cf.lpLogFont = &lf;
				cf.Flags = CF_SCREENFONTS | CF_EFFECTS;
				cf.rgbColors = RGB(0, 0, 0);
				cf.nFontType = SCREEN_FONTTYPE;
				ChooseFont(&cf);
				break;
			case IDM_COLOR:
				COLORREF CustColors[16];
				for (int i = 16; i--;) {
					CustColors[i] = RGB(rand(), rand(), rand());
				}
				CHOOSECOLOR cc;
				memset(&cc, 0, sizeof(CHOOSECOLOR));
				cc.lStructSize = sizeof(CHOOSECOLOR);
				cc.hwndOwner = hWnd;
				cc.lpCustColors = CustColors;
				cc.rgbResult = NULL;
				cc.Flags = CC_FULLOPEN | CC_RGBINIT;
				ChooseColor(&cc);
				break;
			case IDM_BROWSE:
				BROWSEINFO bInfo;
				memset(&bInfo, 0, sizeof(BROWSEINFO));
				bInfo.hwndOwner = hWnd;
				bInfo.lpszTitle = L"SHBrowseForFolder";
				bInfo.pszDisplayName = pszPath;
				bInfo.ulFlags = BIF_RETURNONLYFSDIRS | BIF_EDITBOX | BIF_VALIDATE | BIF_NEWDIALOGSTYLE;
				pidl = SHBrowseForFolder(&bInfo);
				if (pidl) {
					ILFree(pidl);
				}
				break;
			case IDM_RUNDIALOG:
				HINSTANCE hDll;
				hDll = LoadLibrary(L"shell32");
				if (hDll) {
					LPFNSHRunDialog _SHRunDialog = NULL;
					*(FARPROC *)&_SHRunDialog = GetProcAddress(hDll, MAKEINTRESOURCEA(61));
					if (_SHRunDialog) {
						_SHRunDialog(hWnd, NULL, NULL, NULL, NULL, 0);
					}
					FreeLibrary(hDll);
				}
				break;
			case IDM_SHELLABOUT:
				ShellAbout(hWnd, L"Tablacus Dark", L"ShellAbout", NULL);
				break;
			case IDM_PROPERTY:
				GetModuleFileName(NULL, pszPath, MAX_PATH);
				pidl = ILCreateFromPath(pszPath);
				if (pidl) {
					IShellFolder *pSF;
					LPCITEMIDLIST pidlPart;
					if SUCCEEDED(SHBindToParent(pidl, IID_PPV_ARGS(&pSF), &pidlPart)) {
						IContextMenu *pCM;
						if SUCCEEDED(pSF->GetUIObjectOf(hWnd, 1, &pidlPart, IID_IContextMenu, NULL, (LPVOID*)&pCM)) {
							hMenu = ::CreatePopupMenu();
							pCM->QueryContextMenu(hMenu, 0, 1, 0x7fff, CMF_DEFAULTONLY);
							CMINVOKECOMMANDINFO cmi = { sizeof(CMINVOKECOMMANDINFO), 0, NULL, (LPCSTR)(20 - 1) };
							pCM->InvokeCommand(&cmi);
							pCM->Release();
						}
					}
					ILFree(pidl);
				}
				break;

			case IDM_TITLEBAR:
				if (_SetDarkMode) {
					_SetDarkMode(hWnd, NULL, NULL, -1);
					BOOL bZoomed = ::IsZoomed(hWnd);
					::ShowWindow(hWnd, SW_HIDE);
					::ShowWindow(hWnd, bZoomed ? SW_SHOWMAXIMIZED : SW_SHOWDEFAULT);
				}
				break;
			case IDM_MENU:
				if (_SetAppMode) {
					_SetAppMode(NULL, NULL, NULL, -1);
				}
				hMenu = ::CreatePopupMenu();
				::InsertMenu(hMenu, 0, MF_BYPOSITION, 0, L"Menu1");
				::InsertMenu(hMenu, 1, MF_BYPOSITION, 1, L"Menu2");
				::InsertMenu(hMenu, 2, MF_BYPOSITION, 2, L"Menu3");
				::InsertMenu(hMenu, 3, MF_BYPOSITION, 3, L"Menu4");
				::InsertMenu(hMenu, 4, MF_BYPOSITION, 4, L"Menu5");
				POINT pt;
				::GetCursorPos(&pt);
				::TrackPopupMenu(hMenu, TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, NULL, hWnd, NULL);
				break;

			case IDM_HOOK:
				if (_FixWindow) {
					_FixWindow(hWnd, NULL, NULL, 1);
					BOOL bZoomed = ::IsZoomed(hWnd);
					::ShowWindow(hWnd, SW_HIDE);
					::ShowWindow(hWnd, bZoomed ? SW_SHOWMAXIMIZED : SW_SHOWDEFAULT);
				}
				break;

			default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
