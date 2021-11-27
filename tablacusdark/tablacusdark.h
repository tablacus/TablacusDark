// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the TABLACUSDARK_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// TABLACUSDARK_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef TABLACUSDARK_EXPORTS
#define TABLACUSDARK_API __declspec(dllexport)
#else
#define TABLACUSDARK_API __declspec(dllimport)
#endif

// This class is exported from the dll
class TABLACUSDARK_API Ctablacusdark {
public:
	Ctablacusdark(void);
	// TODO: add your methods here.
};

extern TABLACUSDARK_API int ntablacusdark;

TABLACUSDARK_API int fntablacusdark(void);
