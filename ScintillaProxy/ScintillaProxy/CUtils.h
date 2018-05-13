#pragma once

#include <string>
#include <vector>
#include <tchar.h>
#include <exception>

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
//	static std::string ToChar(const wchar_t* p);

};

class CException : public std::exception
{
public:

	static CException CreateLastErrorException();
	static CException CreateException(const char* what);
	virtual ~CException();
	CException(const CException& other);
	CException& operator=(const CException& other);

	virtual const char* what() const;

private:

	CException();
	CException(const char* what);

	std::string m_what;
};