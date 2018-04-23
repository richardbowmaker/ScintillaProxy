#include "stdafx.h"

#include <windows.h>
#include <Windowsx.h>
#include <tlhelp32.h>

#include "Ghci.h"
#include "CUtils.h"

CGhci::CGhci() :
	m_initialised(false),
	m_threadStopped(false),
	m_hInputWrite(NULL),
	m_hOutputRead(NULL),
	m_handler(NULL),
	m_handlerData(NULL),
	m_outputReady(NULL),
	m_synch(false)
{
	::ZeroMemory(&m_pi, sizeof(m_pi));
	::ZeroMemory(&m_si, sizeof(m_si));
}

CGhci::~CGhci()
{
	Uninitialise();
}

bool CGhci::Initialise(IdT id, const char* options, const char* file, const char* directory)
{
	if (!m_initialised)
	{
		m_outputReady = ::CreateEvent(NULL, FALSE, FALSE, NULL);
		if (m_outputReady != NULL)
		{
			m_id = id;
			::InitializeCriticalSection(&m_cs);
			m_initialised = true;
			if (!StartCommand(options, file, directory))
			{
				Uninitialise();
			}
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

		if (m_pi.hProcess)
		{
			SendCommand(_T(":quit\n"));

			// delete all child processes of GHCI, i.e. the program being debugged in GHCI
			HANDLE h = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
			PROCESSENTRY32 pe;
			::ZeroMemory(&pe, sizeof(PROCESSENTRY32));
			pe.dwSize = sizeof(PROCESSENTRY32);
			bool more = Process32First(h, &pe);
			while (more) 			
			{
				if (pe.th32ParentProcessID == m_pi.dwProcessId)
				{
					HANDLE h = ::OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
					if (h != NULL)
					{
						::TerminateProcess(h, 1);
					}
				}
				more = Process32Next(h, &pe);
			}

			// terminate GHCI
			::TerminateProcess(m_pi.hProcess, 1);
			m_pi.hProcess = NULL;

			// terminate thread
			m_initialised = false;
			while (!m_threadStopped);
		}

		::DeleteCriticalSection(&m_cs);
		::CloseHandle(m_hInputWrite);
		::CloseHandle(m_hOutputRead);
		m_initialised = false;
	}
}

CGhci::IdT CGhci::GetId()
{
	return m_id;
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

void CGhci::SendCommandAsynch(const char* cmd, const char* eod)
{
	m_eod = eod;
	m_response.clear();
	m_synch = false;
	SendCommand(cmd);
}

bool CGhci::SendCommandSynch(const char* cmd, const char* eod, DWORD timeout, const char** response)
{
	bool isok = false;
	m_response.clear();
	::EnterCriticalSection(&m_cs);
	m_output.clear();
	::LeaveCriticalSection(&m_cs);
	::ResetEvent(m_outputReady);
	m_synch = true;

	SendCommand(cmd);	
	isok = WaitForResponse(eod, timeout, response);
	m_synch = false;
	return isok;
}

bool CGhci::WaitForResponse(const char* eod, DWORD timeout, const char** response)
{
	bool isok = false;
	m_synch = true;

	while (true)
	{
		DWORD res = ::WaitForSingleObject(m_outputReady, timeout);
		if (res == WAIT_OBJECT_0)
		{
			::EnterCriticalSection(&m_cs);
			m_response += m_output;
			m_output.clear();
			::LeaveCriticalSection(&m_cs);

			if (m_response.find(eod, 0) != std::string::npos)
			{
				*response = m_response.c_str();
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

void CGhci::Notify(const char* text)
{
	if (m_handler)
	{
		m_handler(m_id, text, m_handlerData);
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

bool CGhci::StartCommand(const char* options, const char* file, const char* directory)
{
	HANDLE hOutputReadTmp	= NULL; 
	HANDLE hOutputWrite		= NULL;
	HANDLE hInputWriteTmp	= NULL;
	HANDLE hInputRead		= NULL;
	HANDLE hErrorReadTmp	= NULL;
	HANDLE hErrorWrite		= NULL;
	HANDLE hThread			= NULL;
	DWORD ThreadId			= 0;
	SECURITY_ATTRIBUTES sa;

	// Set up the security attributes struct.
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;

	try
	{
		// Create the child output pipe.
		if (!::CreatePipe(&hOutputReadTmp, &hOutputWrite, &sa, 0))
			throw CException::CreateException("Can't create output pipe");

		// Create a duplicate of the output write handle for the std error
		// write handle. This is necessary in case the child application
		// closes one of its std output handles.
		if (!::DuplicateHandle(
			GetCurrentProcess(),
			hOutputWrite,
			GetCurrentProcess(),
			&hErrorWrite,
			0,
			TRUE,
			DUPLICATE_SAME_ACCESS))
			throw CException::CreateException("Can't duplicate error output pipe");

		// Create the child input pipe.
		if (!::CreatePipe(&hInputRead, &hInputWriteTmp, &sa, 0))
			throw CException::CreateException("Can't create input pipe");

		// Create new output read handle and the input write handles. Set
		// the Properties to FALSE. Otherwise, the child inherits the
		// properties and, as a result, non-closeable handles to the pipes
		// are created.
		if (!::DuplicateHandle(
			GetCurrentProcess(),
			hOutputReadTmp,
			GetCurrentProcess(),
			&m_hOutputRead, // Address of new handle.
			0,
			FALSE, // Make it uninheritable.
			DUPLICATE_SAME_ACCESS))
			throw CException::CreateException("Can't duplicate output pipe");

		if (!::DuplicateHandle(
			GetCurrentProcess(),
			hInputWriteTmp,
			GetCurrentProcess(),
			&m_hInputWrite, // Address of new handle.
			0,
			FALSE, // Make it uninheritable.
			DUPLICATE_SAME_ACCESS))
			throw CException::CreateException("Can't duplicate imput pipe");

		// Close inheritable copies of the handles you do not want to be
		// inherited.
		if (!::CloseHandle(hOutputReadTmp)) throw CException::CreateException("Can't close output read tmp handle");
		hOutputReadTmp = NULL;
		if (!::CloseHandle(hInputWriteTmp)) throw CException::CreateException("Can't close input write tmp handle");
		hInputWriteTmp = NULL;

		PrepAndLaunchRedirectedChild(options, file, directory, hOutputWrite, hInputRead, hErrorWrite);

		// Close pipe handles (do not continue to modify the parent).
		// You need to make sure that no handles to the write end of the
		// output pipe are maintained in this process or else the pipe will
		// not close when the child process exits and the ReadFile will hang.
		if (!::CloseHandle(hOutputWrite)) throw CException::CreateException("Can't close output write handle");
		hOutputWrite = NULL;
		if (!::CloseHandle(hInputRead)) throw CException::CreateException("Can't close input read handle");
		hInputRead = NULL;
		if (!::CloseHandle(hErrorWrite)) throw CException::CreateException("Can't close error write handle");
		hErrorWrite = NULL;

		// Launch the thread that gets the output and sends it to rich text box
		hThread = ::CreateThread(NULL, 0, ReadAndHandleOutputFn, (LPVOID)this, 0, &ThreadId);

		if (hThread == NULL)
			throw CException::CreateException("Can't create read output worker thread");

		return true;
	}
	catch (CException &)
	{
		if (hOutputReadTmp	!= NULL) ::CloseHandle(hOutputReadTmp);
		if (hOutputWrite	!= NULL) ::CloseHandle(hOutputWrite);
		if (hInputWriteTmp	!= NULL) ::CloseHandle(hInputWriteTmp);
		if (hInputRead		!= NULL) ::CloseHandle(hInputRead);
		if (hErrorReadTmp	!= NULL) ::CloseHandle(hErrorReadTmp);
		if (hErrorWrite		!= NULL) ::CloseHandle(hErrorWrite);
		if (hThread			!= NULL) ::CloseHandle(hThread);
		return false;
	}
}

/////////////////////////////////////////////////////////////////////// 
// PrepAndLaunchRedirectedChild
// Sets up STARTUPINFO structure, and launches redirected child.
/////////////////////////////////////////////////////////////////////// 
void CGhci::PrepAndLaunchRedirectedChild(
	const char* options, 
	const char* file,
	const char* directory,
	HANDLE hChildStdOut,
	HANDLE hChildStdIn,
	HANDLE hChildStdErr)
{
	// Set up the start up info struct.
	ZeroMemory(&m_si, sizeof(STARTUPINFO));
	m_si.cb = sizeof(STARTUPINFO);
	m_si.dwFlags = STARTF_USESTDHANDLES;
	m_si.hStdOutput = hChildStdOut;
	m_si.hStdInput = hChildStdIn;
	m_si.hStdError = hChildStdErr;
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
	//CUtils::StringT cmd = _T("C:\\Windows\\System32\\CMD.EXE");
	CUtils::StringT cmdl = CUtils::ToStringT(options) + _T(' ');
	cmdl += CUtils::ToStringT(file);
	wchar_t wBuff[1000];
	wcscpy_s(wBuff, _countof(wBuff), cmdl.c_str());
	CUtils::StringT dir = CUtils::ToStringT(directory);

	if (!::CreateProcess(cmd.c_str(), wBuff, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, dir.c_str(), &m_si, &m_pi))
	{
		throw CException::CreateLastErrorException();
	}

	// Close any unnecessary handles.
	CloseHandle(m_pi.hThread);
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
		Sleep(50);
		PeekNamedPipe(m_hOutputRead, lpBuffer, (sizeof(lpBuffer) - 1), &nBytesRead, NULL, NULL);
		if (nBytesRead > 0)
		{
			ReadFile(m_hOutputRead, lpBuffer, (sizeof(lpBuffer) - 1), &nBytesRead, NULL);
			if (nBytesRead > 0)
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
					// for asynch command
					m_response += lpBuffer;
					if (m_response.find(m_eod, 0) != std::string::npos)
					{
						Notify(m_response.c_str());
						m_response.clear();
						m_eod.clear();
					}
				}
			}
		}
		// check to see if the child process is still alive
		DWORD code;
		if (::GetExitCodeProcess(m_pi.hProcess, &code))
		{
			if (code != STILL_ACTIVE)
			{
				// GHCI has stopped unexpectedly
				DWORD err = ::GetLastError();
				Notify("GHCI terminated unexpectedly\n");
				break;
			}
		}
	}
	m_threadStopped = true;
	ExitThread(0);
}

