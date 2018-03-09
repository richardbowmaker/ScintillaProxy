#include "stdafx.h"
#include<windows.h>
#include <tchar.h>
#include <stdio.h> 
#include <Commctrl.h>
#include <algorithm>
#include <string>

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

CGhciTerminal::CGhciTerminal() :
	m_mgr(NULL),
	m_notify(NULL),
	m_eventsEnabled(false),
	m_hwnd(NULL),
	m_parent(NULL),
	m_hChildProcess(NULL),
	m_hInputWrite(NULL),
	m_hOutputRead(NULL),
	m_noOfChars(0),
	m_hwndLookup(NULL)
{
}

CGhciTerminal::~CGhciTerminal()
{
	Uninitialise();
}

bool CGhciTerminal::Initialise(CGhciManager* mgr, HWND hParent, char* options, char* file)
{
	m_mgr = mgr;
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

		StartCommand(options, file);
		return true;
	}
	else
	{
		return false;
	}
}

void CGhciTerminal::Uninitialise()
{
	Notify(EventClosed);
	m_eventsEnabled = false;
	::SendMessage(m_hwnd, EM_SETEVENTMASK, 0, 0);

	if (m_hChildProcess)
	{
		SendCommand(_T(":quit\n"));
		m_hChildProcess = NULL;
	}

	HideLookup();

	if (m_hwnd)
	{
		::CloseWindow(m_hwnd);
		::DestroyWindow(m_hwnd);
		m_hwnd = NULL;
	}
	m_cmdHistory.clear();
}


void CGhciTerminal::ClearText()
{
	int ndx = GetWindowTextLength(m_hwnd);
	::SendMessage(m_hwnd, EM_SETSEL, (WPARAM)0, (LPARAM)ndx);
	::SendMessage(m_hwnd, EM_REPLACESEL, 0, (LPARAM)_T(""));
	m_noOfChars = 0;
}

void CGhciTerminal::SetText(StringT text)
{
	ClearText();
	AddText(text);
}

void CGhciTerminal::AddText(StringT text)
{
	int ndx = GetWindowTextLength(m_hwnd);
	::SendMessage(m_hwnd, EM_SETSEL, (WPARAM)ndx, (LPARAM)ndx);
	::SendMessage(m_hwnd, EM_REPLACESEL, 0, (LPARAM)text.c_str());
}

void CGhciTerminal::AddLine(StringT text)
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
	if (pos.cpMin >= m_noOfChars)
	{
		::SendMessage(m_hwnd, WM_CUT, 0, 0);
	}
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
void CGhciTerminal::AddTextTS(StringT text)
{
	TCHAR* p = _tcsdup(text.c_str());
	::SendMessage(m_hwnd, TW_ADDTEXT, 0, reinterpret_cast<LPARAM>(p));
}

CGhciTerminal::StringT CGhciTerminal::GetCommandLine()
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
		StringT cmd = buff;
		StringT::iterator it = std::remove_if(cmd.begin(), cmd.end(), [=](TCHAR& c) -> bool { return c == _T('\r'); });
		cmd.erase(it, cmd.end());
		return cmd;
	}
	else
	{
		return _T("");
	}
}

void CGhciTerminal::Notify(int event)
{
	if (m_eventsEnabled && m_notify != NULL)
	{
		m_notify(m_hwnd, event);
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
			StringT cmd = GetCommandLine();			
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
			else if (pos.cpMax != pos.cpMin)
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

void CGhciTerminal::WndProcRetHook(int nCode, WPARAM wParam, LPARAM lParam)
{
	LPCWPRETSTRUCT pData = reinterpret_cast<LPCWPRETSTRUCT>(lParam);

	if (m_parent == pData->hwnd)
	{
		switch (pData->message)
		{			
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

// remove current display of command line and show latest 
void CGhciTerminal::UpdateCommandLine(StringT text)
{
	int ndx = GetWindowTextLength(m_hwnd);
	::SendMessage(m_hwnd, EM_SETSEL, (WPARAM)m_noOfChars, (LPARAM)ndx);
	::SendMessage(m_hwnd, EM_REPLACESEL, 0, (LPARAM)text.c_str());
}

// send a command to the process
void CGhciTerminal::SendCommand(StringT text)
{
	DWORD nBytesWrote;
	std::string cmd = ToChar(text) + std::string("\n");

	WriteFile(m_hInputWrite, cmd.c_str(), (DWORD)cmd.size(), &nBytesWrote, NULL);

	// update the command history
	if (text.size() > 0)
	{
		if (m_cmdHistory.size() == 0)
		{
			m_cmdHistory.push_back(text);
		}
		else if (!StringStartsWith(m_cmdHistory[m_cmdHistory.size() - 1], text))
		{
			m_cmdHistory.push_back(text);
		}
	}
	m_hix = -1;
}

void CGhciTerminal::SendCommand(char* cmd)
{
	DWORD nBytesWrote;
	std::string s = cmd + std::string("\n");

	WriteFile(m_hInputWrite, s.c_str(), (DWORD)s.size(), &nBytesWrote, NULL);
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
	return ::GetWindowTextLength(m_hwnd);
}

 int CGhciTerminal::GetText(char* buff, int size)
{
	int n = min(size - 1, ::GetWindowTextLength(m_hwnd));
	TCHAR* p = (TCHAR*)::malloc((n + 1) * sizeof(TCHAR));
	if (p)
	{
		_textrange tr;
		tr.chrg.cpMin = 0;
		tr.chrg.cpMax = n;
		tr.lpstrText = (char*)p;
		::SendMessage(m_hwnd, EM_GETTEXTRANGE, 0, (LPARAM)&tr);
		StringT str(p);
		std::replace(str.begin(), str.end(), _T('\r'), _T('\n'));

		std::string text = ToChar(str);
		strncpy_s(buff, n + 1, text.c_str(), text.size());
		::free(p);
		return text.size();
	}
	else
	{
		return 0;
	}
}

//-----------------------------------------------------------
// helpers
//-----------------------------------------------------------

bool CGhciTerminal::StringStartsWith(StringT str, StringT start)
{
	if (str.size() == 0 || start.size() == 0) return false;
	StringT s = str.substr(0, start.size());
	std::transform(s.begin(), s.end(), s.begin(), ::toupper);
	std::transform(start.begin(), start.end(), start.begin(), ::toupper);
	return s == start;
}

CGhciTerminal::StringT CGhciTerminal::StringRemoveAt(StringT str, unsigned int start, unsigned int len)
{
	StringT s1, s2;
	if (start > 0 && start < str.size()) s1 = str.substr(0, start);
	if (start + len > 0 && start + len < str.size()) s2 = str.substr(start + len);
	return s1 + s2;
}

int CGhciTerminal::GetNoOfChars()
{
	_charrange pos;
	::SendMessage(m_hwnd, EM_EXGETSEL, 0, reinterpret_cast<LPARAM>(&pos));
	return pos.cpMin;
}

CGhciTerminal::StringT CGhciTerminal::ToStringT(char* p)
{
#ifdef _UNICODE
	wchar_t wBuff[1000];
	int n = MultiByteToWideChar(CP_ACP, MB_COMPOSITE, p, (int)strlen(p), wBuff, sizeof(wBuff));
	return StringT(wBuff, n);
#else
	return StringT(p);
#endif
}

CGhciTerminal::StringT CGhciTerminal::ToStringT(wchar_t* p)
{
#ifdef _UNICODE
	return StringT(p);
#else
	char buff[1000];
	int n = WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, p, (int)wcslen(p), buff, sizeof(buff), NULL, NULL);
	return std::string(buff, n);
#endif
}

std::string CGhciTerminal::ToChar(StringT& s)
{
#ifdef _UNICODE
	char buff[1000];
	int n = WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, s.c_str(), (int)s.size(), buff, sizeof(buff), NULL, NULL);
	return std::string(buff, n);
#else
	return s;
#endif
}

//-----------------------------------------------------------
// external command to run in terminal
//-----------------------------------------------------------

DWORD WINAPI ReadAndHandleOutputFn(_In_ LPVOID lpParameter)
{
	CGhciTerminal* p = reinterpret_cast<CGhciTerminal*>(lpParameter);
	if (p) p->ReadAndHandleOutput();
	return 0;
}

void CGhciTerminal::StartCommand(char* options, char* file)
{
	HANDLE hOutputReadTmp, hOutputWrite;
	HANDLE hInputWriteTmp, hInputRead;
	HANDLE hErrorWrite;
	HANDLE hThread;
	DWORD ThreadId;
	SECURITY_ATTRIBUTES sa;

	// Set up the security attributes struct.
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;

	// Create the child output pipe.
	CreatePipe(&hOutputReadTmp, &hOutputWrite, &sa, 0);

	// Create a duplicate of the output write handle for the std error
	// write handle. This is necessary in case the child application
	// closes one of its std output handles.
	DuplicateHandle(GetCurrentProcess(), hOutputWrite,
		GetCurrentProcess(), &hErrorWrite, 0,
		TRUE, DUPLICATE_SAME_ACCESS);

	// Create the child input pipe.
	CreatePipe(&hInputRead, &hInputWriteTmp, &sa, 0);

	// Create new output read handle and the input write handles. Set
	// the Properties to FALSE. Otherwise, the child inherits the
	// properties and, as a result, non-closeable handles to the pipes
	// are created.
	DuplicateHandle(GetCurrentProcess(), hOutputReadTmp,
		GetCurrentProcess(),
		&m_hOutputRead, // Address of new handle.
		0, FALSE, // Make it uninheritable.
		DUPLICATE_SAME_ACCESS);

	DuplicateHandle(GetCurrentProcess(), hInputWriteTmp,
		GetCurrentProcess(),
		&m_hInputWrite, // Address of new handle.
		0, FALSE, // Make it uninheritable.
		DUPLICATE_SAME_ACCESS);

	// Close inheritable copies of the handles you do not want to be
	// inherited.
	CloseHandle(hOutputReadTmp);
	CloseHandle(hInputWriteTmp);

	PrepAndLaunchRedirectedChild(options, file, hOutputWrite, hInputRead, hErrorWrite);

	// Close pipe handles (do not continue to modify the parent).
	// You need to make sure that no handles to the write end of the
	// output pipe are maintained in this process or else the pipe will
	// not close when the child process exits and the ReadFile will hang.
	CloseHandle(hOutputWrite);
	CloseHandle(hInputRead);
	CloseHandle(hErrorWrite);

	// Launch the thread that gets the output and sends it to rich text box
	hThread = CreateThread(NULL, 0, ReadAndHandleOutputFn,
		(LPVOID)this, 0, &ThreadId);
}

/////////////////////////////////////////////////////////////////////// 
// PrepAndLaunchRedirectedChild
// Sets up STARTUPINFO structure, and launches redirected child.
/////////////////////////////////////////////////////////////////////// 
void CGhciTerminal::PrepAndLaunchRedirectedChild(
	char* options, char* file,
	HANDLE hChildStdOut,
	HANDLE hChildStdIn,
	HANDLE hChildStdErr)
{
	PROCESS_INFORMATION pi;
	STARTUPINFO si;

	// Set up the start up info struct.
	ZeroMemory(&si, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	si.dwFlags = STARTF_USESTDHANDLES;
	si.hStdOutput = hChildStdOut;
	si.hStdInput = hChildStdIn;
	si.hStdError = hChildStdErr;
	// Use this if you want to hide the child:
	//     si.wShowWindow = SW_HIDE;
	// Note that dwFlags must include STARTF_USESHOWWINDOW if you want to
	// use the wShowWindow flags.


	// Launch the process that you want to redirect (in this case,
	// Child.exe). Make sure Child.exe is in the same directory as
	// redirect.c launch redirect from a command line to prevent location
	// confusion.
	// "C:\\Program Files\\Haskell Platform\\8.2.2\\bin\\ghci.exe"
	// "C:\\Windows\\System32\\CMD.EXE"
	StringT cmd = _T("C:\\Program Files\\Haskell Platform\\8.0.1\\bin\\ghci.exe");
	StringT cmdl = ToStringT(options) + _T(' ');
	cmdl += ToStringT(file);
	wchar_t wBuff[1000];
	wcscpy_s(wBuff, _countof(wBuff), cmdl.c_str());
	BOOL b = CreateProcess(cmd.c_str(), wBuff, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);

	// Set global child process handle to cause threads to exit.
	m_hChildProcess = pi.hProcess;

	// Close any unnecessary handles.
	CloseHandle(pi.hThread);
}

/////////////////////////////////////////////////////////////////////// 
// ReadAndHandleOutput
// Monitors handle for input. Exits when child exits or pipe breaks.
/////////////////////////////////////////////////////////////////////// 
void CGhciTerminal::ReadAndHandleOutput()
{
	CHAR lpBuffer[256];
	DWORD nBytesRead;

	while (TRUE)
	{
		ReadFile(m_hOutputRead, lpBuffer, (sizeof(lpBuffer) - 1), &nBytesRead, NULL);
		lpBuffer[nBytesRead] = 0;
		AddTextTS(ToStringT(lpBuffer));
		m_noOfChars = GetNoOfChars();
	}
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
		StringT s1 = GetCommandLine();
		std::transform(s1.begin(), s1.end(), s1.begin(), ::toupper);
		std::vector<StringsT::const_iterator> filtered;
		for (StringsT::const_iterator it = m_cmdHistory.begin(); it != m_cmdHistory.end(); ++it)
		{
			StringT s2 = *it;
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

			for (std::vector<StringsT::const_iterator>::const_iterator it = filtered.begin(); it != filtered.end(); ++it)
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








