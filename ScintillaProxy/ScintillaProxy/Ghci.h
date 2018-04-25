#pragma once

#include <memory>

#include "CUtils.h"

class CGhci
{
public:

	enum EventT
	{
		EventError = 0x01,
		EventOutput = 0x02,
		EventInput = 0x04,
		EventAsynchOutput = 0x08
	};

	// GHCI id, event type, output, userdata
	typedef void (*EventHandlerT)(int, int, const char*, void*);
	typedef unsigned __int32 IdT;
	typedef std::shared_ptr<CGhci> CGhciPtrT;

	CGhci();
	~CGhci();

	bool Initialise(IdT id, const char* options, const char* file, const char* directory);
	void Uninitialise();

	IdT GetId();

	void SendCommand(CUtils::StringT text);
	void SendCommand(const char* cmd);
	void SendCommandAsynch(const char* cmd, const char* sod, const char* eod);
	// send command 'cmd' and wait till the returned output ends with 'eod'
	// returns false if no eod after timeout ms
	bool SendCommandSynch(const char* cmd, const char* eod, DWORD timeout, const char** response);
	bool WaitForResponse(const char* eod, DWORD timeout, const char** response);
	void SetEventHandler(EventHandlerT handler, void* data);
	void ReadAndHandleOutput();

private:

	enum ModeT {Echo, Synch, AsynchStarted, AsynchWaitEod};

	bool StartCommand(const char* options, const char* file, const char* directory);
	void PrepAndLaunchRedirectedChild(
		const char* options, 
		const char* file,
		const char* directory,
		HANDLE hChildStdOut,
		HANDLE hChildStdIn,
		HANDLE hChildStdErr);
	void Notify(EventT event, const char* text);

	volatile bool m_initialised;
	volatile bool m_threadStopped;
	IdT m_id;
	HANDLE m_hInputWrite;
	HANDLE m_hOutputRead;
	PROCESS_INFORMATION m_pi;
	STARTUPINFO m_si;
	EventHandlerT m_handler;
	void* m_handlerData;

	// synchronised send command
	volatile ModeT m_mode;
	std::string m_output;
	HANDLE m_outputReady;
	CRITICAL_SECTION m_cs;
	std::string m_response;

	// asynch send, start of resposne and end of response
	std::string m_sod;
	std::string m_eod;
};

