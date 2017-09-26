#ifndef COMMON_H
#define COMMON_H

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <iterator>
#include <algorithm>
#include <locale>
#include <codecvt>

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

using namespace std;
const wstring CurlPath = L"curl.exe";
const uint32_t BufferSize = 1024 * 100; // 100 kb

#define Check(eval, message) if (eval) throw std::runtime_error(message)

inline bool IsFileExists(const std::wstring& name) 
{
	ifstream stream(name);
	return stream.is_open();
}

#endif // COMMON_H