#include "stdafx.h"

#include <algorithm>
#include <tchar.h>

#include "ScintillaManager.h"

CScintillaManager::CScintillaManager()
{
}

CScintillaManager::~CScintillaManager()
{
	Uninitialise();
}

void CScintillaManager::Initialise()
{
	m_editors.clear();

	HMODULE hLib = ::LoadLibrary(_T("SciLexer64.DLL"));

	if (hLib != 0)
	{
		OutputDebugString(_T("Loaded SCI Lexer DLL 64 bits"));
	}
	else
	{
		OutputDebugString(_T("*** Failed to load SCI Lexer DLL 64 bits ***"));
	}
}

void CScintillaManager::Uninitialise()
{
	for (EditorsT::iterator itr = m_editors.begin(); itr != m_editors.end(); ++itr)
	{
		(*itr)->Uninitialise();
	}
	m_editors.clear();
}

HWND CScintillaManager::NewEditor(HWND parent)
{
	if (parent == NULL) return NULL;

	// if parent already has a scintilla editor return it
	EditorsT::const_iterator itr = std::find_if(m_editors.begin(), m_editors.end(),
		[=](ScintillaEditorPtrT& ptrEditor) -> bool { return ptrEditor->GetParentHwnd() == parent; });
	if (itr != m_editors.end()) return (*itr)->GetHwnd();

	// start a new editor
	ScintillaEditorPtrT ptrEditor = ScintillaEditorPtrT(new CScintillaEditor);
	ptrEditor->Initialise(parent);

	// add to list and return it
	m_editors.push_back(ptrEditor);
	return ptrEditor->GetHwnd();
}

void CScintillaManager::DestroyEditor(HWND scintilla)
{
	EditorsT::const_iterator itr = std::find_if(m_editors.begin(), m_editors.end(),
		[=](ScintillaEditorPtrT& ptrEditor) -> bool { return ptrEditor->GetHwnd() == scintilla; });

	// uninitialise editor and remove it
	if (itr != m_editors.end())
	{
		(*itr)->Uninitialise();
		m_editors.erase(itr);
	}
}

LRESULT CScintillaManager::SendEditor(HWND scintilla, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	for (EditorsT::iterator itr = m_editors.begin(); itr != m_editors.end(); ++itr)
	{
		if ((*itr)->GetHwnd() == scintilla)
		{
			return (*itr)->SendEditor(Msg, wParam, lParam);
		}
	}
	return 0;
}

void CScintillaManager::SetEventHandler(HWND scintilla, CScintillaEditor::EventHandlerT callback)
{
	for (EditorsT::iterator itr = m_editors.begin(); itr != m_editors.end(); ++itr)
	{
		if ((*itr)->GetHwnd() == scintilla)
		{
			(*itr)->SetEventHandler(callback);
			return;
		}
	}
}

void CScintillaManager::EnableEvents(HWND scintilla)
{
	for (EditorsT::iterator itr = m_editors.begin(); itr != m_editors.end(); ++itr)
	{
		if ((*itr)->GetHwnd() == scintilla)
		{
			(*itr)->EnableEvents();
			return;
		}
	}
}

void CScintillaManager::DisableEvents(HWND scintilla)
{
	for (EditorsT::iterator itr = m_editors.begin(); itr != m_editors.end(); ++itr)
	{
		if ((*itr)->GetHwnd() == scintilla)
		{
			(*itr)->DisableEvents();
			return;
		}
	}
}

void CScintillaManager::AddPopupMenuItem(HWND scintilla, int id, char* title,
	CScintillaEditor::MenuHandlerT handler,
	CScintillaEditor::MenuEnabledT enabled)
{
	for (EditorsT::iterator itr = m_editors.begin(); itr != m_editors.end(); ++itr)
	{
		if ((*itr)->GetHwnd() == scintilla)
		{
			(*itr)->AddPopupMenuItem(id, title, handler, enabled);
			return;
		}
	}
}

void CScintillaManager::WndProcRetHook(LPCWPRETSTRUCT pData)
{
	// if the source of the message is either the editor or its parent
	// then pass it on
	for (EditorsT::iterator itr = m_editors.begin(); itr != m_editors.end(); ++itr)
	{
		if ((*itr)->GetHwnd() == pData->hwnd || (*itr)->GetParentHwnd() == pData->hwnd)
		{
			(*itr)->WndProcRetHook(pData);
		}
	}
}

void CScintillaManager::GetMsgProc(LPMSG pData)
{
	for (EditorsT::iterator itr = m_editors.begin(); itr != m_editors.end(); ++itr)
	{
		if ((*itr)->GetHwnd() == pData->hwnd || (*itr)->GetParentHwnd() == pData->hwnd)
		{
			(*itr)->GetMsgProc(pData);
		}
	}
}




