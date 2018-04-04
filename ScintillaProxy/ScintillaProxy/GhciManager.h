#pragma once


#include <vector>
#include <memory>

#include "GhciTerminal.h"
#include "Ghci.h"


class CGhciManager
{
public:

	typedef std::shared_ptr<CGhci> CGhciPtrT;
	typedef std::vector<CGhciPtrT> SGhcisT;

	CGhciManager();
	~CGhciManager();

	bool Initialise();
	void Uninitialise();
	HWND New(HWND parent, char* options, char* file);
	void SetEventHandler(HWND hwnd, CGhciTerminal::EventHandlerT callback);


private:


	SGhcisT m_ghcis;

};

