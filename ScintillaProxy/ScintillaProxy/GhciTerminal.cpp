#include "stdafx.h"

#include <algorithm>
#include <Commctrl.h>
#include <stdio.h> 
#include <string>
#include <tchar.h>
#include <windows.h>

#include "GhciTerminal.h"

// rich text edit control, subclass win proc
LRESULT CALLBACK RichTextBoxProcFn(HWND hWnd, UINT uMsg, WPARAM wParam,
	LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	CGhciTerminal* p = reinterpret_cast<CGhciTerminal*>(dwRefData);
	if (p)
	{
		bool b = p->RichTextBoxProc(hWnd, uMsg, wParam, lParam);
		if (b)
		{
			return DefSubclassProc(hWnd, uMsg, wParam, lParam);
		}
	}
	return 0;
}

// list box control, subclass win proc
LRESULT CALLBACK ListBoxProcFn(HWND hWnd, UINT uMsg, WPARAM wParam,
	LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	CGhciTerminal* p = reinterpret_cast<CGhciTerminal*>(dwRefData);
	if (p)
	{
		bool b = p->ListBoxProc(uMsg, wParam, lParam);
		if (b)
		{
			return DefSubclassProc(hWnd, uMsg, wParam, lParam);
		}
	}
	return 0;
}

CGhciTerminal::CGhciTerminal() :
	m_hwnd(NULL),
	m_parent(NULL),
	m_hdll(NULL),
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

bool CGhciTerminal::Initialise(HWND hParent, char* file)
{
	m_parent = hParent;
	m_hdll = ::LoadLibrary(TEXT("Msftedit.dll"));

	if (m_hdll)
	{
		m_hwnd = ::CreateWindowExW(0, MSFTEDIT_CLASS, L"",
			ES_MULTILINE | ES_READONLY | WS_HSCROLL  | WS_VSCROLL | WS_VISIBLE | WS_CHILD | WS_BORDER | WS_TABSTOP,
			0, 0, 0, 0, hParent, NULL, NULL, NULL);

		if (m_hwnd) 
		{
			RECT r;
			::GetClientRect(hParent, &r);
			::MoveWindow(m_hwnd, 0, 0, r.right - r.left, r.bottom - r.top, TRUE);

			_charformat cf;
			ZeroMemory(&cf, sizeof(cf));
			cf.cbSize = sizeof(_charformat);
			cf.dwMask = CFM_FACE;
			strncpy_s(cf.szFaceName, LF_FACESIZE, "Courier New", 9);
			::SendMessage(m_hwnd, EM_SETCHARFORMAT, SCF_ALL | SCF_DEFAULT, (LPARAM)&cf);

			// subclass the rich edit text control
			::SetWindowSubclass(m_hwnd, RichTextBoxProcFn, 0, reinterpret_cast<DWORD_PTR>(this));

			StartCommand(file);
			return true;
		}
	}
	return false;
}

void CGhciTerminal::Uninitialise()
{
	if (m_hChildProcess)
	{
		TerminateProcess(m_hChildProcess, 0);
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

void CGhciTerminal::ShowLookup()
{
	if (m_hwndLookup == NULL)
	{
		m_hwndLookup = CreateWindowExW(0, L"ListBox", 
			NULL, WS_CHILD | WS_BORDER | WS_VSCROLL | WS_HSCROLL | WS_VISIBLE, 
			0, 0, 0, 0, m_hwnd, NULL, NULL, NULL);

		RECT r;
		::GetClientRect(m_hwnd, &r);
		::MoveWindow(m_hwndLookup, (r.right - r.left) / 2, 0, (r.right - r.left) / 2, (r.bottom - r.top) / 2, TRUE);

		int ndx = GetWindowTextLength(m_hwnd);
		::SendMessage(m_hwnd, EM_SETSEL, (WPARAM)m_noOfChars, (LPARAM)ndx);

		// fill list box with command history filtered on starting with current command line
		if (ndx > m_noOfChars)
		{
			StringT s1 = m_cmdLine;
			std::transform(s1.begin(), s1.end(), s1.begin(), ::toupper);

			for (StringsT::const_iterator it = m_cmdHistory.begin(); it != m_cmdHistory.end(); ++it)
			{
				StringT s2 = *it;
				std::transform(s2.begin(), s2.end(), s2.begin(), ::toupper);
				s2 = s2.substr(0, s1.size());
				if (s1 == s2)
				{
					int ix = (int)::SendMessage(m_hwndLookup, LB_ADDSTRING, 0, (LPARAM)it->c_str());
					::SendMessage(m_hwndLookup, LB_SETITEMDATA, ix, (LPARAM)it->c_str());
				}
			}
		}
		::SetWindowSubclass(m_hwndLookup, ListBoxProcFn, 0, reinterpret_cast<DWORD_PTR>(this));
	}
}

void CGhciTerminal::HideLookup()
{
	if (m_hwndLookup)
	{
		::CloseWindow(m_hwndLookup);
		::DestroyWindow(m_hwndLookup);
		m_hwndLookup = NULL;
	}
}

void CGhciTerminal::ClearText()
{
	SetText(_T(""));
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

// non-blocking, thread safe
void CGhciTerminal::AddTextTS(StringT text)
{
	TCHAR* p = _tcsdup(text.c_str());
	::SendMessage(m_hwnd, TW_ADDTEXT, 0, reinterpret_cast<LPARAM>(p));
}

void CGhciTerminal::HandleKeyPress(WPARAM wParam, LPARAM lParam)
{
	_charrange pos;
	BOOL b = (BOOL)::SendMessage(m_hwnd, EM_EXGETSEL, 0, reinterpret_cast<LPARAM>(&pos));

	// only allowed to backspace to last output position
	if (wParam == 0x08)
	{
		unsigned int lix = pos.cpMin - m_noOfChars - 1;

		if (lix >= 0 && lix < m_cmdLine.size())
		{
			m_cmdLine = StringRemoveAt(m_cmdLine, lix);
			UpdateCommandLine();
			::SendMessage(m_hwnd, EM_SETSEL, (WPARAM)pos.cpMin - 1, (LPARAM)pos.cpMin - 1);
		}
	}
	if (wParam == 0x0d)
	{
		UpdateCommandLine();
		AddText(_T("\n"));
		m_noOfChars = GetNoOfChars();
		SendCommand(m_cmdLine);
		m_cmdLine.clear();
	}
	if (wParam >= 0x20 && wParam <= 0x7e)
	{
		m_cmdLine += (TCHAR)wParam;
		UpdateCommandLine();
	}
}

bool CGhciTerminal::ListBoxProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_LBUTTONUP:
	case WM_LBUTTONDBLCLK:
		{
			unsigned int ix = (unsigned int)::SendMessage(m_hwndLookup, LB_GETCURSEL, 0, 0);
			if (ix != LB_ERR)
			{ 
				 TCHAR* p = reinterpret_cast<TCHAR*>(::SendMessage(m_hwndLookup, LB_GETITEMDATA, ix, 0));
				 if (p)
				 {
					 m_cmdLine = p;
					 UpdateCommandLine();
				 }
			}
		}
		break;
	case WM_KEYDOWN:
		{
			switch (wParam)
			{
			case VK_ESCAPE:
			case VK_TAB:
				HideLookup();
				break;
			}
		}
		break;
	default:
		break;
	}
	return true;
}

bool CGhciTerminal::RichTextBoxProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_SIZE:
		{
			int n = 0;
		}
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
		{
			HandleKeyPress(wParam, lParam);
		}
		break;
	case WM_KEYDOWN:
		{
			HideLookup();

			switch (wParam)
			{
				case VK_UP:
				{
					if (m_hix == -1)
					{
						m_hix = (int)m_cmdHistory.size() - 1;
					}
					else
					{
						m_hix = (m_hix + ((int)m_cmdHistory.size() - 1)) % (int)m_cmdHistory.size();
					}
					m_cmdLine = m_cmdHistory[m_hix];
					UpdateCommandLine();
					return false;
				}
				case VK_DOWN:
				{
					if (m_hix == -1)
					{
						m_hix = 0;
					}
					else
					{
						m_hix = (m_hix + 1) % m_cmdHistory.size();
					}
					m_cmdLine = m_cmdHistory[m_hix];
					UpdateCommandLine();
					return false;
				}
				case VK_TAB:
					ShowLookup();
					break;
				case VK_ESCAPE:
					m_cmdLine.clear();
					UpdateCommandLine();
					break;
			}
		}
		break;
	default:
		break;
	}
	return true;
}

// remove current display of command line and show latest 
void CGhciTerminal::UpdateCommandLine()
{
	int ndx = GetWindowTextLength(m_hwnd);
	::SendMessage(m_hwnd, EM_SETSEL, (WPARAM)m_noOfChars, (LPARAM)ndx);
	::SendMessage(m_hwnd, EM_REPLACESEL, 0, (LPARAM)m_cmdLine.c_str());
}

// send a command to the process
void CGhciTerminal::SendCommand(StringT text)
{
	DWORD nBytesWrote;
	std::string cmd = ToChar(text) + '\n';

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
	BOOL b = (BOOL)::SendMessage(m_hwnd, EM_EXGETSEL, 0, reinterpret_cast<LPARAM>(&pos));
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

void CGhciTerminal::StartCommand(char* file)
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

	PrepAndLaunchRedirectedChild(file, hOutputWrite, hInputRead, hErrorWrite);

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
void CGhciTerminal::PrepAndLaunchRedirectedChild(char* file, HANDLE hChildStdOut,
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
	// "C:\\Program Files\\Haskell Platform\\8.0.1\\bin\\ghci.exe"
	// "C:\\Windows\\System32\\CMD.EXE"
	StringT fn = ToStringT(file);
	wchar_t wFile[1000];
	int n = (int)fn.copy(wFile, fn.size(), 0);
	wFile[n] = 0;

	CreateProcess(_T("C:\\Program Files\\Haskell Platform\\8.0.1\\bin\\ghci.exe"), wFile, NULL, NULL, TRUE,
		CREATE_NO_WINDOW, NULL, NULL, &si, &pi);

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
		ReadFile(m_hOutputRead, lpBuffer, (sizeof(lpBuffer)-1), &nBytesRead, NULL);
		lpBuffer[nBytesRead] = 0;
		AddTextTS(ToStringT(lpBuffer));
		m_noOfChars = GetNoOfChars();
	}
}

void CGhciTerminal::WndProcRetHook(int nCode, WPARAM wParam, LPARAM lParam)
{
	LPCWPRETSTRUCT pData = reinterpret_cast<LPCWPRETSTRUCT>(lParam);

	switch (pData->message)
	{
	case WM_SIZE:
	{
		if (m_parent == pData->hwnd)
		{
			int w = (int)(pData->lParam) & 0x0ffff;
			int h = (int)(pData->lParam) >> 16;
			::MoveWindow(m_hwnd, 0, 0, w, h, TRUE);
		}
	}
	break;
	default:
		break;
	}
}


