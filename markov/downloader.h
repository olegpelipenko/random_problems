#pragma once

#include <string>
#include <list>

using namespace std;

class Downloader
{
public:
	Downloader()
	{

	}

	void Download(list<wstring>& links)
	{
		m_links = links;
		Run();
	}

private:
	void Run()
	{

	}

	list<wstring> m_links;
};