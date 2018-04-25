#pragma once

#include <vector>
#include <memory>

#include "GhciTerminal.h"
#include "Ghci.h"

class CGhciTerminalManager
{
public:
	typedef std::shared_ptr<CGhciTerminal> CGhciTerminalPtrT;
	typedef std::vector<CGhciTerminalPtrT> SGhciTerminalsT;

	static CGhciTerminalManager& Instance();
	~CGhciTerminalManager();

	bool Initialise();
	void Uninitialise();
	HWND New(HWND parent, const char* options, const char* file, const char* directory);
	void SetEventHandler(HWND hwnd, CGhciTerminal::EventHandlerT callback);
	void EnableEvents(HWND hwnd);
	void DisableEvents(HWND hwnd);
	void CloseGhci(HWND hwnd);
	void Paste(HWND hwnd);
	void Cut(HWND hwnd);
	void Copy(HWND hwnd);
	void SelectAll(HWND hwnd);
	bool HasFocus(HWND hwnd);
	void SendCommand(HWND hwnd, char* cmd);
	void SendCommandAsynch(HWND hwnd, const char* cmd, const char* sod, const char* eod);
	bool SendCommandSynch(HWND hwnd, const char* cmd, const char* eod, DWORD timeout, const char** response);
	bool WaitForResponse(HWND hwnd, const char* eod, DWORD timeout, const char** response);
	bool IsTextSelected(HWND hwnd);
	void SetFocus(HWND hwnd);
	int  GetTextLength(HWND hwnd);
	int  GetText(HWND hwnd, char* buff, int size);
	void Clear(HWND hwnd);
	void WndProcRetHook(LPCWPRETSTRUCT pData);
	void GetMsgProc(LPMSG pData);

private:

	CGhciTerminalManager();

	SGhciTerminalsT m_ghciTerminals;
	HMODULE m_hdll;
};


