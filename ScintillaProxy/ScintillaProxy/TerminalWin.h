#pragma once

#include <RichEdit.h>
#include <tchar.h>

// Terminal window in which an external processes stdin and stdouty are echoed

#define TW_ADDTEXT WM_USER 

class CTerminalWin
{
public:

	CTerminalWin();
	~CTerminalWin();

	bool Initialise(HWND hParent, bool echoInput = false);
	void Uninitialise();
	void ClearText();
	void SetText(TCHAR* pText);
	void AddText(TCHAR* pText);
	void AddLine(TCHAR* pText);
	void NewLine();
	HWND GetHwnd();
	void WinProc(UINT Msg, WPARAM wParam, LPARAM lParam);
	void HandleKeyPress(WPARAM wParam, LPARAM lParam);
	void AddTextTS(TCHAR* pText); // non-blocking, thread safe
	void SendCommand(TCHAR* pText);
	void ShowCommandLine();

	HWND m_hwnd;
	HWND m_parent;
	HMODULE m_hdll;
	bool m_echoInput;
	int m_outLen;
	int m_lix;
	TCHAR m_line[500];



//--------------------

	void StartCommand();
	void DisplayError(TCHAR *pszAPI);
	void ReadAndHandleOutput();
	void PrepAndLaunchRedirectedChild(HANDLE hChildStdOut,
		HANDLE hChildStdIn,
		HANDLE hChildStdErr);

	HANDLE hChildProcess;
	HANDLE hStdIn; // Handle to parents std input.
	HANDLE hInputWrite;
	HANDLE hOutputRead;
	BOOL bRunThread;
};



