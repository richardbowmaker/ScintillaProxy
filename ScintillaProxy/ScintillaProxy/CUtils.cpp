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

CUtils::StringT CUtils::ToStringT(const char* p)
{
#ifdef _UNICODE
	wchar_t wBuff[1000];
	int n = MultiByteToWideChar(CP_ACP, MB_COMPOSITE, p, (int)strlen(p), wBuff, sizeof(wBuff));
	return StringT(wBuff, n);
#else
	return StringT(p);
#endif
}

CUtils::StringT CUtils::ToStringT(const wchar_t* p)
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

//-----------------------------------------------------
// CException
//-----------------------------------------------------

CException CException::CreateLastErrorException()
{
	DWORD err = ::GetLastError();
	if (err != 0)
	{
		LPSTR message = nullptr;
		size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&message, 0, NULL);
		std::string s(message, size);
		LocalFree(message);
		CException ex(s.c_str());
		return ex;
	}
	else
	{
		CException ex;
		return ex;
	}
}

CException CException::CreateException(const char* what)
{
	return CException(what);
}

CException::CException()
{
}

CException::CException(const char* what)
{
	m_what = what;
}

CException::~CException()
{
}

CException::CException(const CException& other)
{
	*this = other;
}

CException& CException::operator=(const CException& other)
{
	m_what = other.what();
	return *this;
}

const char* CException::what() const
{
	return m_what.c_str();
}

