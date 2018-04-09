#pragma once


#include <vector>
#include <memory>

#include "GhciTerminal.h"
#include "Ghci.h"


class CGhciManager
{
public:

	typedef std::vector<CGhci::CGhciPtrT> SGhcisT;

	static CGhciManager& Instance();
	~CGhciManager();

	bool Initialise();
	void Uninitialise();
	CGhci::CGhciPtrT New(const char* options, const char* file);
	CGhci::CGhciPtrT GetChci(CGhci::IdT id);
	void Close(CGhci::IdT id);
	void SetEventHandler(CGhci::IdT id, CGhci::EventHandlerT callback, void* data);
	void SendCommand(CGhci::IdT id, CUtils::StringT text);
	void SendCommand(CGhci::IdT id, const char* cmd);
	// send command 'cmd' and wait till the returned output ends with 'eod'
	// returns false if no eod after timeout ms
	void SendCommandAsynch(CGhci::IdT id, const char* cmd, const char* eod);
	bool SendCommandSynch(CGhci::IdT id, const char* cmd, const char* eod, DWORD timeout, const char** response);
	bool WaitForResponse(CGhci::IdT id, const char* eod, DWORD timeout, const char** response);

private:

	CGhciManager();
	SGhcisT m_ghcis;
	CGhci::IdT m_idNext;
};

