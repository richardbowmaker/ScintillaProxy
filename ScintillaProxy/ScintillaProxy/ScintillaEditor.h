#pragma once

#include <Scintilla.h>
#include <SciLexer.h>


class CScintillaEditor
{
public:
	CScintillaEditor();
	~CScintillaEditor();

	// pointer to haskell event handler
	typedef void* (*EventHandlerT)(SCNotification*);
	typedef unsigned __int64(__cdecl *ScintillaDirect)(void*, int, __int64, unsigned __int64);

	HWND	Initialise(HWND parent);
	void	Uninitialise();
	LRESULT SendEditor(UINT Msg, WPARAM wParam = 0, LPARAM lParam = 0);
	void	SetEventHandler(void* callback);
	void	EnableEvents();
	void	DisableEvents();
	void WndProcRetHook(int nCode, WPARAM wParam, LPARAM lParam);
	void	ConfigureEditorHaskell();
	void    SetAStyle(int style, COLORREF fore, COLORREF back = RGB(255, 255, 255), int size = -1, const char *face = 0);
	HWND	GetHwnd() const;
	HWND	GetParentHwnd() const;

private:

	HWND			m_scintilla;	// HWND for scintilla editor
	HWND			m_parent;		// HWND for onwer window
	EventHandlerT	m_notify;
	bool			m_eventsEnabled;

	// pointer to scintilla direct function
	ScintillaDirect m_direct;
	void*			m_sptr;


};

