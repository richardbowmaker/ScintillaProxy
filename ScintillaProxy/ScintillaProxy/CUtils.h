#pragma once

#include <string>
#include <vector>
#include <tchar.h>

class CUtils
{
public:

#ifdef _UNICODE
	typedef std::wstring StringT;
	typedef std::vector<StringT> StringsT;
#else
	typedef std::string StringT;
	typedef std::vector<StringT> StringsT;
#endif

	static bool StringStartsWith(StringT str, StringT start);
	static StringT StringRemoveAt(StringT str, unsigned int start, unsigned int len = 1);
	static StringT ToStringT(const char* p);
	static StringT ToStringT(const wchar_t* p);
	static std::string ToChar(StringT& s);

};

