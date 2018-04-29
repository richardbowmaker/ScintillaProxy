#pragma once

#include <RichEdit.h>
#include <tchar.h>

#include "CUtils.h"
#include "Ghci.h"

// Terminal window in which an external processes stdin and stdouty are echoed

#define TW_ADDTEXT WM_USER 

class CGhciManager;

class CGhciTerminal
{
public:

	typedef void* (*EventHandlerT)(HWND, int, const char*);

	CGhciTerminal();
	~CGhciTerminal();

	bool Initialise(HWND hParent, const char* options, const char* file, const char* directory);
	void Uninitialise();
	void SetEventHandler(EventHandlerT callback);
	void EnableEvents();
	void DisableEvents();
	void Paste();
	void Cut();
	void Copy();
	void SelectAll();
	void ClearText();
	void SetText(CUtils::StringT text);
	void AddText(CUtils::StringT text);
	void AddLine(CUtils::StringT text);
	void AddTextTS(CUtils::StringT text); // non-blocking, thread safe
	void NewLine();
	HWND GetHwnd() const;
	HWND GetParentHwnd() const;
	int  GetNoOfChars();
	bool IsTextSelected();
	bool HasFocus();
	void SetFocus();
	int  GetTextLength();
	int  GetText(char* buff, int size);
	void Clear();
	void SendCommand(CUtils::StringT text);
	void SendCommand(char* cmd);
	void SendCommandAsynch(const char* cmd, const char* sod, const char* eod);
	// send command 'cmd' and wait till the returned output ends with 'eod'
	// returns false if no eod after timeout ms
	bool SendCommandSynch(const char* cmd, const char* eod, DWORD timeout, const char** response);
	bool WaitForResponse(const char* eod, DWORD timeout, const char** response);
	void WndProcRetHook(LPCWPRETSTRUCT pData);
	void GetMsgProc(LPMSG pData);
	bool RichTextBoxProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam);
	bool ListBoxProc(UINT Msg, WPARAM wParam, LPARAM lParam);
	void GhciEventHandler(int id, int event, const char* text, void* data);

private:

	CGhci::CGhciPtrT m_ghci;
	void Notify(int event, CUtils::StringT text = _T(""));
	void UpdateCommandLine(CUtils::StringT text);
	CUtils::StringT GetCommandLine();
	void SetCommandLineFromList();
	void ShowLookup();
	void HideLookup();
	void ToggleLookup();

	bool m_initialised;
	EventHandlerT m_notify;
	bool m_eventsEnabled;
	HWND m_hwnd;
	HWND m_parent;
	HWND m_hwndLookup;
	int	 m_noOfChars;	// no. of chars displayed excluding current command being typed by user
						// prevents backspacing before start of new line
	CUtils::StringsT m_cmdHistory;
	int m_hix;


	HMENU m_popup;

	enum EventsT
	{
		EventGotFocus		= 0x01,
		EventLostFocus		= 0x02,
		EventSelectionSet	= 0x04,
		EventSelectionClear = 0x08,
		EventClosed			= 0x10,
		EventOutput			= 0x20,
		EventInput			= 0x40,
		EventAsynchOutput   = 0x80
	};
};



