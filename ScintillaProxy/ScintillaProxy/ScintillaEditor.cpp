// ScintillaEditor.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"

#include <map>
#include <memory>
#include <Scintilla.h>
#include <SciLexer.h>
#include <tchar.h>
#include <Windows.h>

#include "ScintillaEditor.h"

CScintillaEditor::CScintillaEditor() :
	m_scintilla(NULL),
	m_parent(NULL),
	m_notify(NULL),
	m_eventsEnabled(false),
	m_direct(NULL),
	m_sptr(NULL)
{
}

CScintillaEditor::~CScintillaEditor()
{
	Uninitialise();
}

HWND CScintillaEditor::GetHwnd() const
{
	return m_scintilla;
}

HWND CScintillaEditor::GetParentHwnd() const
{
	return m_parent;
}

// start a new scintilla editor window
HWND CScintillaEditor::Initialise(HWND parent)
{
	HMODULE hMod = GetModuleHandle(NULL);

	HWND scintilla = ::CreateWindow(
		_T("Scintilla"),
		_T("Source"),
		WS_CHILD | WS_VSCROLL | WS_HSCROLL | WS_CLIPCHILDREN,
		0, 0,
		100, 100,
		parent,
		0,
		hMod,
		0);

	if (scintilla != NULL)
	{
		::ShowWindow(scintilla, SW_SHOW);

		m_parent = parent;
		m_scintilla = scintilla;

		// get pointer to scintilla direct function
		m_direct = (ScintillaDirect)SendMessage(scintilla, SCI_GETDIRECTFUNCTION, 0, 0);
		m_sptr = (void *)SendMessage(scintilla, SCI_GETDIRECTPOINTER, 0, 0);

		// set size of editor to fill parent window
		RECT r;
		::GetWindowRect(parent, &r);
		::MoveWindow(scintilla, 0, 0, r.right - r.left, r.bottom - r.top, TRUE);

		OutputDebugString(_T("::CreateWindow OK, Scintilla editor\n"));

		//		ConfigureEditorHaskell(scintilla);
	}
	else
	{
		OutputDebugString(_T("*** ::CreateWindow failed, Scintilla editor ***\n"));
	}

	return scintilla;
}

void CScintillaEditor::Uninitialise()
{
	if (m_scintilla)
	{
		DisableEvents();
		::CloseWindow(m_scintilla);
		::DestroyWindow(m_scintilla);
		m_scintilla = NULL;
	}
}

LRESULT CScintillaEditor::SendEditor(UINT Msg, WPARAM wParam, LPARAM lParam)
{
	return m_direct(m_sptr, Msg, wParam, lParam);
}

void CScintillaEditor::SetEventHandler(void* callback)
{
	if (callback == 0)
	{
		// to be safe
		m_eventsEnabled = false;
	}
	m_notify = (EventHandlerT)callback;
}

void CScintillaEditor::EnableEvents()
{
	m_eventsEnabled = true;
}

void CScintillaEditor::DisableEvents()
{
	m_eventsEnabled = false;
}

void CScintillaEditor::WndProcRetHook(int nCode, WPARAM wParam, LPARAM lParam)
{
	LPCWPRETSTRUCT pData = reinterpret_cast<LPCWPRETSTRUCT>(lParam);

	switch (pData->message)
	{
	case WM_NOTIFY:
		{
			// notify haskell
			if (m_eventsEnabled && m_notify != NULL)
			{
				LPNMHDR lpnmhdr = (LPNMHDR)pData->lParam;
				SCNotification* scn = (SCNotification*)pData->lParam;
				(m_notify)(scn);
			}
		}
		break;
	case WM_SIZE:
		{
			if (m_parent == pData->hwnd)
			{
				int w = (int)(pData->lParam) & 0x0ffff;
				int h = (int)(pData->lParam) >> 16;
				::MoveWindow(m_scintilla, 0, 0, w, h, TRUE);
			}
		}
		break;
	default:
		break;
	}
}

void CScintillaEditor::ConfigureEditorHaskell()
{
	const COLORREF red = RGB(0xFF, 0, 0);
	const COLORREF white = RGB(0xFF, 0xFF, 0xFF);
	const COLORREF black = RGB(0, 0, 0);
	const COLORREF offWhite = RGB(0xFF, 0xFB, 0xF0);
	const COLORREF darkGreen = RGB(0, 0x80, 0);
	const COLORREF darkBlue = RGB(0, 0, 0x80);
	const COLORREF lightBlue = RGB(0xA6, 0xCA, 0xF0);
	const COLORREF keyBlue = RGB(0x20, 0x50, 0xF0);
	const COLORREF stringBrown = RGB(0xA0, 0x10, 0x20);

	SendEditor(SCI_SETLEXER, SCLEX_HASKELL);
	//	SendEditor(SCI_SETSTYLEBITS, 7);

	const char keyWords[] = "do if else return then case import as qualified";

	SendEditor(SCI_SETKEYWORDS, 0,
		reinterpret_cast<LPARAM>(keyWords));

	// Set up the global default style. These attributes are used wherever no explicit choices are made.
	SetAStyle(STYLE_DEFAULT, stringBrown, white, 9, "Courier New");
	SendEditor(SCI_STYLECLEARALL);	// Copies global style to all others

												// Hypertext default is used for all the document's text
	SetAStyle(SCE_H_DEFAULT, stringBrown, white, 9, "Courier New");
	SetAStyle(SCE_HA_DEFAULT, black);
	SetAStyle(SCE_HA_IDENTIFIER, black);
	SetAStyle(SCE_HA_KEYWORD, keyBlue);
	SetAStyle(SCE_HA_NUMBER, black);
	SetAStyle(SCE_HA_STRING, stringBrown);
	SetAStyle(SCE_HA_CHARACTER, black);
	SetAStyle(SCE_HA_CLASS, black);
	SetAStyle(SCE_HA_MODULE, black);
	SetAStyle(SCE_HA_CAPITAL, black);
	SetAStyle(SCE_HA_DATA, black);
	SetAStyle(SCE_HA_IMPORT, keyBlue);
	SetAStyle(SCE_HA_OPERATOR, black);
	SetAStyle(SCE_HA_INSTANCE, black);
	SetAStyle(SCE_HA_COMMENTLINE, darkGreen);
	SetAStyle(SCE_HA_COMMENTBLOCK, darkGreen);
	SetAStyle(SCE_HA_COMMENTBLOCK2, darkGreen);
	SetAStyle(SCE_HA_COMMENTBLOCK3, darkGreen);
	SetAStyle(SCE_HA_PRAGMA, black);
	SetAStyle(SCE_HA_PREPROCESSOR, black);
	SetAStyle(SCE_HA_STRINGEOL, black);
	SetAStyle(SCE_HA_RESERVED_OPERATOR, black);
	SetAStyle(SCE_HA_LITERATE_COMMENT, black);
	SetAStyle(SCE_HA_LITERATE_CODEDELIM, black);
}

void CScintillaEditor::SetAStyle(int style, COLORREF fore, COLORREF back, int size, const char *face)
{
	SendEditor(SCI_STYLESETFORE, style, fore);
	SendEditor(SCI_STYLESETBACK, style, back);
	if (size >= 1)
		SendEditor(SCI_STYLESETSIZE, style, size);
	if (face)
		SendEditor(SCI_STYLESETFONT, style, reinterpret_cast<LPARAM>(face));
}





