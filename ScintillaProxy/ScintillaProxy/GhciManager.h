#pragma once


#include <vector>
#include <memory>

#include "GhciTerminal.h"

class CGhciManager
{
public:

	typedef std::shared_ptr<CGhciTerminal> CGhciTerminalPtrT;
	typedef std::vector<CGhciTerminalPtrT> SGhcisT;

	CGhciManager();
	~CGhciManager();

	bool Initialise();
	void Uninitialise();
	HWND NewGhci(HWND parent, char* options, char* file);
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
	bool IsTextSelected(HWND hwnd);
	void WndProcRetHook(int nCode, WPARAM wParam, LPARAM lParam);

private:

	SGhcisT m_ghcis;
	HMODULE m_hdll;
};

