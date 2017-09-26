#include <iostream>

#include "gtest/gtest.h"

#include "helpers.h"
#include "words_graph.h"

TEST(DistanceCalculationTest, CheckDistanceBaseCases)
{
	std::string word1 = "word1", word2 = "word2";
	EXPECT_TRUE(IsDistanceMeetsExpectations(word1, word2, 1));
	EXPECT_FALSE(IsDistanceMeetsExpectations(word1, word2, 2));

	std::string word3 = "long_word";
	ASSERT_THROW(IsDistanceMeetsExpectations(word2, word3, 0), std::exception);
}

TEST(ReadWordsFromFileTest, IsReadSucceededOnSmallDict)
{
	std::string filePath = "./data/small_dict_en.txt";
	size_t wordLength = 3;

	{
		WordsGraph<Word> graph;
		ReadWordsFromFile(filePath, wordLength, graph);
		EXPECT_EQ(graph.GetSize(), 9);
	}

	{
		WordsGraph<Word> graph;
		wordLength = 4;
		ReadWordsFromFile(filePath, wordLength, graph);
		EXPECT_EQ(graph.GetSize(), 0);
	}
}

TEST(ReadWordsFromFileTest, IsReadSucceededOnBigDict)
{
	std::string filePath = "./data/google-10000-english.txt";
	WordsGraph<Word> graph;
	size_t wordLength = 6;

	ReadWordsFromFile(filePath, wordLength, graph);
	EXPECT_EQ(graph.GetSize(), 1509);
}

TEST(GraphCorrectness, BaseCasesWithNodesAndEdges)
{
	typedef WordsGraph<Word> Graph;
	Graph graph;
	std::set<Word> words = {L"aa", L"bb", L"cc"};
	graph.SetNodes(words);
	EXPECT_EQ(graph.GetSize(), 3);
	
	// Create edge
	graph.AddEdge(L"aa", L"bb");
	Graph::Edges edges;
	graph.GetEdges(L"aa", edges);
	EXPECT_EQ(edges.size(), 1);
	EXPECT_EQ(graph.GetNodeValue(*edges.begin()), L"bb");
	
	// Append existing: should be the same size
	graph.AddEdge(L"aa", L"bb");
	graph.GetEdges(L"aa", edges);
	EXPECT_EQ(edges.size(), 1);

	// Append edge
	graph.AddEdge(L"aa", L"cc");
	graph.GetEdges(L"aa", edges);
	EXPECT_EQ(edges.size(), 2);
	EXPECT_TRUE(edges.find(graph.GetNodeIndex(L"bb")) != edges.end());
	EXPECT_TRUE(edges.find(graph.GetNodeIndex(L"cc")) != edges.end());

	// Add not existing edge
	ASSERT_THROW(graph.AddEdge(L"dd", L"cc"), std::exception);
}

TEST(CreateEdge, EdgesCreated)
{
	typedef WordsGraph<Word> Graph;
	Graph graph;
	std::set<Word> words({ L"bat", L"rat", L"god", L"fat", L"rod", L"rad", L"bad", L"ooo" });
	graph.SetNodes(words);
	CreateEdges(graph);
	
	Graph::Edges edges;
	graph.GetEdges(L"bat", edges);
	EXPECT_EQ(edges.size(), 3);

	graph.GetEdges(L"ooo", edges);
	EXPECT_EQ(edges.size(), 0);
}

TEST(FindShortestPathTest, FindInSmallDictionary)
{
	typedef WordsGraph<Word> Graph;
	Graph graph;
	graph.SetNodes({ L"bat", L"rat", L"god", L"fat", L"rod", L"rad", L"bad" });
	CreateEdges(graph);
	auto result = FindShortestPath(graph, Word(L"god"), Word(L"fat"));
	decltype(result) expectedResult = { L"god", L"rod", L"rad", L"rat", L"fat" };
	EXPECT_EQ(result, expectedResult);
}

int main(int argc, char** argv)
{
	std::locale::global(std::locale(""));
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
