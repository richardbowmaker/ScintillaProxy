#include "stdafx.h"
#include "GhciManager.h"
#include "GhciTerminal.h"
#include "Ghci.h"

#include <algorithm>
#include <functional>

// The GHCI manager class //

CGhciManager::CGhciManager() : m_idNext(0)
{
}

CGhciManager::~CGhciManager()
{
	Uninitialise();
}

CGhciManager& CGhciManager::Instance()
{
	static CGhciManager instance;
	return instance;
}

bool CGhciManager::Initialise()
{
	return true;
}

void CGhciManager::Uninitialise()
{
	for (SGhcisT::iterator itr = m_ghcis.begin(); itr != m_ghcis.end(); ++itr)
	{
		(*itr)->Uninitialise();
	}
	m_ghcis.clear();
}

CGhci::CGhci::CGhciPtrT CGhciManager::New(const char* options, const char* file, const char* directory)
{
	CGhci::CGhciPtrT ptrGhci = CGhci::CGhciPtrT(new CGhci);
	m_idNext++;
	if (ptrGhci->Initialise(m_idNext, options, file, directory))
	{
		// add to list and return it
		m_ghcis.push_back(ptrGhci);
		return ptrGhci;
	}
	else
	{
		return NULL;
	}
}

CGhci::CGhciPtrT CGhciManager::GetChci(CGhci::IdT id)
{
	SGhcisT::const_iterator itr = std::find_if(m_ghcis.begin(), m_ghcis.end(),
		[=](CGhci::CGhciPtrT& ptrGhci) -> bool { return ptrGhci->GetId() == id; });

	// uninitialise terminal and remove it
	if (itr != m_ghcis.end())
	{
		return *itr;
	}
	else
	{
		return NULL;
	}
}

// either the GHCI hwnd or parent will do
void CGhciManager::Close(CGhci::IdT id)
{
	SGhcisT::const_iterator itr = std::find_if(m_ghcis.begin(), m_ghcis.end(),
		[=](CGhci::CGhciPtrT& ptrGhci) -> bool { return ptrGhci->GetId() == id; });

	// uninitialise terminal and remove it
	if (itr != m_ghcis.end())
	{
		(*itr)->Uninitialise();
		m_ghcis.erase(itr);
	}
}

void CGhciManager::SetEventHandler(CGhci::IdT id, CGhci::EventHandlerT callback, void* data)
{
	SGhcisT::const_iterator itr = std::find_if(m_ghcis.begin(), m_ghcis.end(),
		[=](CGhci::CGhciPtrT& ptrGhci) -> bool { return ptrGhci->GetId() == id; });

	if (itr != m_ghcis.end())
	{
		(*itr)->SetEventHandler(callback, data);
	}
}

void CGhciManager::SendCommand(CGhci::IdT id, CUtils::StringT text)
{
	SGhcisT::const_iterator itr = std::find_if(m_ghcis.begin(), m_ghcis.end(),
		[=](CGhci::CGhciPtrT& ptrGhci) -> bool { return ptrGhci->GetId() == id; });

	if (itr != m_ghcis.end())
	{
		(*itr)->SendCommand(text);
	}
}

void CGhciManager::SendCommand(CGhci::IdT id, const char* cmd)
{
	SGhcisT::const_iterator itr = std::find_if(m_ghcis.begin(), m_ghcis.end(),
		[=](CGhci::CGhciPtrT& ptrGhci) -> bool { return ptrGhci->GetId() == id; });

	if (itr != m_ghcis.end())
	{
		(*itr)->SendCommand(cmd);
	}
}

void CGhciManager::SendCommandAsynch(CGhci::IdT id, const char* cmd, const char* eod)
{
	SGhcisT::const_iterator itr = std::find_if(m_ghcis.begin(), m_ghcis.end(),
		[=](CGhci::CGhciPtrT& ptrGhci) -> bool { return ptrGhci->GetId() == id; });

	if (itr != m_ghcis.end())
	{
		(*itr)->SendCommandAsynch(cmd, eod);
	}
}

// send command 'cmd' and wait till the returned output ends with 'eod'
// returns false if no eod after timeout ms
bool CGhciManager::SendCommandSynch(CGhci::IdT id, const char* cmd, const char* eod, DWORD timeout, const char** response)
{
	SGhcisT::const_iterator itr = std::find_if(m_ghcis.begin(), m_ghcis.end(),
		[=](CGhci::CGhciPtrT& ptrGhci) -> bool { return ptrGhci->GetId() == id; });

	if (itr != m_ghcis.end())
	{
		return (*itr)->SendCommandSynch(cmd, eod, timeout, response);
	}
	else
	{
		return false;
	}
}

bool CGhciManager::WaitForResponse(CGhci::IdT id, const char* eod, DWORD timeout, const char** response)
{
	SGhcisT::const_iterator itr = std::find_if(m_ghcis.begin(), m_ghcis.end(),
		[=](CGhci::CGhciPtrT& ptrGhci) -> bool { return ptrGhci->GetId() == id; });

	if (itr != m_ghcis.end())
	{
		return (*itr)->WaitForResponse(eod, timeout, response);
	}
	else
	{
		return false;
	}
}



