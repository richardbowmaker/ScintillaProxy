#pragma once

#include <RichEdit.h>
#include <tchar.h>
#include <vector>
#include <string>

// Terminal window in which an external processes stdin and stdouty are echoed

#define TW_ADDTEXT WM_USER 

class CGhciTerminal
{
public:

#ifdef _UNICODE
	typedef std::wstring StringT;
	typedef std::vector<StringT> StringsT;
#else
	typedef std::string StringT;
	typedef std::vector<StringT> StringsT;
#endif


	CGhciTerminal();
	~CGhciTerminal();

	bool Initialise(HWND hParent, char* file);
	void Uninitialise();
	void ClearText();
	void SetText(StringT text);
	void AddText(StringT text);
	void AddLine(StringT text);
	void AddTextTS(StringT text); // non-blocking, thread safe
	void NewLine();
	HWND GetHwnd() const;
	HWND GetParentHwnd() const;
	int GetNoOfChars();

	bool RichTextBoxProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam);
	bool ListBoxProc(UINT Msg, WPARAM wParam, LPARAM lParam);
	void ReadAndHandleOutput();

	void WndProcRetHook(int nCode, WPARAM wParam, LPARAM lParam);

private:

	void HandleKeyPress(WPARAM wParam, LPARAM lParam);
	void SendCommand(StringT text);
	void UpdateCommandLine();
	void ShowLookup();
	void HideLookup();
	bool StringStartsWith(StringT str, StringT start);
	StringT StringRemoveAt(StringT str, unsigned int start, unsigned int len = 1);
	StringT ToStringT(char* p);
	std::string ToChar(StringT& s);

	HWND m_hwnd;
	HWND m_parent;
	HWND m_hwndLookup;
	HMODULE m_hdll;
	int m_noOfChars;	// no. of chars displayed excluding current command being typed by user
				        // prevents backspacing before start of new line

	StringT m_cmdLine;
	StringsT m_cmdHistory;
	int m_hix;

//--------------------

	void StartCommand(char* file);
	void PrepAndLaunchRedirectedChild(char* file,
		HANDLE hChildStdOut,
		HANDLE hChildStdIn,
		HANDLE hChildStdErr);

	HANDLE m_hChildProcess;
	HANDLE m_hInputWrite;
	HANDLE m_hOutputRead;
};



