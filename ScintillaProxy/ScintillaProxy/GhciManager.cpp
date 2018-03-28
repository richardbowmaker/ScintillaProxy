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
	if (m_hdll)
	{
		::FreeLibrary(m_hdll);
	}
}

void CGhciManager::WndProcRetHook(LPCWPRETSTRUCT pData)
{
	// if the source of the message is either the editor or its parent
	// then pass it on
	for (SGhcisT::iterator itr = m_ghcis.begin(); itr != m_ghcis.end(); ++itr)
	{
		if ((*itr)->GetHwnd() == pData->hwnd || (*itr)->GetParentHwnd() == pData->hwnd)
		{
			(*itr)->WndProcRetHook(pData);
		}
	}
}

void CGhciManager::GetMsgProc(LPMSG pData)
{
	for (SGhcisT::iterator itr = m_ghcis.begin(); itr != m_ghcis.end(); ++itr)
	{
		if ((*itr)->GetHwnd() == pData->hwnd || (*itr)->GetParentHwnd() == pData->hwnd)
		{
			(*itr)->GetMsgProc(pData);
		}
	}
}

HWND CGhciManager::NewGhci(HWND parent, char* options, char* file)
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

void CGhciManager::Paste(HWND hwnd)
{
	SGhcisT::const_iterator itr = std::find_if(m_ghcis.begin(), m_ghcis.end(),
		[=](CGhciTerminalPtrT& ptrGhci) -> bool { return (ptrGhci->GetHwnd() == hwnd || ptrGhci->GetParentHwnd() == hwnd); });

	if (itr != m_ghcis.end())
	{
		(*itr)->Paste();
	}
}

void CGhciManager::Cut(HWND hwnd)
{
	SGhcisT::const_iterator itr = std::find_if(m_ghcis.begin(), m_ghcis.end(),
		[=](CGhciTerminalPtrT& ptrGhci) -> bool { return (ptrGhci->GetHwnd() == hwnd || ptrGhci->GetParentHwnd() == hwnd); });

	if (itr != m_ghcis.end())
	{
		(*itr)->Cut();
	}
}

void CGhciManager::Copy(HWND hwnd)
{
	SGhcisT::const_iterator itr = std::find_if(m_ghcis.begin(), m_ghcis.end(),
		[=](CGhciTerminalPtrT& ptrGhci) -> bool { return (ptrGhci->GetHwnd() == hwnd || ptrGhci->GetParentHwnd() == hwnd); });

	if (itr != m_ghcis.end())
	{
		(*itr)->Copy();
	}
}

void CGhciManager::SelectAll(HWND hwnd)
{
	SGhcisT::const_iterator itr = std::find_if(m_ghcis.begin(), m_ghcis.end(),
		[=](CGhciTerminalPtrT& ptrGhci) -> bool { return (ptrGhci->GetHwnd() == hwnd || ptrGhci->GetParentHwnd() == hwnd); });

	if (itr != m_ghcis.end())
	{
		(*itr)->SelectAll();
	}
}

bool CGhciManager::HasFocus(HWND hwnd)
{
	SGhcisT::const_iterator itr = std::find_if(m_ghcis.begin(), m_ghcis.end(),
		[=](CGhciTerminalPtrT& ptrGhci) -> bool { return (ptrGhci->GetHwnd() == hwnd || ptrGhci->GetParentHwnd() == hwnd); });

	if (itr != m_ghcis.end())
	{
		return (*itr)->HasFocus();
	}
	else
	{
		return false;
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

void CGhciManager::EnableEvents(HWND hwnd)
{
	SGhcisT::const_iterator itr = std::find_if(m_ghcis.begin(), m_ghcis.end(),
		[=](CGhciTerminalPtrT& ptrGhci) -> bool { return (ptrGhci->GetHwnd() == hwnd || ptrGhci->GetParentHwnd() == hwnd); });

	if (itr != m_ghcis.end())
	{
		(*itr)->EnableEvents();
	}
}

void CGhciManager::DisableEvents(HWND hwnd)
{
	SGhcisT::const_iterator itr = std::find_if(m_ghcis.begin(), m_ghcis.end(),
		[=](CGhciTerminalPtrT& ptrGhci) -> bool { return (ptrGhci->GetHwnd() == hwnd || ptrGhci->GetParentHwnd() == hwnd); });

	if (itr != m_ghcis.end())
	{
		(*itr)->DisableEvents();
	}
}

void CGhciManager::SendCommand(HWND hwnd, char* cmd)
{
	SGhcisT::const_iterator itr = std::find_if(m_ghcis.begin(), m_ghcis.end(),
		[=](CGhciTerminalPtrT& ptrGhci) -> bool { return (ptrGhci->GetHwnd() == hwnd || ptrGhci->GetParentHwnd() == hwnd); });

	if (itr != m_ghcis.end())
	{
		(*itr)->SendCommand(cmd);
	}
}

bool CGhciManager::IsTextSelected(HWND hwnd)
{
	SGhcisT::const_iterator itr = std::find_if(m_ghcis.begin(), m_ghcis.end(),
		[=](CGhciTerminalPtrT& ptrGhci) -> bool { return (ptrGhci->GetHwnd() == hwnd || ptrGhci->GetParentHwnd() == hwnd); });

	if (itr != m_ghcis.end())
	{
		return (*itr)->IsTextSelected();
	}
	else
	{
		return false;
	}
}

void CGhciManager::SetFocus(HWND hwnd)
{
	SGhcisT::const_iterator itr = std::find_if(m_ghcis.begin(), m_ghcis.end(),
		[=](CGhciTerminalPtrT& ptrGhci) -> bool { return (ptrGhci->GetHwnd() == hwnd || ptrGhci->GetParentHwnd() == hwnd); });

	if (itr != m_ghcis.end())
	{
		(*itr)->SetFocus();
	}
}

int CGhciManager::GetTextLength(HWND hwnd)
{
	SGhcisT::const_iterator itr = std::find_if(m_ghcis.begin(), m_ghcis.end(),
		[=](CGhciTerminalPtrT& ptrGhci) -> bool { return (ptrGhci->GetHwnd() == hwnd || ptrGhci->GetParentHwnd() == hwnd); });

	if (itr != m_ghcis.end())
	{
		return (*itr)->GetTextLength();
	}
	else
	{
		return 0;
	}
}

int CGhciManager::GetText(HWND hwnd, char* buff, int size)
{
	SGhcisT::const_iterator itr = std::find_if(m_ghcis.begin(), m_ghcis.end(),
		[=](CGhciTerminalPtrT& ptrGhci) -> bool { return (ptrGhci->GetHwnd() == hwnd || ptrGhci->GetParentHwnd() == hwnd); });

	if (itr != m_ghcis.end())
	{
		return (*itr)->GetText(buff, size);
	}
	else
	{
		return 0;
	}
}

void CGhciManager::Clear(HWND hwnd)
{
	SGhcisT::const_iterator itr = std::find_if(m_ghcis.begin(), m_ghcis.end(),
		[=](CGhciTerminalPtrT& ptrGhci) -> bool { return (ptrGhci->GetHwnd() == hwnd || ptrGhci->GetParentHwnd() == hwnd); });

	if (itr != m_ghcis.end())
	{
		(*itr)->Clear();
	}
}





