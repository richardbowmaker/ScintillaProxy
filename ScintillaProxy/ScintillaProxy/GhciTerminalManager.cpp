

#include "stdafx.h"
#include "GhciTerminalManager.h"
#include "GhciTerminal.h"
#include "Ghci.h"

#include <algorithm>
#include <functional>

// The GHCI manager class //

CGhciTerminalManager::CGhciTerminalManager() : m_hdll(NULL)
{
}

CGhciTerminalManager::~CGhciTerminalManager()
{
	Uninitialise();
}

bool CGhciTerminalManager::Initialise()
{
	m_hdll = ::LoadLibrary(TEXT("Msftedit.dll"));
	return (m_hdll != NULL);
}

void CGhciTerminalManager::Uninitialise()
{
	for (SGhciTerminalsT::iterator itr = m_ghciTerminals.begin(); itr != m_ghciTerminals.end(); ++itr)
	{
		(*itr)->Uninitialise();
	}
	m_ghciTerminals.clear();

	if (m_hdll)
	{
		::FreeLibrary(m_hdll);
	}
}

void CGhciTerminalManager::WndProcRetHook(LPCWPRETSTRUCT pData)
{
	// if the source of the message is either the editor or its parent
	// then pass it on
	for (SGhciTerminalsT::iterator itr = m_ghciTerminals.begin(); itr != m_ghciTerminals.end(); ++itr)
	{
		if ((*itr)->GetHwnd() == pData->hwnd || (*itr)->GetParentHwnd() == pData->hwnd)
		{
			(*itr)->WndProcRetHook(pData);
		}
	}
}

void CGhciTerminalManager::GetMsgProc(LPMSG pData)
{
	for (SGhciTerminalsT::iterator itr = m_ghciTerminals.begin(); itr != m_ghciTerminals.end(); ++itr)
	{
		if ((*itr)->GetHwnd() == pData->hwnd || (*itr)->GetParentHwnd() == pData->hwnd)
		{
			(*itr)->GetMsgProc(pData);
		}
	}
}

HWND CGhciTerminalManager::New(HWND parent, char* options, char* file)
{
	if (parent == NULL || m_hdll == NULL) return NULL;

	// if parent already has a GHCI terminal return it
	SGhciTerminalsT::const_iterator itr = std::find_if(m_ghciTerminals.begin(), m_ghciTerminals.end(),
		[=](CGhciTerminalPtrT& ptrGhci) -> bool { return ptrGhci->GetParentHwnd() == parent; });
	if (itr != m_ghciTerminals.end()) return (*itr)->GetHwnd();

	// start a new GHCI terminal
	CGhciTerminalPtrT ptrGhci = CGhciTerminalPtrT(new CGhciTerminal);
	if (ptrGhci->Initialise(parent, options, file))
	{
		// add to list and return it
		m_ghciTerminals.push_back(ptrGhci);
		return ptrGhci->GetHwnd();
	}
	else
	{
		return NULL;
	}
}

// either the GHCI hwnd or parent will do
void CGhciTerminalManager::CloseGhci(HWND hwnd)
{
	SGhciTerminalsT::const_iterator itr = std::find_if(m_ghciTerminals.begin(), m_ghciTerminals.end(),
		[=](CGhciTerminalPtrT& ptrGhci) -> bool { return (ptrGhci->GetHwnd() == hwnd || ptrGhci->GetParentHwnd() == hwnd); });

	// uninitialise terminal and remove it
	if (itr != m_ghciTerminals.end())
	{
		(*itr)->Uninitialise();
		m_ghciTerminals.erase(itr);
	}
}

void CGhciTerminalManager::Paste(HWND hwnd)
{
	SGhciTerminalsT::const_iterator itr = std::find_if(m_ghciTerminals.begin(), m_ghciTerminals.end(),
		[=](CGhciTerminalPtrT& ptrGhci) -> bool { return (ptrGhci->GetHwnd() == hwnd || ptrGhci->GetParentHwnd() == hwnd); });

	if (itr != m_ghciTerminals.end())
	{
		(*itr)->Paste();
	}
}

void CGhciTerminalManager::Cut(HWND hwnd)
{
	SGhciTerminalsT::const_iterator itr = std::find_if(m_ghciTerminals.begin(), m_ghciTerminals.end(),
		[=](CGhciTerminalPtrT& ptrGhci) -> bool { return (ptrGhci->GetHwnd() == hwnd || ptrGhci->GetParentHwnd() == hwnd); });

	if (itr != m_ghciTerminals.end())
	{
		(*itr)->Cut();
	}
}

void CGhciTerminalManager::Copy(HWND hwnd)
{
	SGhciTerminalsT::const_iterator itr = std::find_if(m_ghciTerminals.begin(), m_ghciTerminals.end(),
		[=](CGhciTerminalPtrT& ptrGhci) -> bool { return (ptrGhci->GetHwnd() == hwnd || ptrGhci->GetParentHwnd() == hwnd); });

	if (itr != m_ghciTerminals.end())
	{
		(*itr)->Copy();
	}
}

void CGhciTerminalManager::SelectAll(HWND hwnd)
{
	SGhciTerminalsT::const_iterator itr = std::find_if(m_ghciTerminals.begin(), m_ghciTerminals.end(),
		[=](CGhciTerminalPtrT& ptrGhci) -> bool { return (ptrGhci->GetHwnd() == hwnd || ptrGhci->GetParentHwnd() == hwnd); });

	if (itr != m_ghciTerminals.end())
	{
		(*itr)->SelectAll();
	}
}

bool CGhciTerminalManager::HasFocus(HWND hwnd)
{
	SGhciTerminalsT::const_iterator itr = std::find_if(m_ghciTerminals.begin(), m_ghciTerminals.end(),
		[=](CGhciTerminalPtrT& ptrGhci) -> bool { return (ptrGhci->GetHwnd() == hwnd || ptrGhci->GetParentHwnd() == hwnd); });

	if (itr != m_ghciTerminals.end())
	{
		return (*itr)->HasFocus();
	}
	else
	{
		return false;
	}
}

void CGhciTerminalManager::SetEventHandler(HWND hwnd, CGhciTerminal::EventHandlerT callback)
{
	SGhciTerminalsT::const_iterator itr = std::find_if(m_ghciTerminals.begin(), m_ghciTerminals.end(),
		[=](CGhciTerminalPtrT& ptrGhci) -> bool { return (ptrGhci->GetHwnd() == hwnd || ptrGhci->GetParentHwnd() == hwnd); });

	if (itr != m_ghciTerminals.end())
	{
		(*itr)->SetEventHandler(callback);
	}
}

void CGhciTerminalManager::EnableEvents(HWND hwnd)
{
	SGhciTerminalsT::const_iterator itr = std::find_if(m_ghciTerminals.begin(), m_ghciTerminals.end(),
		[=](CGhciTerminalPtrT& ptrGhci) -> bool { return (ptrGhci->GetHwnd() == hwnd || ptrGhci->GetParentHwnd() == hwnd); });

	if (itr != m_ghciTerminals.end())
	{
		(*itr)->EnableEvents();
	}
}

void CGhciTerminalManager::DisableEvents(HWND hwnd)
{
	SGhciTerminalsT::const_iterator itr = std::find_if(m_ghciTerminals.begin(), m_ghciTerminals.end(),
		[=](CGhciTerminalPtrT& ptrGhci) -> bool { return (ptrGhci->GetHwnd() == hwnd || ptrGhci->GetParentHwnd() == hwnd); });

	if (itr != m_ghciTerminals.end())
	{
		(*itr)->DisableEvents();
	}
}

void CGhciTerminalManager::SendCommand(HWND hwnd, char* cmd)
{
	SGhciTerminalsT::const_iterator itr = std::find_if(m_ghciTerminals.begin(), m_ghciTerminals.end(),
		[=](CGhciTerminalPtrT& ptrGhci) -> bool { return (ptrGhci->GetHwnd() == hwnd || ptrGhci->GetParentHwnd() == hwnd); });

	if (itr != m_ghciTerminals.end())
	{
		(*itr)->SendCommand(cmd);
	}
}

bool CGhciTerminalManager::IsTextSelected(HWND hwnd)
{
	SGhciTerminalsT::const_iterator itr = std::find_if(m_ghciTerminals.begin(), m_ghciTerminals.end(),
		[=](CGhciTerminalPtrT& ptrGhci) -> bool { return (ptrGhci->GetHwnd() == hwnd || ptrGhci->GetParentHwnd() == hwnd); });

	if (itr != m_ghciTerminals.end())
	{
		return (*itr)->IsTextSelected();
	}
	else
	{
		return false;
	}
}

void CGhciTerminalManager::SetFocus(HWND hwnd)
{
	SGhciTerminalsT::const_iterator itr = std::find_if(m_ghciTerminals.begin(), m_ghciTerminals.end(),
		[=](CGhciTerminalPtrT& ptrGhci) -> bool { return (ptrGhci->GetHwnd() == hwnd || ptrGhci->GetParentHwnd() == hwnd); });

	if (itr != m_ghciTerminals.end())
	{
		(*itr)->SetFocus();
	}
}

int CGhciTerminalManager::GetTextLength(HWND hwnd)
{
	SGhciTerminalsT::const_iterator itr = std::find_if(m_ghciTerminals.begin(), m_ghciTerminals.end(),
		[=](CGhciTerminalPtrT& ptrGhci) -> bool { return (ptrGhci->GetHwnd() == hwnd || ptrGhci->GetParentHwnd() == hwnd); });

	if (itr != m_ghciTerminals.end())
	{
		return (*itr)->GetTextLength();
	}
	else
	{
		return 0;
	}
}

int CGhciTerminalManager::GetText(HWND hwnd, char* buff, int size)
{
	SGhciTerminalsT::const_iterator itr = std::find_if(m_ghciTerminals.begin(), m_ghciTerminals.end(),
		[=](CGhciTerminalPtrT& ptrGhci) -> bool { return (ptrGhci->GetHwnd() == hwnd || ptrGhci->GetParentHwnd() == hwnd); });

	if (itr != m_ghciTerminals.end())
	{
		return (*itr)->GetText(buff, size);
	}
	else
	{
		return 0;
	}
}

void CGhciTerminalManager::Clear(HWND hwnd)
{
	SGhciTerminalsT::const_iterator itr = std::find_if(m_ghciTerminals.begin(), m_ghciTerminals.end(),
		[=](CGhciTerminalPtrT& ptrGhci) -> bool { return (ptrGhci->GetHwnd() == hwnd || ptrGhci->GetParentHwnd() == hwnd); });

	if (itr != m_ghciTerminals.end())
	{
		(*itr)->Clear();
	}
}





