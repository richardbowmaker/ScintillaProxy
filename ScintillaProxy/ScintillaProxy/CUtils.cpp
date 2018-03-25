#include "stdafx.h"
#include "CUtils.h"

#include <algorithm>
#include <string>


bool CUtils::StringStartsWith(StringT str, StringT start)
{
	if (str.size() == 0 || start.size() == 0) return false;
	StringT s = str.substr(0, start.size());
	return s == start;
}

CUtils::StringT CUtils::StringRemoveAt(StringT str, unsigned int start, unsigned int len)
{
	StringT s1, s2;
	if (start > 0 && start < str.size()) s1 = str.substr(0, start);
	if (start + len > 0 && start + len < str.size()) s2 = str.substr(start + len);
	return s1 + s2;
}

CUtils::StringT CUtils::ToStringT(char* p)
{
#ifdef _UNICODE
	wchar_t wBuff[1000];
	int n = MultiByteToWideChar(CP_ACP, MB_COMPOSITE, p, (int)strlen(p), wBuff, sizeof(wBuff));
	return StringT(wBuff, n);
#else
	return StringT(p);
#endif
}

CUtils::StringT CUtils::ToStringT(wchar_t* p)
{
#ifdef _UNICODE
	return StringT(p);
#else
	char buff[1000];
	int n = WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, p, (int)wcslen(p), buff, sizeof(buff), NULL, NULL);
	return std::string(buff, n);
#endif
}

std::string CUtils::ToChar(StringT& s)
{
#ifdef _UNICODE
	char buff[1000];
	int n = WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, s.c_str(), (int)s.size(), buff, sizeof(buff), NULL, NULL);
	return std::string(buff, n);
#else
	return s;
#endif
}

