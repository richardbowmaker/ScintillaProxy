#pragma once

#include <memory>

#include "CUtils.h"

class CGhci
{
public:

	typedef void (*EventHandlerT)(int, char*, void*);
	typedef unsigned __int32 IdT;
	typedef std::shared_ptr<CGhci> CGhciPtrT;

	CGhci();
	~CGhci();

	bool Initialise(IdT id, const char* options, const char* file);
	void Uninitialise();

	IdT GetId();

	void SendCommand(CUtils::StringT text);
	void SendCommand(const char* cmd);
	// send command 'cmd' and wait till the returned output ends with 'eod'
	// returns false if no eod after timeout ms
	bool SendCommandSynch(const char* cmd, const char* eod, DWORD timeout, const char** response);
	bool WaitForResponse(const char* eod, DWORD timeout, const char** response);
	void SetEventHandler(EventHandlerT handler, void* data);
	void ReadAndHandleOutput();

private:

	void StartCommand(const char* options, const char* file);
	void PrepAndLaunchRedirectedChild(
		const char* options, const char* file,
		HANDLE hChildStdOut,
		HANDLE hChildStdIn,
		HANDLE hChildStdErr);
	void Notify(char* text);

	volatile bool m_initialised;
	volatile bool m_threadStopped;
	IdT m_id;
	HANDLE m_hChildProcess;
	HANDLE m_hInputWrite;
	HANDLE m_hOutputRead;
	EventHandlerT m_handler;
	void* m_handlerData;

	// synchronised send command
	volatile bool m_synch;
	std::string m_output;
	HANDLE m_outputReady;
	CRITICAL_SECTION m_cs;
	std::string m_response;
};

