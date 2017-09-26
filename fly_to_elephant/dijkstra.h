#ifndef DIJKSTRA_H
#define DIJKSTRA_H

#include <algorithm>

// Restore path from array of indexes
template<typename IndexType>
void RestorePath(std::vector<IndexType>& p, IndexType from, IndexType to)
{
	std::vector<IndexType> path;
	for (int v = to; v != from; v = p[v])
		path.push_back(v);
	path.push_back(from);
	std::reverse(path.begin(), path.end());
	p.swap(path);
}

template <typename Graph, typename IndexType = typename Graph::NodeIndex, typename EdgeType = typename Graph::Edges>
std::vector<IndexType> Dijkstra(const Graph& graph, IndexType start, IndexType end)
{
	const IndexType Infinity = std::numeric_limits<IndexType>::max();
	const unsigned int EdgeWeight = 1;

	std::vector<IndexType> d(graph.GetSize(), Infinity), path(graph.GetSize());
	d[start] = 0;
	std::set<std::pair<IndexType, IndexType> > q;
	q.insert(std::make_pair(d[start], start));

	while (!q.empty())
	{
		IndexType v = q.begin()->second;
		q.erase(q.begin());

		EdgeType edges;
		graph.GetEdges(v, edges);

		for (auto edge = edges.begin(); edge != edges.end(); edge++)
		{
			IndexType to = *edge;
			if (d[v] + EdgeWeight < d[to])
			{
				q.erase(std::make_pair(d[to], to));
				d[to] = d[v] + EdgeWeight;
				path[to] = v;
				q.insert(std::make_pair(d[to], to));
			}
		}
	}
	
	// End node is unreachable
	if (d[end] == Infinity)
	{
		path.clear();
		return path;
	}
	else
	{
		RestorePath(path, start, end);
		return path;
	}
}

#endif // DIJKSTRA_H
