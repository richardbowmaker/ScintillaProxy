#include "stdafx.h"
#include<windows.h>
#include <tchar.h>
#include <stdio.h> 
#include <Commctrl.h>
//#pragma comment(lib, "User32.lib")
#include "TerminalWin.h"


// rich text edit control, subclass win proc
LRESULT CALLBACK RichTextEditProc(HWND hWnd, UINT uMsg, WPARAM wParam,
	LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	CTerminalWin* p = reinterpret_cast<CTerminalWin*>(dwRefData);
	if (p) p->WinProc(uMsg, wParam, lParam);
	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

CTerminalWin::CTerminalWin() : 
	m_hwnd(NULL),
	m_parent(NULL),
	m_hdll(NULL),
	m_echoInput(false),
	hChildProcess(NULL),
	hStdIn(NULL),
	bRunThread(TRUE),
	hInputWrite(NULL),
	m_outLen(0),
	m_lix(0)
{
}

CTerminalWin::~CTerminalWin()
{
	Uninitialise();
}

bool CTerminalWin::Initialise(HWND hParent, bool echoInput)
{
	m_parent = hParent;
	m_echoInput = echoInput;
	m_hdll = ::LoadLibrary(TEXT("Msftedit.dll"));

	if (m_hdll)
	{
		m_hwnd = ::CreateWindowExW(0, MSFTEDIT_CLASS, L"",
			ES_MULTILINE | ES_READONLY | WS_VISIBLE | WS_CHILD | WS_BORDER | WS_TABSTOP,
			0, 0, 0, 0,
			hParent, NULL, NULL, NULL);

		if (m_hwnd) 
		{
			RECT r;
			::GetWindowRect(hParent, &r);
			::MoveWindow(m_hwnd, 0, 0, r.right - r.left, r.bottom - r.top, TRUE);

			// subclass the rich edit text control
			::SetWindowSubclass(m_hwnd, RichTextEditProc, 0, reinterpret_cast<DWORD_PTR>(this));

			StartCommand();
			return true;
		}
	}
	return false;
}

void CTerminalWin::Uninitialise()
{
	if (hChildProcess)
	{
		TerminateProcess(hChildProcess, 0);
	}

	if (m_hwnd)
	{
		::CloseWindow(m_hwnd);
		::DestroyWindow(m_hwnd);
		m_hwnd = NULL;
	}
}

void CTerminalWin::ClearText()
{
	SetText(_T(""));
	m_outLen = 0;
}

void CTerminalWin::SetText(TCHAR* pText)
{
	int ndx = GetWindowTextLength(m_hwnd);
	::SendMessage(m_hwnd, EM_SETSEL, 0, (LPARAM)ndx);
	::SendMessage(m_hwnd, EM_REPLACESEL, 0, (LPARAM)pText);
}

void CTerminalWin::AddText(TCHAR* pText)
{
	int ndx = GetWindowTextLength(m_hwnd);
	::SendMessage(m_hwnd, EM_SETSEL, (WPARAM)ndx, (LPARAM)ndx);
	::SendMessage(m_hwnd, EM_REPLACESEL, 0, (LPARAM)pText);
}

void CTerminalWin::AddLine(TCHAR* pText)
{
	AddText(pText);
	NewLine();
}

void CTerminalWin::NewLine()
{
	AddText(_T("\n"));
}

HWND CTerminalWin::GetHwnd()
{
	return m_hwnd;
}

// non-blocking, thread safe
void CTerminalWin::AddTextTS(TCHAR* pText)
{
	TCHAR* p = _tcsdup(pText);
	if (p) ::PostMessage(m_hwnd, TW_ADDTEXT, 0, reinterpret_cast<LPARAM>(p));
}

void CTerminalWin::HandleKeyPress(WPARAM wParam, LPARAM lParam)
{
	_charrange pos;
	BOOL b = ::SendMessage(m_hwnd, EM_EXGETSEL, 0, reinterpret_cast<LPARAM>(&pos));

	// only allowed to backspace to last output position
	if (wParam == 0x08)
	{
		int lix = pos.cpMin - m_outLen - 1;

		if (lix >= 0 && lix < m_lix)
		{
			_tcsncpy(&m_line[lix], &m_line[lix + 1], m_lix - lix + 1);
			m_lix--;
			ShowCommandLine();
			::SendMessage(m_hwnd, EM_SETSEL, (WPARAM)pos.cpMin - 1, (LPARAM)pos.cpMin - 1);
		}
	}

	if (wParam == 0x0d)
	{
		m_line[m_lix++] = '\n';
		m_line[m_lix] = 0;
		ShowCommandLine();
		m_outLen += m_lix;
		SendCommand(m_line);
		m_lix = 0;
	}

	if (wParam >= 0x20 && wParam <= 0x7e)
	{
		m_line[m_lix++] = (TCHAR)wParam;
		ShowCommandLine();
	}
}

void CTerminalWin::ShowCommandLine()
{
	m_line[m_lix] = 0;
	int ndx = GetWindowTextLength(m_hwnd);
	::SendMessage(m_hwnd, EM_SETSEL, (WPARAM)m_outLen, (LPARAM)ndx);
	::SendMessage(m_hwnd, EM_REPLACESEL, 0, (LPARAM)m_line);
}
	
void CTerminalWin::WinProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
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

	default:
		break;
	}
}

void CTerminalWin::SendCommand(TCHAR* pText)
{
	DWORD nBytesRead, nBytesWrote;

	nBytesRead = _tcslen(pText);

	if (!WriteFile(hInputWrite, pText, nBytesRead, &nBytesWrote, NULL))
	{
		if (GetLastError() == ERROR_NO_DATA)
			return; // Pipe was closed (normal exit path).
		else
			DisplayError(_T("WriteFile"));
	}
}

//-----------------------------------------------------------
// external command to run in terminal
//-----------------------------------------------------------

DWORD WINAPI ReadAndHandleOutputX(_In_ LPVOID lpParameter)
{
	CTerminalWin* p = reinterpret_cast<CTerminalWin*>(lpParameter);
	if (p) p->ReadAndHandleOutput();
	return 0;
}

void CTerminalWin::StartCommand()
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
	if (!CreatePipe(&hOutputReadTmp, &hOutputWrite, &sa, 0))
		DisplayError(_T("CreatePipe"));

	// Create a duplicate of the output write handle for the std error
	// write handle. This is necessary in case the child application
	// closes one of its std output handles.
	if (!DuplicateHandle(GetCurrentProcess(), hOutputWrite,
		GetCurrentProcess(), &hErrorWrite, 0,
		TRUE, DUPLICATE_SAME_ACCESS))
		DisplayError(_T("DuplicateHandle"));

	// Create the child input pipe.
	if (!CreatePipe(&hInputRead, &hInputWriteTmp, &sa, 0))
		DisplayError(_T("CreatePipe"));

	// Create new output read handle and the input write handles. Set
	// the Properties to FALSE. Otherwise, the child inherits the
	// properties and, as a result, non-closeable handles to the pipes
	// are created.
	if (!DuplicateHandle(GetCurrentProcess(), hOutputReadTmp,
		GetCurrentProcess(),
		&hOutputRead, // Address of new handle.
		0, FALSE, // Make it uninheritable.
		DUPLICATE_SAME_ACCESS))
		DisplayError(_T("DupliateHandle"));

	if (!DuplicateHandle(GetCurrentProcess(), hInputWriteTmp,
		GetCurrentProcess(),
		&hInputWrite, // Address of new handle.
		0, FALSE, // Make it uninheritable.
		DUPLICATE_SAME_ACCESS))
		DisplayError(_T("DupliateHandle"));

	// Close inheritable copies of the handles you do not want to be
	// inherited.
	if (!CloseHandle(hOutputReadTmp)) DisplayError(_T("CloseHandle"));
	if (!CloseHandle(hInputWriteTmp)) DisplayError(_T("CloseHandle"));

	//// Get std input handle so you can close it and force the ReadFile to
	//// fail when you want the input thread to exit.
	//if ((hStdIn = GetStdHandle(STD_INPUT_HANDLE)) ==
	//	INVALID_HANDLE_VALUE)
	//	DisplayError("GetStdHandle");

	PrepAndLaunchRedirectedChild(hOutputWrite, hInputRead, hErrorWrite);

	// Close pipe handles (do not continue to modify the parent).
	// You need to make sure that no handles to the write end of the
	// output pipe are maintained in this process or else the pipe will
	// not close when the child process exits and the ReadFile will hang.
	if (!CloseHandle(hOutputWrite)) DisplayError(_T("CloseHandle"));
	if (!CloseHandle(hInputRead)) DisplayError(_T("CloseHandle"));
	if (!CloseHandle(hErrorWrite)) DisplayError(_T("CloseHandle"));

	// Launch the thread that gets the output and sends it to rich text box
	hThread = CreateThread(NULL, 0, ReadAndHandleOutputX,
		(LPVOID)this, 0, &ThreadId);
	if (hThread == NULL) DisplayError(_T("CreateThread"));

	//// Force the read on the input to return by closing the stdin handle.
	//if (!CloseHandle(hStdIn)) DisplayError(_T("CloseHandle"));



	//if (WaitForSingleObject(hThread, INFINITE) == WAIT_FAILED)
	//	DisplayError(_T("WaitForSingleObject"));

	//if (!CloseHandle(hOutputRead)) DisplayError(_T("CloseHandle"));
	//if (!CloseHandle(hInputWrite)) DisplayError(_T("CloseHandle"));
}


/////////////////////////////////////////////////////////////////////// 
// PrepAndLaunchRedirectedChild
// Sets up STARTUPINFO structure, and launches redirected child.
/////////////////////////////////////////////////////////////////////// 
void CTerminalWin::PrepAndLaunchRedirectedChild(HANDLE hChildStdOut,
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
	if (!CreateProcess(_T("C:\\Windows\\System32\\CMD.EXE"), NULL, NULL, NULL, TRUE,
		CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
		DisplayError(_T("CreateProcess"));

	// Set global child process handle to cause threads to exit.
	hChildProcess = pi.hProcess;

	// Close any unnecessary handles.
	if (!CloseHandle(pi.hThread)) DisplayError(_T("CloseHandle"));
}

/////////////////////////////////////////////////////////////////////// 
// ReadAndHandleOutput
// Monitors handle for input. Exits when child exits or pipe breaks.
/////////////////////////////////////////////////////////////////////// 
void CTerminalWin::ReadAndHandleOutput()
{
	CHAR lpBuffer[256];
	DWORD nBytesRead;
//	DWORD nCharsWritten;

	while (TRUE)
	{
		::ZeroMemory(lpBuffer, sizeof(lpBuffer));

		if (!ReadFile(hOutputRead, lpBuffer, (sizeof(lpBuffer)-1),
			&nBytesRead, NULL) || !nBytesRead)
		{
			if (GetLastError() == ERROR_BROKEN_PIPE)
				break; // pipe done - normal exit path.
			else
				DisplayError(_T("ReadFile")); // Something bad happened.
		}
		AddTextTS(lpBuffer);

		// count characters in string excluding carriage returns
		CHAR* p = lpBuffer;
		while (*p++ != 0) if (*p != '\r') m_outLen++;
	}
}

/////////////////////////////////////////////////////////////////////// 
// DisplayError
// Displays the error number and corresponding message.
/////////////////////////////////////////////////////////////////////// 
void CTerminalWin::DisplayError(TCHAR *pszAPI)
{
	LPVOID lpvMessageBuffer;
	TCHAR szPrintBuffer[512];

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpvMessageBuffer, 0, NULL);

	_stprintf_s(szPrintBuffer, _countof(szPrintBuffer),
		_T("ERROR: API    = %s.\n   error code = %d.\n   message    = %s.\n"),
		pszAPI, GetLastError(), (TCHAR*)lpvMessageBuffer);

	OutputDebugString(szPrintBuffer);

}


