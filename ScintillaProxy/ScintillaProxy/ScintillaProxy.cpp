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

CGhciManager			ghciMgr;
CGhciTerminalManager	ghciTerminalMgr;
CScintillaManager		scintillaMgr;

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

	ghciTerminalMgr.WndProcRetHook(pData);
	scintillaMgr.WndProcRetHook(pData);
	return CallNextHookEx(winHookRet, nCode, wParam, lParam);
}

LRESULT CALLBACK GetMsgProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	LPMSG pData = reinterpret_cast<LPMSG>(lParam);

	if (nCode < 0)
	{
		return CallNextHookEx(winHookPost, nCode, wParam, lParam);
	}

	ghciTerminalMgr.GetMsgProc(pData);
	scintillaMgr.GetMsgProc(pData);
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
		bool sok = scintillaMgr.Initialise();
		bool gok = ghciTerminalMgr.Initialise();
		winHookRet  = SetWindowsHookEx(WH_CALLWNDPROCRET, &WndProcRetHook, 0, ::GetCurrentThreadId());
		winHookPost = SetWindowsHookEx(WH_GETMESSAGE, &GetMsgProc, 0, ::GetCurrentThreadId());

		if (sok && gok && winHookRet != 0 && winHookPost != 0)
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
	scintillaMgr.Uninitialise();
	ghciTerminalMgr.Uninitialise();
	isInitialised = false;
}

// start a new scintilla editor window
HWND ScnNewEditor(HWND parent)
{
	return scintillaMgr.NewEditor(parent);
}

void ScnDestroyEditor(HWND scintilla)
{
	scintillaMgr.DestroyEditor(scintilla);
}

LRESULT ScnSendEditor(HWND scintilla, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	return scintillaMgr.SendEditor(scintilla, Msg, wParam, lParam);
}

void ScnSetEventHandler(HWND scintilla, void* callback)
{
	return scintillaMgr.SetEventHandler(scintilla, (CScintillaEditor::EventHandlerT)callback);
}

void ScnEnableEvents(HWND scintilla)
{
	scintillaMgr.EnableEvents(scintilla);
}

void ScnDisableEvents(HWND scintilla)
{
	scintillaMgr.DisableEvents(scintilla);
}

void ScnAddPopupMenuItem(HWND scintilla, int id, char* title, void* handler, void* enabled)
{
	scintillaMgr.AddPopupMenuItem(scintilla, id, title, 
		(CScintillaEditor::MenuHandlerT)handler,
		(CScintillaEditor::MenuEnabledT)enabled);
}

HWND GhciTerminalNew(HWND parent, char* options, char* file)
{
	return ghciTerminalMgr.New(parent, options, file);
}

void GhciTerminalClose(HWND hwnd)
{
	ghciTerminalMgr.CloseGhci(hwnd);
}

void GhciTerminalPaste(HWND hwnd)
{
	ghciTerminalMgr.Paste(hwnd);
}

void GhciTerminalCut(HWND hwnd)
{
	ghciTerminalMgr.Cut(hwnd);
}

void GhciTerminalCopy(HWND hwnd)
{
	ghciTerminalMgr.Copy(hwnd);
}

void GhciTerminalSelectAll(HWND hwnd)
{
	ghciTerminalMgr.SelectAll(hwnd);
}

BOOL GhciTerminalHasFocus(HWND hwnd)
{
	return (BOOL)ghciTerminalMgr.HasFocus(hwnd);
}

void GhciTerminalSetEventHandler(HWND hwnd, void* callback)
{
	ghciTerminalMgr.SetEventHandler(hwnd, (CGhciTerminal::EventHandlerT)callback);
}

void GhciTerminalEnableEvents(HWND hwnd)
{
	ghciTerminalMgr.EnableEvents(hwnd);
}

void GhciTerminalDisableEvents(HWND hwnd)
{
	ghciTerminalMgr.DisableEvents(hwnd);
}

void GhciTerminalSendCommand(HWND hwnd, char* cmd)
{
	ghciTerminalMgr.SendCommand(hwnd, cmd);
}

BOOL GhciTerminalIsTextSelected(HWND hwnd)
{
	return ghciTerminalMgr.IsTextSelected(hwnd);
}

void GhciTerminalSetFocus(HWND hwnd)
{
	ghciTerminalMgr.SetFocus(hwnd);
}

int GhciTerminalGetTextLength(HWND hwnd)
{
	return ghciTerminalMgr.GetTextLength(hwnd);
}

int GhciTerminalGetText(HWND hwnd, char* buff, int size)
{
	return ghciTerminalMgr.GetText(hwnd, buff, size);
}

void GhciTerminalClear(HWND hwnd)
{
	return ghciTerminalMgr.Clear(hwnd);
}










