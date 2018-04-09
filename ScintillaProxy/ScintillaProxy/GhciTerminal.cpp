#include "stdafx.h"
#include <windows.h>
#include <Windowsx.h>
#include <tchar.h>
#include <stdio.h> 
#include <Commctrl.h>
#include <algorithm>

#include "GhciManager.h"
#include "GhciTerminal.h"

// rich text edit control, subclass win proc
LRESULT CALLBACK RichTextBoxProcFn(HWND hWnd, UINT uMsg, WPARAM wParam,
	LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	CGhciTerminal* p = reinterpret_cast<CGhciTerminal*>(dwRefData);
	if (p)
	{
		if (!p->RichTextBoxProc(hWnd, uMsg, wParam, lParam))
		{
			return DefSubclassProc(hWnd, uMsg, wParam, lParam);
		}
	}
	return 0;
}

void GhciEventHandlerFn(int id, const char* text, void* data)
{
	CGhciTerminal* p = reinterpret_cast<CGhciTerminal*>(data);
	if (p) p->GhciEventHandler(id, text, data);
}

CGhciTerminal::CGhciTerminal() :
	m_initialised(NULL),
	m_notify(NULL),
	m_eventsEnabled(false),
	m_hwnd(NULL),
	m_parent(NULL),
	m_noOfChars(0),
	m_hwndLookup(NULL),
	m_popup(NULL),
	m_ghci(NULL)
{
}

CGhciTerminal::~CGhciTerminal()
{
	Uninitialise();
}

bool CGhciTerminal::Initialise(HWND hParent, const char* options, const char* file)
{
	if (!m_initialised)
	{
		m_parent = hParent;
		m_hwnd = ::CreateWindowExW(0, MSFTEDIT_CLASS, L"",
			ES_MULTILINE | WS_HSCROLL | WS_VSCROLL | WS_VISIBLE | WS_CHILD | WS_TABSTOP,
			0, 0, 0, 0, hParent, NULL, NULL, NULL);

		if (m_hwnd)
		{
			RECT r;
			::GetClientRect(hParent, &r);
			::MoveWindow(m_hwnd, 0, 0, r.right - r.left, r.bottom - r.top, TRUE);

			_charformat cf;
			ZeroMemory(&cf, sizeof(cf));
			cf.cbSize = sizeof(_charformat);
			cf.dwMask = CFM_FACE | CFM_SIZE;
			cf.yHeight = 180;
			strncpy_s(cf.szFaceName, LF_FACESIZE, "Courier New", 12); // including terminating 0
			::SendMessage(m_hwnd, EM_SETCHARFORMAT, SCF_ALL | SCF_DEFAULT, (LPARAM)&cf);

			// subclass the rich edit text control
			::SetWindowSubclass(m_hwnd, RichTextBoxProcFn, 0, reinterpret_cast<DWORD_PTR>(this));

			// create the GHCI session
			m_ghci = CGhciManager::Instance().New(options, file);
			if (m_ghci != 0)
			{
				m_ghci->SetEventHandler(GhciEventHandlerFn, reinterpret_cast<void*>(this));
				m_initialised = true;
			}
			else
			{
				::CloseWindow(m_hwnd);
				::DestroyWindow(m_hwnd);
				m_hwnd = NULL;
			}
		}
	}
	return m_initialised;
}

void CGhciTerminal::Uninitialise()
{
	if (m_initialised)
	{
		m_ghci->Uninitialise();
		Notify(EventClosed);
		m_eventsEnabled = false;
		::SendMessage(m_hwnd, EM_SETEVENTMASK, 0, 0);

		HideLookup();

		if (m_popup)
		{
			::DestroyMenu(m_popup);
			m_popup = NULL;
		}

		if (m_hwnd)
		{
			::CloseWindow(m_hwnd);
			::DestroyWindow(m_hwnd);
			m_hwnd = NULL;
		}
		m_cmdHistory.clear();
		m_initialised = false;
	}
}

void CGhciTerminal::GhciEventHandler(int id, const char* text, void* data)
{
	CUtils::StringT str = CUtils::ToStringT(text);
	AddTextTS(str);
	m_noOfChars = GetNoOfChars();
	Notify(EventOutput, str);
}

void CGhciTerminal::ClearText()
{
	int ndx = GetWindowTextLength(m_hwnd);
	::SendMessage(m_hwnd, EM_SETSEL, (WPARAM)0, (LPARAM)ndx);
	::SendMessage(m_hwnd, EM_REPLACESEL, 0, (LPARAM)_T(""));
	m_noOfChars = 0;
}

void CGhciTerminal::SetText(CUtils::StringT text)
{
	ClearText();
	AddText(text);
}

void CGhciTerminal::AddText(CUtils::StringT text)
{
	int ndx = GetWindowTextLength(m_hwnd);
	::SendMessage(m_hwnd, EM_SETSEL, (WPARAM)ndx, (LPARAM)ndx);
	::SendMessage(m_hwnd, EM_REPLACESEL, 0, (LPARAM)text.c_str());
}

void CGhciTerminal::AddLine(CUtils::StringT text)
{
	AddText(text);
	NewLine();
}

void CGhciTerminal::NewLine()
{
	AddText(_T("\n"));
}

HWND CGhciTerminal::GetHwnd() const
{
	return m_hwnd;
}

HWND CGhciTerminal::GetParentHwnd() const
{
	return m_parent;
}

void CGhciTerminal::SetEventHandler(EventHandlerT callback)
{
	m_notify = callback;
}

void CGhciTerminal::EnableEvents()
{
	::SendMessage(m_hwnd, EM_SETEVENTMASK, 0, ENM_SELCHANGE);
	m_eventsEnabled = true;
}

void CGhciTerminal::DisableEvents()
{
	::SendMessage(m_hwnd, EM_SETEVENTMASK, 0, 0);
	m_eventsEnabled = false;
}

void CGhciTerminal::Paste()
{
	// if cursor position is before the prompt move it to
	// the end of the doc
	_charrange pos;
	::SendMessage(m_hwnd, EM_EXGETSEL, 0, reinterpret_cast<LPARAM>(&pos));
	if (pos.cpMin < m_noOfChars)
	{
		int ndx = GetWindowTextLength(m_hwnd);
		::SendMessage(m_hwnd, EM_SETSEL, (WPARAM)ndx, (LPARAM)ndx);
	}

	// paste text 
	::SendMessage(m_hwnd, WM_PASTE, 0, 0);
}

void CGhciTerminal::Cut()
{
	// can only cut if at end of doc
	_charrange pos;
	::SendMessage(m_hwnd, EM_EXGETSEL, 0, reinterpret_cast<LPARAM>(&pos));
	if (pos.cpMax < m_noOfChars)
	{
		return;
	}
	else if (pos.cpMin < m_noOfChars)
	{
		::SendMessage(m_hwnd, EM_SETSEL, (WPARAM)m_noOfChars, (LPARAM)pos.cpMax);
	}
	::SendMessage(m_hwnd, WM_CUT, 0, 0);
}

void CGhciTerminal::SelectAll()
{
	int ndx = GetWindowTextLength(m_hwnd);
	::SendMessage(m_hwnd, EM_SETSEL, 0, (LPARAM)ndx);
}
	
void CGhciTerminal::Copy()
{
	::SendMessage(m_hwnd, WM_COPY, 0, 0);
}

// non-blocking, thread safe
void CGhciTerminal::AddTextTS(CUtils::StringT text)
{
	TCHAR* p = _tcsdup(text.c_str());
	::SendMessage(m_hwnd, TW_ADDTEXT, 0, reinterpret_cast<LPARAM>(p));
}

CUtils::StringT CGhciTerminal::GetCommandLine()
{
	int n = GetWindowTextLength(m_hwnd);
	if (n > m_noOfChars)
	{
		_textrange tr;
		wchar_t buff[1000];
		tr.chrg.cpMin = m_noOfChars;
		tr.chrg.cpMax = n;
		tr.lpstrText = (char*)buff;
		::SendMessage(m_hwnd, EM_GETTEXTRANGE, 0, (LPARAM)&tr);
		CUtils::StringT cmd = buff;
		CUtils::StringT::iterator it = std::remove_if(cmd.begin(), cmd.end(), [=](TCHAR& c) -> bool { return c == _T('\r'); });
		cmd.erase(it, cmd.end());
		return cmd;
	}
	else
	{
		return _T("");
	}
}

void CGhciTerminal::Notify(int event, CUtils::StringT text)
{
	if (m_eventsEnabled && m_notify != NULL)
	{
		switch (event)
		{
		case EventOutput:
		case EventInput:
		{
			// copy text to heap
			std::string str = CUtils::ToChar(text);
			m_notify(m_hwnd, event, str.c_str());
		}
			break;
		default:
			m_notify(m_hwnd, event, NULL);
		}
	}
}

bool CGhciTerminal::RichTextBoxProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_SETFOCUS:
		Notify(EventGotFocus);
		break;
	case WM_KILLFOCUS:
		Notify(EventLostFocus);
		break;
	case TW_ADDTEXT:
		if (lParam)
		{
			TCHAR* p = reinterpret_cast<TCHAR*>(lParam);
			AddText(p);
			free(p);
		}
		break;
	case WM_CHAR:
		switch (wParam)
		{
		case VK_SPACE:
		{
			// Ctrl-Space toggles the command line history list box
			if ((GetKeyState(VK_CONTROL) & 0x80000) > 0)
			{
				ToggleLookup();
				return true;
			}
		}
		break;
		case VK_RETURN:
		{
			CUtils::StringT cmd = GetCommandLine();
			SendCommand(cmd);
			AddText(_T("\n"));
			m_noOfChars = GetNoOfChars();
		}
		return true;
		}
		break;
	case WM_KEYDOWN:
	{
		HideLookup();

		if (wParam >= VK_SPACE)
		{
			// if cursor position is before the prompt move it to
			// the end of the doc
			_charrange pos;
			::SendMessage(m_hwnd, EM_EXGETSEL, 0, reinterpret_cast<LPARAM>(&pos));
			if (pos.cpMin < m_noOfChars)
			{
				int ndx = ::GetWindowTextLength(m_hwnd);
				::SendMessage(m_hwnd, EM_SETSEL, (WPARAM)ndx, (LPARAM)ndx);
			}
		}

		switch (wParam)
		{
		case VK_UP:
			if (m_cmdHistory.size() > 0)
			{
				if (m_hix == -1)
				{
					m_hix = (int)m_cmdHistory.size() - 1;
				}
				else
				{
					m_hix = (m_hix + ((int)m_cmdHistory.size() - 1)) % (int)m_cmdHistory.size();
				}
				UpdateCommandLine(m_cmdHistory[m_hix]);
			}
			return true;
		case VK_DOWN:
		{
			if (m_cmdHistory.size() > 0)
			{
				if (m_hix == -1)
				{
					m_hix = 0;
				}
				else
				{
					m_hix = (m_hix + 1) % m_cmdHistory.size();
				}
				UpdateCommandLine(m_cmdHistory[m_hix]);
			}
			return true;
		}
		case VK_ESCAPE:
			HideLookup();
			UpdateCommandLine(_T(""));
			return true;
		case VK_BACK:
		{
			// prevent backspacing out the prompt
			_charrange pos;
			::SendMessage(m_hwnd, EM_EXGETSEL, 0, reinterpret_cast<LPARAM>(&pos));
			if (pos.cpMax <= m_noOfChars)
			{
				return true;
			}
			else if (pos.cpMin < m_noOfChars)
			{
				// modify selected text so it doesn't include the prompt
				::SendMessage(m_hwnd, EM_SETSEL, (WPARAM)m_noOfChars, (LPARAM)pos.cpMax);
			}
		}
		default:
			break;
		}
	}
	break;
	default:
		break;
	}
	return false;
}

void CGhciTerminal::WndProcRetHook(LPCWPRETSTRUCT pData)
{
	if (m_parent == pData->hwnd)
	{
		switch (pData->message)
		{
		case WM_CONTEXTMENU:
		{
			if (m_popup)
			{
				int x = GET_X_LPARAM(pData->lParam);
				int y = GET_Y_LPARAM(pData->lParam);
				::TrackPopupMenu(m_popup, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON, x, y, 0, m_parent, 0);
			}
		}
		break;
		case WM_NOTIFY:
		{
			LPNMHDR lpnmhdr = (LPNMHDR)pData->lParam;
			if (lpnmhdr->code == EN_SELCHANGE)
			{
				// notify selection change, only if mouse left button up
				// avoid sending lots of spurious notifications
				if ((GetAsyncKeyState(VK_LBUTTON) & 0x80) == 0)
				{
					if (IsTextSelected())
					{
						Notify(EventSelectionSet);
					}
					else
					{
						Notify(EventSelectionClear);
					}
				}
			}
		}
		break;
		case WM_SIZE:
		{
			int w = (int)(pData->lParam) & 0x0ffff;
			int h = (int)(pData->lParam) >> 16;
			::MoveWindow(m_hwnd, 0, 0, w, h, TRUE);
		}
		break;
		default:
			break;
		}
	}
}

void CGhciTerminal::GetMsgProc(LPMSG pData)
{
	if (m_parent == pData->hwnd)
	{
		switch (pData->message)
		{
		case WM_COMMAND:
		{
			int id = LOWORD(pData->wParam);
			if (id == 1000)
			{
				int n = 0;
			}
		}
		break;
		default:
			break;
		}
	}
}

// remove current display of command line and show latest 
void CGhciTerminal::UpdateCommandLine(CUtils::StringT text)
{
	int ndx = GetWindowTextLength(m_hwnd);
	::SendMessage(m_hwnd, EM_SETSEL, (WPARAM)m_noOfChars, (LPARAM)ndx);
	::SendMessage(m_hwnd, EM_REPLACESEL, 0, (LPARAM)text.c_str());
}

// send a command to the process
void CGhciTerminal::SendCommand(CUtils::StringT text)
{
	m_ghci->SendCommand(text);

	// update the command history
	if (text.size() > 0)
	{
		if (m_cmdHistory.size() == 0)
		{
			m_cmdHistory.push_back(text);
		}
		else if (!CUtils::StringStartsWith(m_cmdHistory[m_cmdHistory.size() - 1], text))
		{
			m_cmdHistory.push_back(text);
		}
	}
	m_hix = -1;

	Notify(EventInput, text + _T("\n"));
}

void CGhciTerminal::SendCommand(char* cmd)
{
	m_ghci->SendCommand(cmd);
	Notify(EventInput, CUtils::ToStringT(cmd) + _T("\n"));
}

bool CGhciTerminal::IsTextSelected()
{
	_charrange pos;
	::SendMessage(m_hwnd, EM_EXGETSEL, 0, reinterpret_cast<LPARAM>(&pos));
	return (pos.cpMin != pos.cpMax);
}

bool CGhciTerminal::HasFocus()
{
	return m_hwnd == ::GetFocus();
}

void CGhciTerminal::SetFocus()
{
	::SetFocus(m_hwnd);
}

int CGhciTerminal::GetTextLength()
{
	GETTEXTLENGTHEX tl;
	tl.flags = GTL_USECRLF | GTL_NUMCHARS;
	tl.codepage = CP_ACP;
	return (int) ::SendMessage(m_hwnd, EM_GETTEXTLENGTHEX, (WPARAM)&tl, 0);
}

int CGhciTerminal::GetText(char* buff, int size)
{
	int n = GetTextLength();
	int s = 1 + min(size, n);
	GETTEXTEX gt;
	gt.cb = s;
	gt.flags = GT_USECRLF;
	gt.codepage = CP_ACP;
	gt.lpDefaultChar = NULL;
	gt.lpUsedDefChar = NULL;
	int nc = (int)::SendMessage(m_hwnd, EM_GETTEXTEX, (WPARAM)&gt, (LPARAM)buff);
	return nc;
}

void CGhciTerminal::Clear()
{
	ClearText();
	SendCommand("");
}

int CGhciTerminal::GetNoOfChars()
{
	_charrange pos;
	::SendMessage(m_hwnd, EM_EXGETSEL, 0, reinterpret_cast<LPARAM>(&pos));
	return pos.cpMin;
}


//---------------------------------------------------------------
// Command line list box handling
//---------------------------------------------------------------

// rich text edit control, subclass win proc
LRESULT CALLBACK ListBoxProcFn(HWND hWnd, UINT uMsg, WPARAM wParam,
	LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	CGhciTerminal* p = reinterpret_cast<CGhciTerminal*>(dwRefData);
	if (p)
	{
		if (!p->ListBoxProc(uMsg, wParam, lParam))
		{
			return DefSubclassProc(hWnd, uMsg, wParam, lParam);
		}
	}
	return 0;
}

bool CGhciTerminal::ListBoxProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_LBUTTONUP:
		SetCommandLineFromList();
		return true;
	case WM_LBUTTONDBLCLK:
		SetCommandLineFromList();
		HideLookup();
		return true;
	case WM_CHAR:
		if (wParam == VK_RETURN)
		{
			SetCommandLineFromList();
			HideLookup();
			return true;
		}
		break;
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE)
		{
			HideLookup();
			return true;
		}
		break;
	}
	return false;
}

void CGhciTerminal::ShowLookup()
{
	if (m_hwndLookup == NULL)
	{
		// filter command history on start of current command
		CUtils::StringT s1 = GetCommandLine();
		std::transform(s1.begin(), s1.end(), s1.begin(), ::toupper);
		std::vector<CUtils::StringsT::const_iterator> filtered;
		for (CUtils::StringsT::const_iterator it = m_cmdHistory.begin(); it != m_cmdHistory.end(); ++it)
		{
			CUtils::StringT s2 = *it;
			std::transform(s2.begin(), s2.end(), s2.begin(), ::toupper);
			s2 = s2.substr(0, s1.size());
			if (s1 == s2) filtered.push_back(it);
		}

		if (filtered.size() > 0)
		{
			m_hwndLookup = CreateWindowExW(0, L"ListBox",
				NULL, WS_CHILD | WS_BORDER | WS_VSCROLL | WS_HSCROLL | WS_VISIBLE,
				0, 0, 0, 0, m_hwnd, NULL, NULL, NULL);

			RECT r;
			::GetClientRect(m_hwnd, &r);
			::MoveWindow(m_hwndLookup, (r.right - r.left) / 2, 0, (r.right - r.left) / 2, (r.bottom - r.top) / 2, TRUE);

			for (std::vector<CUtils::StringsT::const_iterator>::const_iterator it = filtered.begin(); it != filtered.end(); ++it)
			{
				int ix = (int)::SendMessage(m_hwndLookup, LB_ADDSTRING, 0, (LPARAM)(*it)->c_str());
				::SendMessage(m_hwndLookup, LB_SETITEMDATA, ix, (LPARAM)(*it)->c_str());
			}
			::SetWindowSubclass(m_hwndLookup, ListBoxProcFn, 0, reinterpret_cast<DWORD_PTR>(this));
			::SetFocus(m_hwndLookup);
		}
	}
}

void CGhciTerminal::HideLookup()
{
	if (m_hwndLookup)
	{
		::CloseWindow(m_hwndLookup);
		::DestroyWindow(m_hwndLookup);
		::RedrawWindow(m_hwnd, NULL, NULL, RDW_INTERNALPAINT);
		m_hwndLookup = NULL;
	}
}

void CGhciTerminal::ToggleLookup()
{
	if (m_hwndLookup)
	{
		HideLookup();
	}
	else
	{
		ShowLookup();
	}
}

void CGhciTerminal::SetCommandLineFromList()
{
	unsigned int ix = (unsigned int)::SendMessage(m_hwndLookup, LB_GETCURSEL, 0, 0);
	if (ix != LB_ERR)
	{
		TCHAR* p = reinterpret_cast<TCHAR*>(::SendMessage(m_hwndLookup, LB_GETITEMDATA, ix, 0));
		if (p)
		{
			UpdateCommandLine(p);
		}
	}
}








