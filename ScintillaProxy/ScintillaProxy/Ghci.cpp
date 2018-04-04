#include "stdafx.h"

#include <windows.h>
#include <Windowsx.h>

#include "Ghci.h"

CGhci::CGhci() :
	m_initialised(false),
	m_threadStopped(false),
	m_hChildProcess(NULL),
	m_hInputWrite(NULL),
	m_hOutputRead(NULL),
	m_handler(NULL),
	m_handlerData(NULL),
	m_outputReady(NULL),
	m_synch(false)
{
}

CGhci::~CGhci()
{
	Uninitialise();
}

bool CGhci::Initialise(char* options, char* file)
{
	if (!m_initialised)
	{
		m_outputReady = ::CreateEvent(NULL, FALSE, FALSE, NULL);
		if (m_outputReady != NULL)
		{
			::InitializeCriticalSection(&m_cs);
			m_initialised = true;
			StartCommand(options, file);
		}
	}
	return m_initialised;
}

void CGhci::Uninitialise()
{
	if (m_initialised)
	{
		m_handler = NULL;
		m_synch = false;

		if (m_outputReady)
		{
			::CloseHandle(m_outputReady);
		}

		if (m_hChildProcess)
		{
			SendCommand(_T(":quit\n"));
			m_hChildProcess = NULL;
		}

		// terminate thread
		m_initialised = false;
		while (!m_threadStopped);

		::DeleteCriticalSection(&m_cs);
		::CloseHandle(m_hInputWrite);
		::CloseHandle(m_hOutputRead);
	}
}

// send a command to the process
void CGhci::SendCommand(CUtils::StringT text)
{
	SendCommand(CUtils::ToChar(text).c_str());
}

void CGhci::SendCommand(const char* cmd)
{
	DWORD nBytesWrote;
	std::string cmd1 = std::string(cmd) + "\n";
	WriteFile(m_hInputWrite, cmd1.c_str(), (DWORD)cmd1.size(), &nBytesWrote, NULL);
}

bool CGhci::SendCommandSynch(char* cmd, char* eod, DWORD timeout, const char** output)
{
	bool isok = false;
	::EnterCriticalSection(&m_cs);
	m_output = "";
	::LeaveCriticalSection(&m_cs);
	::ResetEvent(m_outputReady);
	m_synch = true;

	SendCommand(cmd);

	while (true)
	{
		DWORD res = ::WaitForSingleObject(m_outputReady, timeout);
		if (res == WAIT_OBJECT_0)
		{
			::EnterCriticalSection(&m_cs);
			std::string s = m_output;
			::LeaveCriticalSection(&m_cs);

			if (s.find(eod, 0) != std::string::npos)
			{
				*output = s.c_str();
				isok = true;
				break;
			}
		}
		else
		{
			break;
		}
	}
	m_synch = false;
	return isok;
}

void CGhci::Notify(char* text)
{
	if (m_handler)
	{
		m_handler(text, m_handlerData);
	}
}

void CGhci::SetEventHandler(EventHandlerT handler, void* data)
{
	m_handler = handler;
	m_handlerData = data;
}

//-----------------------------------------------------------
// external command to run in terminal
//-----------------------------------------------------------

DWORD WINAPI ReadAndHandleOutputFn(_In_ LPVOID lpParameter)
{
	CGhci* p = reinterpret_cast<CGhci*>(lpParameter);
	if (p) p->ReadAndHandleOutput();
	return 0;
}

void CGhci::StartCommand(char* options, char* file)
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
void CGhci::PrepAndLaunchRedirectedChild(
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
	CUtils::StringT cmd = _T("C:\\Program Files\\Haskell Platform\\8.0.1\\bin\\ghci.exe");
	CUtils::StringT cmdl = CUtils::ToStringT(options) + _T(' ');
	cmdl += CUtils::ToStringT(file);
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
void CGhci::ReadAndHandleOutput()
{
	CHAR lpBuffer[256];
	DWORD nBytesRead;

	while (m_initialised)
	{
		ReadFile(m_hOutputRead, lpBuffer, (sizeof(lpBuffer) - 1), &nBytesRead, NULL);
		if (m_initialised && nBytesRead > 0)
		{
			lpBuffer[nBytesRead] = 0;
			if (m_synch)
			{
				// for synchronised command
				::EnterCriticalSection(&m_cs);
				m_output += lpBuffer;
				::LeaveCriticalSection(&m_cs);
				::SetEvent(m_outputReady);
			}
			else
			{
				// fr asynchronous command
				Notify(lpBuffer);
			}
		}
	}
	m_threadStopped = true;
	ExitThread(0);
}

