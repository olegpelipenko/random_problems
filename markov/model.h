#ifndef MODEL_H
#define MODEL_H

#include "common.h"
#include <map>
#include <forward_list>
#include <deque>
#include <random>

class Sentence
{
public:
	Sentence(const wstring& str, uint32_t size)
	{
		boost::split(m_deque, str, boost::is_any_of(" "));
	}

	Sentence(vector<wstring> words, uint32_t size)
	{
		copy_n(words.begin(), size, back_inserter(m_deque));
	}

	void InsertWord(const wstring& word)
	{
		m_deque.pop_front();
		m_deque.push_back(word);
	}

	wstring GetKey()
	{
		return boost::join(m_deque, L" ");
	}

private:
	deque<wstring> m_deque;
};

class MarkovChainModel
{
public:
	MarkovChainModel(uint32_t order);

	bool CreateModel(vector<wchar_t>& text);
	void Merge(const MarkovChainModel& otherModel);
	bool Load(const wstring& filePath, bool fullLoad = false);
	void Save(const string& filePath) const;

	bool operator== (const MarkovChainModel& model) const
	{
		return m_order == model.m_order && m_chain == model.m_chain;
	}

	size_t GetSize() const
	{
		return m_chain.size();
	}

protected:
	typedef map<wstring, forward_list<wstring>> Chain;
	typedef pair<wstring, forward_list<wstring>> ChainValue;
	
	Chain m_chain;
	uint32_t m_order;
};

class MarkovChainView : protected MarkovChainModel
{
public:
	MarkovChainView(string filePath, uint32_t order);
	bool GetNextWord(const wstring& key, wstring& word);
	
private:
	wstring GetRandomWord(ChainValue& value) const;
	bool SearchKey(const wstring& key, uint32_t l, uint32_t r, ChainValue& result);
	void GetLine(uint32_t pos, wstring& line);
	void SplitLine(const wstring& line, ChainValue& result);
	
	wfstream m_stream;
	uint32_t m_size;
};



#endif // MODEL_H