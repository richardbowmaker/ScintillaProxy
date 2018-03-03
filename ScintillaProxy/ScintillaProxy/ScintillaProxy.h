// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the SCINTILLAPROXY_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// SCINTILLAPROXY_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef SCINTILLAPROXY_EXPORTS
#define SCINTILLAPROXY_API __declspec(dllexport)
#else
#define SCINTILLAPROXY_API __declspec(dllimport)
#endif


// interface for haskell
extern "C"
{
	SCINTILLAPROXY_API HWND		__cdecl ScnNewEditor(HWND parent);
	SCINTILLAPROXY_API void		__cdecl ScnDestroyEditor(HWND scintilla);
	SCINTILLAPROXY_API LRESULT	__cdecl ScnSendEditor(HWND scintilla, UINT Msg, WPARAM wParam, LPARAM lParam);
	SCINTILLAPROXY_API void		__cdecl ScnSetEventHandler(HWND scintilla, void* callback);
	SCINTILLAPROXY_API void		__cdecl ScnEnableEvents(HWND scintilla);
	SCINTILLAPROXY_API void		__cdecl ScnDisableEvents(HWND scintilla);


	SCINTILLAPROXY_API HWND __cdecl GhciNew(HWND parent, char* options, char* file);
	SCINTILLAPROXY_API void __cdecl GhciSetEventHandler(HWND hwnd, void* callback);
	SCINTILLAPROXY_API void __cdecl GhciEnableEvents(HWND hwnd);
	SCINTILLAPROXY_API void __cdecl GhciDisableEvents(HWND hwnd);
	SCINTILLAPROXY_API void __cdecl GhciClose(HWND hwnd);
	SCINTILLAPROXY_API void __cdecl GhciPaste(HWND hwnd);
	SCINTILLAPROXY_API void __cdecl GhciCut(HWND hwnd);
	SCINTILLAPROXY_API void __cdecl GhciCopy(HWND hwnd);
	SCINTILLAPROXY_API void __cdecl GhciSelectAll(HWND hwnd);
	SCINTILLAPROXY_API BOOL __cdecl GhciHasFocus(HWND hwnd);
	SCINTILLAPROXY_API void __cdecl GhciSendCommand(HWND hwnd, char* cmd);
	SCINTILLAPROXY_API BOOL __cdecl GhciIsTextSelected(HWND hwnd);
}

// internal functions
void Initialise();
void Uninitialise();


