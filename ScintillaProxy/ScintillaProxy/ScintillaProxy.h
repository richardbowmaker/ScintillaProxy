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
	SCINTILLAPROXY_API BOOL		__cdecl ScintillaProxyInitialise();
	SCINTILLAPROXY_API void		__cdecl ScintillaProxyUninitialise();

	// scintilla editor
	SCINTILLAPROXY_API HWND		__cdecl ScnNewEditor(HWND parent);
	SCINTILLAPROXY_API void		__cdecl ScnDestroyEditor(HWND scintilla);
	SCINTILLAPROXY_API LRESULT	__cdecl ScnSendEditor(HWND scintilla, UINT Msg, WPARAM wParam, LPARAM lParam);
	SCINTILLAPROXY_API void		__cdecl ScnSetEventHandler(HWND scintilla, void* callback);
	SCINTILLAPROXY_API void		__cdecl ScnEnableEvents(HWND scintilla);
	SCINTILLAPROXY_API void		__cdecl ScnDisableEvents(HWND scintilla);
	SCINTILLAPROXY_API void		__cdecl ScnAddPopupMenuItem(HWND scintilla, int id, char* title, void* handler, void* enabled);

	// GHCI terminal
	SCINTILLAPROXY_API HWND __cdecl GhciTerminalNew(HWND parent, const char* options, const char* file, const char* directory);
	SCINTILLAPROXY_API void __cdecl GhciTerminalSetEventHandler(HWND hwnd, void* callback);
	SCINTILLAPROXY_API void __cdecl GhciTerminalEnableEvents(HWND hwnd);
	SCINTILLAPROXY_API void __cdecl GhciTerminalDisableEvents(HWND hwnd);
	SCINTILLAPROXY_API void __cdecl GhciTerminalClose(HWND hwnd);
	SCINTILLAPROXY_API void __cdecl GhciTerminalPaste(HWND hwnd);
	SCINTILLAPROXY_API void __cdecl GhciTerminalCut(HWND hwnd);
	SCINTILLAPROXY_API void __cdecl GhciTerminalCopy(HWND hwnd);
	SCINTILLAPROXY_API void __cdecl GhciTerminalSelectAll(HWND hwnd);
	SCINTILLAPROXY_API BOOL __cdecl GhciTerminalHasFocus(HWND hwnd);
	SCINTILLAPROXY_API void __cdecl GhciTerminalSetFocus(HWND hwnd);
	SCINTILLAPROXY_API void __cdecl GhciTerminalSendCommand(HWND hwnd, char* cmd);
	SCINTILLAPROXY_API void __cdecl GhciTerminalSendCommandAsynch(HWND hwnd, const char* cmd, const char* sod, const char* eod);
	SCINTILLAPROXY_API BOOL __cdecl GhciTerminalSendCommandSynch(HWND hwnd, const char* cmd, const char* eod, DWORD timeout, const char** response);
	SCINTILLAPROXY_API BOOL __cdecl GhciTerminalWaitForResponse(HWND hwnd, const char* eod, DWORD timeout, const char** response);
	SCINTILLAPROXY_API BOOL __cdecl GhciTerminalIsTextSelected(HWND hwnd);
	SCINTILLAPROXY_API int  __cdecl GhciTerminalGetTextLength(HWND hwnd);
	SCINTILLAPROXY_API int  __cdecl GhciTerminalGetText(HWND hwnd, char* buff, int size);
	SCINTILLAPROXY_API void __cdecl GhciTerminalClear(HWND hwnd);

	// GHCI session
	SCINTILLAPROXY_API int  __cdecl GhciNew(const char* options, const char* file, const char* directory);
	SCINTILLAPROXY_API void __cdecl GhciClose(int id);
	SCINTILLAPROXY_API void __cdecl GhciSetEventHandler(int id, void* callback, void* data);
	SCINTILLAPROXY_API void __cdecl GhciSendCommand(int id, const char* cmd);
	SCINTILLAPROXY_API void __cdecl GhciSendCommandAsynch(int id, const char* cmd, const char* sod, const char* eod);
	SCINTILLAPROXY_API BOOL __cdecl GhciSendCommandSynch(int id, const char* cmd, const char* eod, DWORD timeout, const char** response);
	SCINTILLAPROXY_API BOOL __cdecl GhciWaitForResponse(int id, const char* eod, DWORD timeout, const char** response);

	// file dialogs, moved here as they now crash in wxHaskell caused by a windows update of 10/5/2018
	SCINTILLAPROXY_API int  __cdecl WinOpenFileDialog(HWND parent, const char* prompt, const char* dir, const char* filter, const char* filterName, int flags, const char** fileName);
	SCINTILLAPROXY_API int  __cdecl WinSaveFileDialog(HWND parent, const char* prompt, const char* dir, const char* filter, const char* filterName, const char* defName, int flags, const char** fileName);
}

// internal functions
bool Initialise();
void Uninitialise();


