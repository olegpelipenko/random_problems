#ifndef HELPERS_H
#define HELPERS_H

#include "dijkstra.h"
#include "words_graph.h"
#include <string>
#include <set>
#include <fstream>

#include <locale>
#include <codecvt>

typedef std::wstring Word;
typedef std::set<Word> WordsList;

template <typename W>
std::vector<W> FindShortestPath(const WordsGraph<W>& graph, const W& from, const W& to)
{
	auto pathIndexes = Dijkstra(graph, graph.GetNodeIndex(from), graph.GetNodeIndex(to));
	return graph.ConvertIndexesToWords(pathIndexes);
}

template<typename W>
void ReadWordsFromFile(const std::string& filePath, const size_t wordLength, WordsGraph<W>& graph)
{
	std::wifstream file(filePath);
	W tempStr;
	WordsList words;
	auto& facet = std::use_facet<std::ctype<typename W::value_type>>(std::locale());

	while (std::getline(file, tempStr))
	{

		// Remove escape symbols
		while (tempStr.back() == '\r' || tempStr.back() == '\n' || tempStr.back() == ' ')
			tempStr.pop_back();

		// We are working only with words of the same word length
		if (tempStr.size() == wordLength)
		{
			// Convert to lower case
			facet.tolower(&tempStr[0], &tempStr[0] + tempStr.size());
			words.insert(tempStr);
		}
	}
	
	graph.SetNodes(words);
}

// Counts only number of different letters in a words, it is not an 'edit distance'
template <typename W>
bool IsDistanceMeetsExpectations(const W& w1, const W& w2, size_t expectedDistance)
{
	if (w1.length() != w2.length())
		throw std::runtime_error(std::string("Words are of different length").c_str());

	size_t currentDistance = 0;
	for (size_t i = 0; i < w1.length(); ++i)
	{
		if (w1[i] != w2[i])
			currentDistance += 1;

		if (currentDistance > expectedDistance)
			return false;
	}

	return currentDistance == expectedDistance;
}

// Create edges in graph. 
// Will be created edges only of the same word length and "edit distance" equals 1
template <class W>
void CreateEdges(WordsGraph<W>& graph)
{
	for (auto w1 = graph.Begin(); w1 != graph.End(); w1++)
	{
		for (auto w2 = graph.Begin(); w2 != graph.End(); w2++)
		{
			if (w2 == w1)
				continue;

			// Graph should contain only words with distance equals 1
			bool result = IsDistanceMeetsExpectations(*w1, *w2, 1);
			if (result)
			{
				graph.AddEdge(*w1, *w2);
				graph.AddEdge(*w2, *w1);
			}
		}
	}
}

// Find shortest path using Dijkstra algorithm
template<typename W>
std::vector<W> FindPath(const std::string& filePath, const W& from, const W& to)
{
	WordsGraph<W> graph;
	ReadWordsFromFile(filePath, from.length(), graph);
	CreateEdges(graph);
	return FindShortestPath(graph, from, to);
}

#endif // HELPERS_H
