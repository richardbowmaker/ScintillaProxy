#pragma once

#include <vector>
#include <memory>

#include "ScintillaEditor.h"

class CScintillaManager
{
public:

	typedef std::shared_ptr<CScintillaEditor> ScintillaEditorPtrT;
	typedef std::vector<ScintillaEditorPtrT> EditorsT;

	CScintillaManager();
	~CScintillaManager();

	void	Initialise();
	void	Uninitialise();
	HWND	NewEditor(HWND parent);
	void	DestroyEditor(HWND scintilla);
	LRESULT SendEditor(HWND scintilla, UINT Msg, WPARAM wParam, LPARAM lParam);
	void	SetEventHandler(HWND scintilla, CScintillaEditor::EventHandlerT callback);
	void	EnableEvents(HWND scintilla);
	void	DisableEvents(HWND scintilla);
	void    AddPopupMenuItem(HWND scintilla, int id, char* title, CScintillaEditor::MenuHandlerT callback);

	void	WndProcRetHook(LPCWPRETSTRUCT pData);
	void    GetMsgProc(LPMSG pData);

private:

	EditorsT m_editors;
};

