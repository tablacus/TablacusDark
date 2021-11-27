# TablacusDark
Tablacus Dark は dll ファイルをLoadLibraryするだけでWindowsのアプリのダークモードが有効な場合にアプリから開くダイアログ等をダークモード化します。
MIT Lisenseなのであなたのアプリに同封して配布することも自由です。

C++でexeとdllの同じフォルダにある場合に以下の様にLoadLibraryするとダイアログのダークモードが有効になります。
```
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


