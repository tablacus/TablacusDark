# TablacusDark
Tablacus Dark は dll ファイルをLoadLibraryするだけでWindowsのアプリのダークモードが有効な場合にアプリから開くダイアログ等をダークモード化します。
MIT Lisenseなのであなたのアプリに同封して配布することも自由です。

C++でexeとdllの同じフォルダにある場合に以下の様にLoadLibraryするとダイアログのダークモードが有効になります。
```C++
	WCHAR pszPath[MAX_PATH];
	::GetModuleFileName(NULL, pszPath, MAX_PATH);
	::PathRemoveFileSpec(pszPath);
#ifdef _WIN64
	::PathAppend(pszPath, L"tablacusdark64.dll");
#else
	::PathAppend(pszPath, L"tablacusdark32.dll");
#endif
	HMODULE hDark = ::LoadLibrary(pszPath);
```

LoadLibraryするだけで クラス名が「#32770」のダイアログが以下の様にダークモードになります。

MessageBox

![image](https://user-images.githubusercontent.com/5156977/144431074-c802a291-161b-43ad-8d5a-9c08756c01eb.png)

ChooseFont

![image](https://user-images.githubusercontent.com/5156977/144431151-4a0afd6b-84da-492e-8749-1da086a59b64.png)

ChooseColor

![image](https://user-images.githubusercontent.com/5156977/144432854-43e7c70e-abe9-4cf7-9a01-088d1afd1228.png)

SHBrowseForFolder

![image](https://user-images.githubusercontent.com/5156977/144431282-3e61237a-b829-44e1-82dd-9284fc4fafee.png)

SHRunDialog

![image](https://user-images.githubusercontent.com/5156977/144431307-495363c0-c070-4804-aec5-1101258d40f3.png)

ShellAbout

![image](https://user-images.githubusercontent.com/5156977/144431454-8b1add4f-7076-4596-aecd-50d00c931437.png)

Property

![image](https://user-images.githubusercontent.com/5156977/144431561-7af80eb0-dc55-48ca-a7ad-c1eb0137da3c.png)

[サンプル実行ファイル](https://github.com/tablacus/TablacusDark/tree/main/test_exe)

![image](https://user-images.githubusercontent.com/5156977/143684389-d347c188-a982-434e-b84a-7b2a880712f5.png)

## Dialog
基本的なダイアログ

### MessageBox
MessageBoxを表示します。

### ChooseFont
フォント選択ダイアログを表示します。

### ChooseColor
色選択ダイアログを表示します。

### SHBrowseForFolder
フォルダ選択ダイアログを表示します。

### SHRunDialog
ファイル名を指定して実行を表示します。

### ShellAbout
バージョンダイアログを表示します。

### Property
プロパティを表示します。

## Main Window
おまけで関数を呼び出すと実行アプリのダークモード化が行えます。

```C++
typedef void (__stdcall* LPFNEntryPointW)(HWND hwnd, HINSTANCE hinst, LPWSTR lpszCmdLine, int nCmdShow);
```

### Title bar
タイトルバーをダークモード対応に変えます。

```C++
LPFNEntryPointW _SetDarkMode = NULL;
*(FARPROC *)&_SetDarkMode = GetProcAddress(hDark, "SetDarkMode");
_SetDarkMode(hWnd, NULL, NULL, -1);

```

### Menu
メニューとツールチップをダークモード対応に変えます。

```C++
LPFNEntryPointW _SetAppMode = NULL;
*(FARPROC *)&_SetAppMode = GetProcAddress(hDark, "SetAppMode");
_SetAppMode(NULL, NULL, NULL, -1);
```

### Window (Hook)
ダイアログと同じようにメインウインドウ側にもダークモード対応にします。

```C++
LPFNEntryPointW _FixWindow = NULL;
*(FARPROC *)&_FixWindow = GetProcAddress(hDark, "FixWindow");
_FixWindow(hWnd, NULL, NULL, 1);
```
					
詳しくは[サンプルプログラム](https://github.com/tablacus/TablacusDark/tree/main/test_exe)のソースをご覧ください。

---

## おまけ

### Meryのプラグインとして使う事もできます。
[Mery](https://www.haijin-boys.com/wiki/%E3%83%A1%E3%82%A4%E3%83%B3%E3%83%9A%E3%83%BC%E3%82%B8)のPluginsフォルダにコピーすると標準ダイアログがダークモード化します。
Meryはすでにダークモードされているので、変更をせずに保存するか確認するダイアログがダークモード化される位です。

![image](https://user-images.githubusercontent.com/5156977/144430605-c025a0cb-84c3-4815-a55b-ffc663046541.png)

### Susie Plug-in
拡張子を32ビット版はspiに、64ビット版はsphに変更するとSusie Plug-inに偽装することもできます。
