#ifndef WORDS_GRAPH_H
#define WORDS_GRAPH_H

#include <string>
#include <map>
#include <algorithm>
#include <iterator>
#include <set>
#include <sstream>
#include <cstdint>

template <typename W>
class WordsGraph
{
public:	
	typedef size_t				 	 NodeIndex;
	typedef std::vector<W>			 Nodes;
	typedef typename Nodes::iterator NodesIterator;
	typedef std::set<NodeIndex>		 Edges;
	typedef std::vector<W>			 Path;

	NodesIterator Begin()
	{
		return m_nodes.begin();
	}

	NodesIterator End()
	{
		return m_nodes.end();
	}

	size_t GetSize() const
	{
		return m_nodes.size();
	}

	void SetNodes(const std::set<W>& nodes)
	{
		std::copy(nodes.begin(), nodes.end(), std::back_inserter(m_nodes));
	}

	void GetEdges(const W& word, Edges& edges)
	{
		GetEdges(GetNodeIndex(word), edges);
	}

	void GetEdges(NodeIndex index, Edges& edges) const
	{
		edges.clear();
		auto it = m_edges.find(index);
		if (it != m_edges.end())
			edges = it->second;
	}

	void AddEdge(const W& node1, const W& node2)
	{
		NodeIndex index1 = GetNodeIndex(node1), index2 = GetNodeIndex(node2);
		if (index1 != index2)
			m_edges[GetNodeIndex(node1)].insert(GetNodeIndex(node2));
	}

	Path ConvertIndexesToWords(const std::vector<NodeIndex>& p) const
	{
		Path path;
		for (size_t i = 0; i < p.size(); ++i)
			path.push_back(GetNodeValue(p[i]));
		return path;
	}

	W GetNodeValue(NodeIndex index) const
	{
		return m_nodes[index];
	}

	size_t GetNodeIndex(const W& word) const
	{
		auto it = BinaryFind(m_nodes.cbegin(), m_nodes.cend(), word);
		if (it == m_nodes.end())
		{
			std::wcout << "Word '" << word.c_str() << "' not found in dictionary" << std::endl;
			throw std::runtime_error("");
		}
		return (it - m_nodes.begin());
	}

private:

	template<class ForwardIt, class Compare = std::less<std::wstring>>
	ForwardIt BinaryFind(ForwardIt first, ForwardIt last, const W& value, Compare comp = {}) const
	{
		first = std::lower_bound(first, last, value, comp);
		return first != last && !comp(value, *first) ? first : last;
	}
	
	// Every node have list of edges, nodes are represented as indexes in m_nodes array
	std::map<NodeIndex, Edges> m_edges;
	
	// All words are stored in sorted order
	Nodes					   m_nodes;
};

#endif // WORDS_GRAPH_H
