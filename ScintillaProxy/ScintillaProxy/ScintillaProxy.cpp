// ScintillaProxy.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "ScintillaProxy.h"
#include <Scintilla.h>
#include <SciLexer.h>
#include <Windows.h>


#include <map>
#include <memory>
#include <tchar.h>

const COLORREF red = RGB(0xFF, 0, 0);
const COLORREF offWhite = RGB(0xFF, 0xFB, 0xF0);
const COLORREF darkGreen = RGB(0, 0x80, 0);
const COLORREF darkBlue = RGB(0, 0, 0x80);
const COLORREF black = RGB(0, 0, 0);
const COLORREF white = RGB(0xff, 0xff, 0xff);


// pointer to haskell event handler
typedef void* (*EventHandlerT)(SCNotification*);
typedef int(__cdecl *ScintillaDirect)(void*, int, int, int);

// foir each attached scintilla editor window
class SEditor
{
public:

	SEditor() : m_scintilla(0), m_parent(0), m_notify(NULL) {}

	HWND	m_scintilla;	// HWND for scintilla editor
	HWND	m_parent;		// HWND for onwer window
	EventHandlerT m_notify;

	// pointer to scintilla direct function
	ScintillaDirect direct;
	void * sptr;

};

typedef std::shared_ptr<SEditor> SEditorPtrT;
typedef std::map<HWND, SEditorPtrT> EditorsT;

EditorsT	editors;				// active editord
int			NoOfEventHandlers;		// no. of haskell clients requesting events
HHOOK		winHook;				// the windows hook to capture scintilla WM_NOTIFY messages

// Initialise the DLL
void Initialise()
{
	editors.clear();
	NoOfEventHandlers = 0;
	winHook = 0;

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

void Uninitialise()
{
	if (winHook != 0)
	{
		UnhookWindowsHookEx(winHook);
		winHook = 0;
	}

	editors.clear();
	NoOfEventHandlers = 0;
}

// start a new scintilla editor window
HWND ScnNewEditor(HWND parent)
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

	if (scintilla != 0)
	{
//		ConfigureEditor();

		::ShowWindow(scintilla, SW_SHOW);

		SEditorPtrT pe = SEditorPtrT(new SEditor);
		pe->m_parent = parent;
		pe->m_scintilla = scintilla;
		pe->m_notify = NULL;

		// get pointer to scintilla direct function
		pe->direct = (ScintillaDirect)SendMessage(scintilla, SCI_GETDIRECTFUNCTION, 0, 0);
		pe->sptr = (void *)SendMessage(scintilla, SCI_GETDIRECTPOINTER, 0, 0);

		// set size of editor to fill parent window
		RECT r;
		::GetWindowRect(parent, &r);
		::SetWindowPos(scintilla, parent, 0, 0, r.right - r.left, r.bottom - r.top, SWP_SHOWWINDOW);

		editors.insert(std::make_pair(scintilla, pe));

		OutputDebugString(_T("::CreateWindow OK, Scintilla editor\n"));
	}
	else
	{
		OutputDebugString(_T("*** ::CreateWindow failed, Scintilla editor ***\n"));
	}

	return scintilla;
}

void ScnDestroyEditor(HWND scintilla)
{
	EditorsT::iterator it = editors.find(scintilla);

	if (it != editors.end())
	{
		SEditorPtrT pe = it->second;

		//????? To do

	}
}

LRESULT ScnSendEditor(HWND scintilla, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if (Msg == SCI_SETTEXT)
	{
		char* p = (char*)lParam;
		int n = 0;
	}

	LRESULT res = -1;
	EditorsT::iterator it = editors.find(scintilla);

	if (it != editors.end())
	{
		SEditorPtrT pe = it->second;
		res = pe->direct(pe->sptr, Msg, wParam, lParam);
	}
	return res;
}

LRESULT SendEditor(HWND scintilla, UINT Msg, WPARAM wParam, LPARAM lParam)
{

	return 0;
	//return ScnSendEditor(scintilla, Msg, wParam, lParam);
}

BOOL ScnEnableEvents(HWND scintilla, void* callback)
{
	BOOL res = FALSE;
	EditorsT::iterator it = editors.find(scintilla);

	if (it != editors.end())
	{
		SEditorPtrT pe = it->second;

		// add windows hook if this is the first event handler
		if (NoOfEventHandlers == 0)
		{
			winHook = SetWindowsHookEx(WH_CALLWNDPROCRET, &WndProcRetHook, 0, ::GetCurrentThreadId());
		}
		if (winHook != 0)
		{
			NoOfEventHandlers++;
			pe->m_notify = (EventHandlerT)callback;
			res = TRUE;

			OutputDebugString(_T("Windows hook setup ok\n"));
		}
	}
	return res;
}

void ScnDisableEvents(HWND scintilla)
{
	EditorsT::iterator it = editors.find(scintilla);

	if (it != editors.end())
	{
		SEditorPtrT pe = it->second;
		pe->m_notify = 0;

		NoOfEventHandlers--;

		// release windows hook if this is the first event handler
		if (NoOfEventHandlers <= 0)
		{
			::UnhookWindowsHookEx(winHook);
			winHook = 0;
		}
	}
}

LRESULT WINAPI WndProcRetHook(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode < 0)
	{
		return CallNextHookEx(winHook, nCode, wParam, lParam);
	}

	LPCWPRETSTRUCT pData = reinterpret_cast<LPCWPRETSTRUCT>(lParam);

	if (pData->message == WM_NOTIFY)
	{
		LPNMHDR lpnmhdr = (LPNMHDR)pData->lParam;

		// is the notification from a scintilla editor window ?
		EditorsT::iterator it = editors.find(lpnmhdr->hwndFrom);

		if (it != editors.end())
		{
			// pass notification onto to event handler
			SEditorPtrT pe = it->second;
			SCNotification* scn = (SCNotification*)pData->lParam;

			// notify haskell
			if (pe->m_notify != NULL)
			{
				(pe->m_notify)(scn);
			}
		}
	}
	else if (pData->message == WM_SIZE)
	{
		// resize scintilla editor after its parent has been resized
		for (EditorsT::const_iterator it = editors.begin(); it != editors.end(); ++it)
		{
			if (it->second->m_parent == pData->hwnd)
			{
				int w = (pData->lParam) & 0x0ffff;
				int h = (pData->lParam) >> 16;
				::MoveWindow(it->second->m_scintilla, 0, 0, w, h, TRUE);
			}
		}
	}
	return CallNextHookEx(winHook, nCode, wParam, lParam);
}

void ConfigureEditor(HWND scintilla)
{
	SendEditor(scintilla, SCI_SETLEXER, SCLEX_HTML);
	SendEditor(scintilla, SCI_SETSTYLEBITS, 7);

	const TCHAR keyWords[] =
		_T("break case catch continue default ")
		_T("do else for function if return throw try var while");

	SendEditor(scintilla, SCI_SETKEYWORDS, 0,
		reinterpret_cast<LPARAM>(keyWords));

	// Set up the global default style. These attributes are used wherever no explicit choices are made.
	SetAStyle(scintilla, STYLE_DEFAULT, black, white, 9, _T("Consolas"));
	SendEditor(scintilla, SCI_STYLECLEARALL);	// Copies global style to all others

	const COLORREF red = RGB(0xFF, 0, 0);
	const COLORREF offWhite = RGB(0xFF, 0xFB, 0xF0);
	const COLORREF darkGreen = RGB(0, 0x80, 0);
	const COLORREF darkBlue = RGB(0, 0, 0x80);

	// Hypertext default is used for all the document's text
	SetAStyle(scintilla, SCE_H_DEFAULT, black, white, 9, _T("Consolas"));

	// Unknown tags and attributes are highlighed in red. 
	// If a tag is actually OK, it should be added in lower case to the htmlKeyWords string.
	SetAStyle(scintilla, SCE_H_TAG, darkBlue);
	SetAStyle(scintilla, SCE_H_TAGUNKNOWN, red);
	SetAStyle(scintilla, SCE_H_ATTRIBUTE, darkBlue);
	SetAStyle(scintilla, SCE_H_ATTRIBUTEUNKNOWN, red);
	SetAStyle(scintilla, SCE_H_NUMBER, RGB(0x80, 0, 0x80));
	SetAStyle(scintilla, SCE_H_DOUBLESTRING, RGB(0, 0x80, 0));
	SetAStyle(scintilla, SCE_H_SINGLESTRING, RGB(0, 0x80, 0));
	SetAStyle(scintilla, SCE_H_OTHER, RGB(0x80, 0, 0x80));
	SetAStyle(scintilla, SCE_H_COMMENT, RGB(0x80, 0x80, 0));
	SetAStyle(scintilla, SCE_H_ENTITY, RGB(0x80, 0, 0x80));

	SetAStyle(scintilla, SCE_H_TAGEND, darkBlue);
	SetAStyle(scintilla, SCE_H_XMLSTART, darkBlue);	// <?
	SetAStyle(scintilla, SCE_H_XMLEND, darkBlue);		// ?>
	SetAStyle(scintilla, SCE_H_SCRIPT, darkBlue);		// <script
	SetAStyle(scintilla, SCE_H_ASP, RGB(0x4F, 0x4F, 0), RGB(0xFF, 0xFF, 0));	// <% ... %>
	SetAStyle(scintilla, SCE_H_ASPAT, RGB(0x4F, 0x4F, 0), RGB(0xFF, 0xFF, 0));	// <%@ ... %>

	SetAStyle(scintilla, SCE_HB_DEFAULT, black);
	SetAStyle(scintilla, SCE_HB_COMMENTLINE, darkGreen);
	SetAStyle(scintilla, SCE_HB_NUMBER, RGB(0, 0x80, 0x80));
	SetAStyle(scintilla, SCE_HB_WORD, darkBlue);
	SendEditor(scintilla, SCI_STYLESETBOLD, SCE_HB_WORD, 1);
	SetAStyle(scintilla, SCE_HB_STRING, RGB(0x80, 0, 0x80));
	SetAStyle(scintilla, SCE_HB_IDENTIFIER, black);

	// This light blue is found in the windows system palette so is safe to use even in 256 colour modes.
	const COLORREF lightBlue = RGB(0xA6, 0xCA, 0xF0);
	// Show the whole section of VBScript with light blue background
	for (int bstyle = SCE_HB_DEFAULT; bstyle <= SCE_HB_STRINGEOL; bstyle++)
	{
		SendEditor(scintilla, SCI_STYLESETFONT, bstyle,
			reinterpret_cast<LPARAM>("Georgia"));
		SendEditor(scintilla, SCI_STYLESETBACK, bstyle, lightBlue);
		// This call extends the backround colour of the last style on the line to the edge of the window
		SendEditor(scintilla, SCI_STYLESETEOLFILLED, bstyle, 1);
	}
	SendEditor(scintilla, SCI_STYLESETBACK, SCE_HB_STRINGEOL, RGB(0x7F, 0x7F, 0xFF));
	SendEditor(scintilla, SCI_STYLESETFONT, SCE_HB_COMMENTLINE,
		reinterpret_cast<LPARAM>("Comic Sans MS"));

	SetAStyle(scintilla, SCE_HBA_DEFAULT, black);
	SetAStyle(scintilla, SCE_HBA_COMMENTLINE, darkGreen);
	SetAStyle(scintilla, SCE_HBA_NUMBER, RGB(0, 0x80, 0x80));
	SetAStyle(scintilla, SCE_HBA_WORD, darkBlue);
	SendEditor(scintilla, SCI_STYLESETBOLD, SCE_HBA_WORD, 1);
	SetAStyle(scintilla, SCE_HBA_STRING, RGB(0x80, 0, 0x80));
	SetAStyle(scintilla, SCE_HBA_IDENTIFIER, black);

	// Show the whole section of ASP VBScript with bright yellow background
	for (int bastyle = SCE_HBA_DEFAULT; bastyle <= SCE_HBA_STRINGEOL; bastyle++)
	{
		SendEditor(scintilla, SCI_STYLESETFONT, bastyle,
			reinterpret_cast<LPARAM>("Georgia"));
		SendEditor(scintilla, SCI_STYLESETBACK, bastyle, RGB(0xFF, 0xFF, 0));
		// This call extends the backround colour of the last style on the line to the edge of the window
		SendEditor(scintilla, SCI_STYLESETEOLFILLED, bastyle, 1);
	}
	SendEditor(scintilla, SCI_STYLESETBACK, SCE_HBA_STRINGEOL, RGB(0xCF, 0xCF, 0x7F));
	SendEditor(scintilla, SCI_STYLESETFONT, SCE_HBA_COMMENTLINE,
		reinterpret_cast<LPARAM>("Comic Sans MS"));


}

void SetAStyle(HWND scintilla, int style, COLORREF fore, COLORREF back, int size, const TCHAR *face)
{
	SendEditor(scintilla, SCI_STYLESETFORE, style, fore);
	SendEditor(scintilla, SCI_STYLESETBACK, style, back);
	if (size >= 1)
		SendEditor(scintilla, SCI_STYLESETSIZE, style, size);
	if (face)
		SendEditor(scintilla, SCI_STYLESETFONT, style, reinterpret_cast<LPARAM>(face));
}





