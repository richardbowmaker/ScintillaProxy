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

	void Uninitialise();
	const HWND NewGhci(HWND parent, char* file);
	void CloseGhci(HWND ghci);
	void WndProcRetHook(int nCode, WPARAM wParam, LPARAM lParam);


private:

	SGhcisT m_ghcis;
};

