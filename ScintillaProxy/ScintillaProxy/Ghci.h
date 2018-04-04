#pragma once

#include "CUtils.h"

class CGhci
{
public:

	typedef void (*EventHandlerT)(char*, void*);

	CGhci();
	~CGhci();

	bool Initialise(char* options, char* file);
	void Uninitialise();

	void SendCommand(CUtils::StringT text);
	void SendCommand(const char* cmd);
	// send command 'cmd' and wait till the returned output ends with 'eod'
	// returns false if no eod after timeout ms
	bool SendCommandSynch(char* cmd, char* eod, DWORD timeout, const char** output);
	void SetEventHandler(EventHandlerT handler, void* data);
	void ReadAndHandleOutput();

private:

	void StartCommand(char* options, char* file);
	void PrepAndLaunchRedirectedChild(
		char* options, char* file,
		HANDLE hChildStdOut,
		HANDLE hChildStdIn,
		HANDLE hChildStdErr);
	void Notify(char* text);

	volatile bool m_initialised;
	volatile bool m_threadStopped;
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
};

