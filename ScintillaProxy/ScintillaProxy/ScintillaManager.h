#pragma once

#include <vector>
#include <memory>

#include "ScintillaEditor.h"

class CScintillaManager
{
public:

	typedef std::shared_ptr<CScintillaEditor> ScintillaEditorPtrT;
	typedef std::vector<ScintillaEditorPtrT> EditorsT;

	static CScintillaManager& Instance();
	~CScintillaManager();

	bool	Initialise();
	void	Uninitialise();
	HWND	NewEditor(HWND parent);
	void	DestroyEditor(HWND scintilla);
	LRESULT SendEditor(HWND scintilla, UINT Msg, WPARAM wParam, LPARAM lParam);
	void	SetEventHandler(HWND scintilla, CScintillaEditor::EventHandlerT callback);
	void	EnableEvents(HWND scintilla);
	void	DisableEvents(HWND scintilla);
	void    AddPopupMenuItem(HWND scintilla, int id, char* title, 
				CScintillaEditor::MenuHandlerT handler,
				CScintillaEditor::MenuEnabledT enabled);

	void	WndProcRetHook(LPCWPRETSTRUCT pData);
	void    GetMsgProc(LPMSG pData);

private:

	CScintillaManager();
	EditorsT m_editors;
	HMODULE m_hdll;
};

