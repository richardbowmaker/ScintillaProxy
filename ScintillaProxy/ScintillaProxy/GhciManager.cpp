#include "stdafx.h"
#include "GhciManager.h"
#include "GhciTerminal.h"

#include <algorithm>

// The GHCI manager class //


CGhciManager::CGhciManager() : m_hdll(NULL)
{
}

CGhciManager::~CGhciManager()
{
	Uninitialise();
}

bool CGhciManager::Initialise()
{
	m_hdll = ::LoadLibrary(TEXT("Msftedit.dll"));
	return (m_hdll != NULL);
}

void CGhciManager::Uninitialise()
{
	for (SGhcisT::iterator itr = m_ghcis.begin(); itr != m_ghcis.end(); ++itr)
	{
		(*itr)->Uninitialise();
	}
	m_ghcis.clear();
}

const HWND CGhciManager::NewGhci(HWND parent, char* options, char* file)
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
void CGhciManager::CloseGhci(HWND hwnd)
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

void CGhciManager::WndProcRetHook(int nCode, WPARAM wParam, LPARAM lParam)
{
	// if the source of the message is either the editor or its parent
	// then pass it on
	LPCWPRETSTRUCT pData = reinterpret_cast<LPCWPRETSTRUCT>(lParam);

	for (SGhcisT::iterator itr = m_ghcis.begin(); itr != m_ghcis.end(); ++itr)
	{
		if ((*itr)->GetHwnd() == pData->hwnd || (*itr)->GetParentHwnd() == pData->hwnd)
		{
			(*itr)->WndProcRetHook(nCode, wParam, lParam);
		}
	}
}
