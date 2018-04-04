#include "stdafx.h"
#include "GhciManager.h"
#include "GhciTerminal.h"
#include "Ghci.h"

#include <algorithm>
#include <functional>

// The GHCI manager class //

CGhciManager::CGhciManager()
{
}

CGhciManager::~CGhciManager()
{
	Uninitialise();
}

bool CGhciManager::Initialise()
{
}

void CGhciManager::Uninitialise()
{
	for (SGhcisT::iterator itr = m_ghcis.begin(); itr != m_ghcis.end(); ++itr)
	{
		(*itr)->Uninitialise();
	}
	m_ghcis.clear();

}

HWND CGhciManager::New(HWND parent, char* options, char* file)
{
	if (parent == NULL || m_hdll == NULL) return NULL;

	// if parent already has a GHCI terminal return it
	SGhcisT::const_iterator itr = std::find_if(m_ghcis.begin(), m_ghcis.end(),
		[=](CGhciTerminalPtrT& ptrGhci) -> bool { return ptrGhci->GetParentHwnd() == parent; });
	if (itr != m_ghcis.end()) return (*itr)->GetHwnd();

	// start a new GHCI terminal
	CGhciTerminalPtrT ptrGhci = CGhciTerminalPtrT(new CGhciTerminal);
	if (ptrGhci->Initialise(parent, options, file))
	{
		// add to list and return it
		m_ghcis.push_back(ptrGhci);
		return ptrGhci->GetHwnd();
	}
	else
	{
		return NULL;
	}
}

// either the GHCI hwnd or parent will do
void CGhciManager::Close(HWND hwnd)
{
	SGhcisT::const_iterator itr = std::find_if(m_ghcis.begin(), m_ghcis.end(),
		[=](CGhciTerminalPtrT& ptrGhci) -> bool { return (ptrGhci->GetHwnd() == hwnd || ptrGhci->GetParentHwnd() == hwnd); });

	// uninitialise terminal and remove it
	if (itr != m_ghcis.end())
	{
		(*itr)->Uninitialise();
		m_ghcis.erase(itr);
	}
}

void CGhciManager::SetEventHandler(HWND hwnd, CGhciTerminal::EventHandlerT callback)
{
	SGhcisT::const_iterator itr = std::find_if(m_ghcis.begin(), m_ghcis.end(),
		[=](CGhciTerminalPtrT& ptrGhci) -> bool { return (ptrGhci->GetHwnd() == hwnd || ptrGhci->GetParentHwnd() == hwnd); });

	if (itr != m_ghcis.end())
	{
		(*itr)->SetEventHandler(callback);
	}
}

