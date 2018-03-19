#pragma once

#include <Scintilla.h>
#include <SciLexer.h>
#include <vector>

class CScintillaEditor
{
public:
	CScintillaEditor();
	~CScintillaEditor();

	typedef void* (*EventHandlerT)(SCNotification*);
	typedef void* (*MenuHandlerT)(int);
	typedef unsigned __int64(__cdecl *ScintillaDirect)(void*, int, __int64, unsigned __int64);

	HWND	Initialise(HWND parent);
	void	Uninitialise();
	LRESULT SendEditor(UINT Msg, WPARAM wParam = 0, LPARAM lParam = 0);
	void	SetEventHandler(EventHandlerT callback);
	void	EnableEvents();
	void	DisableEvents();
	void    AddPopupMenuItem(int id, char* title, MenuHandlerT callback);
	void    WndProcRetHook(LPCWPRETSTRUCT pData);
	void    GetMsgProc(LPMSG pData);
	void	ConfigureEditorHaskell();
	void    SetAStyle(int style, COLORREF fore, COLORREF back = RGB(255, 255, 255), int size = -1, const char *face = 0);
	HWND	GetHwnd() const;
	HWND	GetParentHwnd() const;

private:

	struct SMenu
	{
		int	id;
		MenuHandlerT notify;
	};
	typedef std::vector<SMenu> SMenusT;

	HWND			m_scintilla;	// HWND for scintilla editor
	HWND			m_parent;		// HWND for onwer window
	EventHandlerT	m_notify;
	bool			m_eventsEnabled;

	// pointer to scintilla direct function
	ScintillaDirect m_direct;
	void*			m_sptr;

	HMENU	m_popup;
	SMenusT m_menuItems;
};

