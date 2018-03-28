#pragma once

#include <RichEdit.h>
#include <tchar.h>
#include "CUtils.h"


// Terminal window in which an external processes stdin and stdouty are echoed

#define TW_ADDTEXT WM_USER 

class CGhciManager;

class CGhciTerminal
{
public:

	typedef void* (*EventHandlerT)(HWND, int, const char*);

	CGhciTerminal();
	~CGhciTerminal();

	bool Initialise(HWND hParent, char* options, char* file);
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
	int GetNoOfChars();
	void SendCommand(CUtils::StringT text);
	void SendCommand(char* cmd);
	bool IsTextSelected();
	bool HasFocus();
	void SetFocus();
	int GetTextLength();
	int GetText(char* buff, int size);
	void Clear();
	void WndProcRetHook(LPCWPRETSTRUCT pData);
	void GetMsgProc(LPMSG pData);
	bool RichTextBoxProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam);
	bool ListBoxProc(UINT Msg, WPARAM wParam, LPARAM lParam);
	void ReadAndHandleOutput();

private:

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

	//--------------------

	void StartCommand(char* options, char* file);
	void PrepAndLaunchRedirectedChild(
		char* options, char* file,
		HANDLE hChildStdOut,
		HANDLE hChildStdIn,
		HANDLE hChildStdErr);

	HANDLE m_hChildProcess;
	HANDLE m_hInputWrite;
	HANDLE m_hOutputRead;
	volatile bool m_threadStopped;

	HMENU m_popup;

	enum EventsT
	{
		EventGotFocus = 1,
		EventLostFocus = 2,
		EventSelectionSet = 3,
		EventSelectionClear = 4,
		EventClosed = 5,
		EventOutput = 6,
		EventInput = 7
	};
};



