// ScintillaProxy.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"

#include <map>
#include <memory>
#include <Scintilla.h>
#include <SciLexer.h>
#include <tchar.h>
#include <Windows.h>

#include "ScintillaProxy.h"
#include "GhciManager.h"
#include "GhciTerminalManager.h"
#include "GhciTerminal.h"
#include "ScintillaManager.h"

HHOOK winHookRet  = 0; // the windows hook to capture windows messages
HHOOK winHookPost = 0; 
bool isInitialised = false;

LRESULT WINAPI WndProcRetHook(int nCode, WPARAM wParam, LPARAM lParam)
{
	LPCWPRETSTRUCT pData = reinterpret_cast<LPCWPRETSTRUCT>(lParam);

	if (nCode < 0)
	{
		return CallNextHookEx(winHookRet, nCode, wParam, lParam);
	}

	CGhciTerminalManager::Instance().WndProcRetHook(pData);
	CScintillaManager::Instance().WndProcRetHook(pData);
	return CallNextHookEx(winHookRet, nCode, wParam, lParam);
}

LRESULT CALLBACK GetMsgProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	LPMSG pData = reinterpret_cast<LPMSG>(lParam);

	if (nCode < 0)
	{
		return CallNextHookEx(winHookPost, nCode, wParam, lParam);
	}

	CGhciTerminalManager::Instance().GetMsgProc(pData);
	CScintillaManager::Instance().GetMsgProc(pData);
	return CallNextHookEx(winHookPost, nCode, wParam, lParam);
}

BOOL ScintillaProxyInitialise()
{
	return (BOOL)Initialise();
}

void ScintillaProxyUninitialise()
{
	Uninitialise();
}

// Initialise the DLL
bool Initialise()
{
	if (!isInitialised)
	{
		bool sok = CScintillaManager::Instance().Initialise();
		bool gok = CGhciManager::Instance().Initialise();
		bool tok = CGhciTerminalManager::Instance().Initialise();
		winHookRet  = SetWindowsHookEx(WH_CALLWNDPROCRET, &WndProcRetHook, 0, ::GetCurrentThreadId());
		winHookPost = SetWindowsHookEx(WH_GETMESSAGE, &GetMsgProc, 0, ::GetCurrentThreadId());

		if (sok && gok && tok && winHookRet != 0 && winHookPost != 0)
		{ 
			isInitialised = true;
		}
		else
		{
			Uninitialise();
		}
	}
	return isInitialised;
}

void Uninitialise()
{
	if (winHookRet != 0)
	{
		UnhookWindowsHookEx(winHookRet);
		winHookRet = 0;
	}
	if (winHookPost != 0)
	{
		UnhookWindowsHookEx(winHookPost);
		winHookPost = 0;
	}
	CScintillaManager::Instance().Uninitialise();
	CGhciTerminalManager::Instance().Uninitialise();
	CGhciManager::Instance().Uninitialise();
	isInitialised = false;
}

// start a new scintilla editor window
HWND ScnNewEditor(HWND parent)
{
	return CScintillaManager::Instance().NewEditor(parent);
}

void ScnDestroyEditor(HWND scintilla)
{
	CScintillaManager::Instance().DestroyEditor(scintilla);
}

LRESULT ScnSendEditor(HWND scintilla, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	return CScintillaManager::Instance().SendEditor(scintilla, Msg, wParam, lParam);
}

void ScnSetEventHandler(HWND scintilla, void* callback)
{
	return CScintillaManager::Instance().SetEventHandler(scintilla, (CScintillaEditor::EventHandlerT)callback);
}

void ScnEnableEvents(HWND scintilla)
{
	CScintillaManager::Instance().EnableEvents(scintilla);
}

void ScnDisableEvents(HWND scintilla)
{
	CScintillaManager::Instance().DisableEvents(scintilla);
}

void ScnAddPopupMenuItem(HWND scintilla, int id, char* title, void* handler, void* enabled)
{
	CScintillaManager::Instance().AddPopupMenuItem(scintilla, id, title, 
		(CScintillaEditor::MenuHandlerT)handler,
		(CScintillaEditor::MenuEnabledT)enabled);
}

HWND GhciTerminalNew(HWND parent, const char* options, const char* file)
{
	return CGhciTerminalManager::Instance().New(parent, options, file);
}

void GhciTerminalClose(HWND hwnd)
{
	CGhciTerminalManager::Instance().CloseGhci(hwnd);
}

void GhciTerminalPaste(HWND hwnd)
{
	CGhciTerminalManager::Instance().Paste(hwnd);
}

void GhciTerminalCut(HWND hwnd)
{
	CGhciTerminalManager::Instance().Cut(hwnd);
}

void GhciTerminalCopy(HWND hwnd)
{
	CGhciTerminalManager::Instance().Copy(hwnd);
}

void GhciTerminalSelectAll(HWND hwnd)
{
	CGhciTerminalManager::Instance().SelectAll(hwnd);
}

BOOL GhciTerminalHasFocus(HWND hwnd)
{
	return (BOOL)CGhciTerminalManager::Instance().HasFocus(hwnd);
}

void GhciTerminalSetEventHandler(HWND hwnd, void* callback)
{
	CGhciTerminalManager::Instance().SetEventHandler(hwnd, (CGhciTerminal::EventHandlerT)callback);
}

void GhciTerminalEnableEvents(HWND hwnd)
{
	CGhciTerminalManager::Instance().EnableEvents(hwnd);
}

void GhciTerminalDisableEvents(HWND hwnd)
{
	CGhciTerminalManager::Instance().DisableEvents(hwnd);
}

void GhciTerminalSendCommand(HWND hwnd, char* cmd)
{
	CGhciTerminalManager::Instance().SendCommand(hwnd, cmd);
}

BOOL GhciTerminalIsTextSelected(HWND hwnd)
{
	return CGhciTerminalManager::Instance().IsTextSelected(hwnd);
}

void GhciTerminalSetFocus(HWND hwnd)
{
	CGhciTerminalManager::Instance().SetFocus(hwnd);
}

int GhciTerminalGetTextLength(HWND hwnd)
{
	return CGhciTerminalManager::Instance().GetTextLength(hwnd);
}

int GhciTerminalGetText(HWND hwnd, char* buff, int size)
{
	return CGhciTerminalManager::Instance().GetText(hwnd, buff, size);
}

void GhciTerminalClear(HWND hwnd)
{
	return CGhciTerminalManager::Instance().Clear(hwnd);
}

int GhciNew(const char* options, const char* file)
{
	CGhci::CGhciPtrT ptr = CGhciManager::Instance().New(options, file);
	if (ptr != NULL)
	{
		return ptr->GetId();
	}
	else
	{
		return 0;
	}
}

void GhciClose(int id)
{
	CGhciManager::Instance().Close(id);
}

void GhciSetEventHandler(int id, void* callback, void* data)
{
	CGhciManager::Instance().SetEventHandler(id, (CGhci::EventHandlerT)callback, data);
}

void GhciSendCommand(int id, const char* cmd)
{
	CGhciManager::Instance().SendCommand(id, cmd);
}

BOOL GhciSendCommandSynch(int id, const char* cmd, const char* eod, DWORD timeout, const char** response)
{
	return (BOOL)CGhciManager::Instance().SendCommandSynch(id, cmd, eod, timeout, response);
}

BOOL GhciWaitForResponse(int id, const char* eod, DWORD timeout, const char** response)
{
	return (BOOL)CGhciManager::Instance().WaitForResponse(id, eod, timeout, response);
}














