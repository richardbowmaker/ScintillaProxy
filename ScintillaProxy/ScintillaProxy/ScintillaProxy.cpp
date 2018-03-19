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
#include "GhciTerminal.h"
#include "ScintillaManager.h"

CGhciManager		ghciMgr;
CScintillaManager	scintillaMgr;

HHOOK winHookRet  = 0; // the windows hook to capture windows messages
HHOOK winHookPost = 0; 

LRESULT WINAPI WndProcRetHook(int nCode, WPARAM wParam, LPARAM lParam)
{
	LPCWPRETSTRUCT pData = reinterpret_cast<LPCWPRETSTRUCT>(lParam);

	if (nCode < 0)
	{
		return CallNextHookEx(winHookRet, nCode, wParam, lParam);
	}

	ghciMgr.WndProcRetHook(pData);
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

	ghciMgr.GetMsgProc(pData);
	scintillaMgr.GetMsgProc(pData);
	return CallNextHookEx(winHookPost, nCode, wParam, lParam);
}

// Initialise the DLL
void Initialise()
{
	scintillaMgr.Initialise();
	ghciMgr.Initialise();

	// add windows hook if this is the first editor
	if (winHookRet == 0)
	{
		winHookRet  = SetWindowsHookEx(WH_CALLWNDPROCRET, &WndProcRetHook, 0, ::GetCurrentThreadId());
		winHookPost = SetWindowsHookEx(WH_GETMESSAGE,     &GetMsgProc,     0, ::GetCurrentThreadId());
	}
}

void Uninitialise()
{
	if (winHookRet != 0)
	{
		UnhookWindowsHookEx(winHookRet);
		UnhookWindowsHookEx(winHookPost);
		winHookRet  = 0;
		winHookPost = 0;
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

void ScnAddPopupMenuItem(HWND scintilla, int id, char* title, void* callback)
{
	scintillaMgr.AddPopupMenuItem(scintilla, id, title, (CScintillaEditor::MenuHandlerT)callback);
}

HWND GhciNew(HWND parent, char* options, char* file)
{
	return ghciMgr.NewGhci(parent, options, file);
}

void GhciClose(HWND hwnd)
{
	ghciMgr.CloseGhci(hwnd);
}

void GhciPaste(HWND hwnd)
{
	ghciMgr.Paste(hwnd);
}

void GhciCut(HWND hwnd)
{
	ghciMgr.Cut(hwnd);
}

void GhciCopy(HWND hwnd)
{
	ghciMgr.Copy(hwnd);
}

void GhciSelectAll(HWND hwnd)
{
	ghciMgr.SelectAll(hwnd);
}

BOOL GhciHasFocus(HWND hwnd)
{
	return (BOOL)ghciMgr.HasFocus(hwnd);
}

void GhciSetEventHandler(HWND hwnd, void* callback)
{
	ghciMgr.SetEventHandler(hwnd, (CGhciTerminal::EventHandlerT)callback);
}

void GhciEnableEvents(HWND hwnd)
{
	ghciMgr.EnableEvents(hwnd);
}

void GhciDisableEvents(HWND hwnd)
{
	ghciMgr.DisableEvents(hwnd);
}

void GhciSendCommand(HWND hwnd, char* cmd)
{
	ghciMgr.SendCommand(hwnd, cmd);
}

BOOL GhciIsTextSelected(HWND hwnd)
{
	return ghciMgr.IsTextSelected(hwnd);
}

void GhciSetFocus(HWND hwnd)
{
	ghciMgr.SetFocus(hwnd);
}

int GhciGetTextLength(HWND hwnd)
{
	return ghciMgr.GetTextLength(hwnd);
}

int GhciGetText(HWND hwnd, char* buff, int size)
{
	return ghciMgr.GetText(hwnd, buff, size);
}

void GhciClear(HWND hwnd)
{
	return ghciMgr.Clear(hwnd);
}










