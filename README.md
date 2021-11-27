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

LoadLibraryするだけで クラス名が「#32770」のダイアログが以下の様にダークモードになります。

MessageBox
![image](https://user-images.githubusercontent.com/5156977/143683851-b7677f45-d6ae-4c10-b7c4-2ae5475cadeb.png)

ChooseFont
![image](https://user-images.githubusercontent.com/5156977/143683888-230f267c-36f1-44ac-8a68-02b78b520aed.png)

ChooseColor
![image](https://user-images.githubusercontent.com/5156977/143683920-4bc96a6f-74eb-4e9b-9c90-0d61214e87c8.png)

SHBrowseForFolder
![image](https://user-images.githubusercontent.com/5156977/143683974-ccef60da-ec46-4298-a86a-f28802d51f67.png)

SHRunDialog
![image](https://user-images.githubusercontent.com/5156977/143684001-658c5380-1455-4882-8657-ac9693d1f853.png)

ShellAbout
![image](https://user-images.githubusercontent.com/5156977/143684102-82c16e53-a847-456f-bece-ee957a9660b4.png)

Property
![image](https://user-images.githubusercontent.com/5156977/143684155-16fb130c-f0cc-4717-b190-d81f73e26a11.png)


