#include "model.h"

// Generates random between a and b
int GenerateRandom(int a, int b)
{
	std::random_device r;
	std::default_random_engine e1(r());
	std::uniform_int_distribution<int> uniform_dist(a, b);
	return uniform_dist(e1);
}

MarkovChainModel::MarkovChainModel(uint32_t order) : m_order(order)
{
}

bool MarkovChainModel::CreateModel(vector<wchar_t>& text)
{
	vector<wstring> words;
	boost::split(words, text, boost::algorithm::is_space(), boost::token_compress_on);
	
	Check(m_order > words.size(), "Text is too small");

	Sentence sentence(words, m_order);
	auto currentWord = words.begin();
	std::advance(currentWord, m_order);
	wstring key;

	while (currentWord != words.end())
	{
		key = sentence.GetKey();

		auto currentValue = m_chain.find(key);
		if (m_chain.end() != currentValue)
			currentValue->second.push_front(*currentWord);
		else
			m_chain.emplace(key, forward_list<wstring>{ *currentWord });

		sentence.InsertWord(*currentWord);
		currentWord++;
	}

	return true;
}

void MarkovChainModel::Merge(const MarkovChainModel& otherModel)
{
	if (this == &otherModel)
		return;

	for (auto keyValue : otherModel.m_chain)
	{
		auto current = m_chain.find(keyValue.first);
		if (m_chain.end() != current)
			current->second.splice_after(current->second.before_begin(), keyValue.second);
		else
			m_chain.emplace(keyValue.first, keyValue.second);
	}
}

bool MarkovChainModel::Load(const wstring& filePath, bool fullLoad)
{
	m_chain.clear();

	wifstream stream(filePath);
	wstring line;
	getline(stream, line);
	uint32_t newOrder = stol(line);
	if (newOrder != m_order)
		return false;

	while (getline(stream, line))
	{
		vector<wstring> words;
		boost::split(words, line, boost::is_any_of(" "));
		auto currentWord = words.begin();

		wstring key;
		for (uint32_t i = 1; i <= m_order; ++i, ++currentWord)
		{
			if (currentWord == words.end())
				return false;

			key += *currentWord;
			if (i != m_order)
				key += L" ";
		}
		m_chain.emplace(key, forward_list<wstring>(currentWord, words.end()));
	}
	
	return true;
}

void MarkovChainModel::Save(const string& filePath) const
{
	wofstream stream;
	stream.open(filePath);
	Check(!stream.is_open(), (boost::format("Can't open file %1%") % filePath).str());

	stream << m_order << endl;
	for (auto& value : m_chain)
	{
		stream << value.first << " ";
		stream << boost::join(value.second, " ");
		stream << endl;
	}
}

MarkovChainView::MarkovChainView(string filePath, uint32_t order) 
	: MarkovChainModel(order), 
	  m_stream(filePath)
{
	Check(!m_stream.is_open(), (boost::format("Can't open file: %1%") % filePath).str());

	wstring line;
	getline(m_stream, line);
	Check(m_order != stol(line), "Order from file is not compatible, order wrong");

	struct stat stat_buf;
	Check(stat(filePath.c_str(), &stat_buf) == -1, (boost::format("Can't get file size: %1%") % filePath).str());
	m_size = stat_buf.st_size;
}

bool MarkovChainView::GetNextWord(const wstring& key, wstring& word)
{
	Chain::const_iterator it = m_chain.find(key);
	if (it != m_chain.end())
	{
		word = GetRandomWord((MarkovChainModel::ChainValue)*it);
		return true;
	}
	else
	{
		ChainValue result;
		if (SearchKey(key, 2, m_size, result))
		{
			word = GetRandomWord(result);
			m_chain.insert(result);
			return true;
		}
	}
	
	return false;
}
	
wstring MarkovChainView::GetRandomWord(ChainValue& value) const
{
	const auto& list = value.second;
	size_t size = distance(list.begin(), list.end());
	size_t pos = size != 0 ? GenerateRandom(0, size - 1) : 0;
	auto randomValue = list.begin();
	advance(randomValue, pos);
	return *randomValue;
}

bool MarkovChainView::SearchKey(const wstring& key, uint32_t l, uint32_t r, ChainValue& result)
{
	if (r >= l)
	{
		int mid = l + (r - l) / 2;

		wstring line;
		GetLine(mid, line);
		SplitLine(line, result);

		if (result.first == key)
			return true;

		if (result.first > key)
			return SearchKey(key, l, mid - 1, result);

		return SearchKey(key, mid + 1, r, result);
	}

	return false;
}

void MarkovChainView::SplitLine(const wstring& line, ChainValue& result)
{
	vector<wstring> words;
	boost::split(words, line, boost::is_any_of(" "));
	auto currentWord = words.begin();

	wstring key;
	for (uint32_t i = 1; i <= m_order; ++i, ++currentWord)
	{
		Check(currentWord == words.end(), "Bad model");

		key += *currentWord;
		if (i != m_order)
			key += L" ";
	}
	
	result.first = key;
	result.second = forward_list<wstring>(currentWord, words.end());
}

void MarkovChainView::GetLine(uint32_t pos, wstring& line)
{
	m_stream.seekg(std::ios::beg);
	m_stream.seekg(pos);
	m_stream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	getline(m_stream, line);
}