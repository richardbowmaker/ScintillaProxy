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
#include "ScintillaManager.h"

CGhciManager		ghciMgr;
CScintillaManager	scintillaMgr;

HHOOK winHook = 0; // the windows hook to capture windows messages

// Initialise the DLL
void Initialise()
{
	scintillaMgr.Initialise();
	ghciMgr.Initialise();

	// add windows hook if this is the first editor
	if (winHook == 0)
	{
		winHook = SetWindowsHookEx(WH_CALLWNDPROCRET, &WndProcRetHook, 0, ::GetCurrentThreadId());
	}
}

void Uninitialise()
{
	if (winHook != 0)
	{
		UnhookWindowsHookEx(winHook);
		winHook = 0;
	}

	scintillaMgr.Uninitialise();
	ghciMgr.Uninitialise();
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
	return scintillaMgr.SetEventHandler(scintilla, callback);
}

void ScnEnableEvents(HWND scintilla)
{
	scintillaMgr.EnableEvents(scintilla);
}

void ScnDisableEvents(HWND scintilla)
{
	scintillaMgr.DisableEvents(scintilla);
}

HWND GhciNew(HWND parent, char* options, char* file)
{
	return ghciMgr.NewGhci(parent, options, file);
}

void GhciClose(HWND hwnd)
{
	ghciMgr.CloseGhci(hwnd);
}

LRESULT WINAPI WndProcRetHook(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode < 0)
	{
		return CallNextHookEx(winHook, nCode, wParam, lParam);
	}

	ghciMgr.WndProcRetHook(nCode, wParam, lParam);
	scintillaMgr.WndProcRetHook(nCode, wParam, lParam);
	return CallNextHookEx(winHook, nCode, wParam, lParam);
}




