//Version
#define PRODUCTNAME "Tablacus Dark"
#define INTERNALNAME "tablacusdark"
#define STRING(str) STRING2(str)
#define STRING2(str) #str
#define VER_Y		21
#define VER_M		12
#define VER_D		2
#define VER_Z		0
#define IDS_TEXT 1

//#define USE_VCL

#ifdef USE_VCL
#define TWC_BUTTON		WC_BUTTONA ";TButton;TCheckBox;TGroupBox;TRadioGroup;TGroupButton"
#define TWC_EDIT		WC_EDITA ";TEdit;TSpinEdit"
#define TWC_COMBOBOX	WC_COMBOBOXA ";TComboBox;TListBox"
#define TWC_TREEVIEW	WC_TREEVIEWA ";TTreeView;TTreeViewEx"
#define TWC_LISTVIEW	WC_LISTVIEWA ";TListView"
#define TWC_TABCONTROL	WC_TABCONTROLA ";TPageControl"
#else
#define TWC_BUTTON		WC_BUTTONA
#define TWC_EDIT		WC_EDITA
#define TWC_COMBOBOX	WC_COMBOBOXA
#define TWC_TREEVIEW	WC_TREEVIEWA
#define TWC_LISTVIEW	WC_LISTVIEWA
#define TWC_TABCONTROL	WC_TABCONTROLA
#endif
