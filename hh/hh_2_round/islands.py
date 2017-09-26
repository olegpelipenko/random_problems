import collections
import heapq


def initialize(graph, source):
    d = {}
    p = {}
    for node in graph:
        d[node] = float('Inf')
        p[node] = None
    d[source] = 0
    return d, p


def relax(node, neighbour, graph, d, p):
    if d[neighbour] > (d[node] + graph[node][neighbour]):
        d[neighbour] = d[node] + graph[node][neighbour]
        p[neighbour] = node


# Bellman-ford shortest path
def bellman_ford(graph, source):
    d, p = initialize(graph, source)
    for i in range(len(graph)-1):
        for u in graph:
            for v in graph[u]:
                relax(u, v, graph, d, p)

    for u in graph:
        for v in graph[u]:
            assert d[v] <= d[u] + graph[u][v]

    return d, p


class Graph:
    def __init__(self):
        self.nodes = set()
        self.edges = collections.defaultdict(list)
        self.distances = {}

    def add_node(self, value):
        self.nodes.add(value)

    def add_edge(self, from_node, to_node, distance):
        if to_node not in self.edges[from_node]:
            self.edges[from_node].append(to_node)
        if distance < 0:
            distance = 0
        self.distances[(from_node, to_node)] = distance

    def convert(self):
        '''
        graph = {
        '(0,0)': {'(0,1)': 2, '(0,2)':  4},
        '(2,0)': {'(4,3)':  3, '(2,1)':  2, ...},
        '(3,0)': {},
        ...
        }
        '''

        graph = {}
        for n in self.nodes:
            graph[n] = {}
            edge_list = self.edges[n]
            for e in edge_list:

                if e not in graph[n]:
                    graph[n][e] = {}
                graph[n][e] = self.distances[(n, e)]
        return graph


sea = (0, 0)


def get_top(node):
    return node[0] - 1, node[1]


def get_left(node):
    return node[0], node[1] - 1


def get_right(node):
    return node[0], node[1] + 1


def get_bottom(node):
    return node[0] + 1, node[1]


def create_graph(w, h, weights):
    g = Graph()
    g.add_node(sea)
    for r in range(0, h + 1):
        for c in range(0, w + 1):
            from_node = (r, c)
            g.add_node(from_node)

            top = get_top(from_node)
            if top[0] is 0:
                g.add_edge(from_node, sea, 0)
            else:
                g.add_node(top)
                g.add_edge(from_node, top, weights[top[0]][top[1]] - weights[r][c])

            bottom = get_bottom(from_node)
            if bottom[0] > h:
                g.add_edge(from_node, sea, 0)
            else:
                g.add_node(bottom)
                g.add_edge(from_node, bottom, weights[bottom[0]][bottom[1]] - weights[r][c])

            left = get_left(from_node)
            if left[1] is 0:
                g.add_edge(from_node, sea, 0)
            else:
                g.add_node(left)
                g.add_edge(from_node, left, weights[left[0]][left[1]] - weights[r][c])

            right = get_right(from_node)
            if right[1] > w:
                g.add_edge(from_node, sea, 0)
            else:
                g.add_node(right)
                g.add_edge(from_node, right, weights[right[0]][right[1]] - weights[r][c])
    return g


def get_weights(w, h, values):
    weights = [[0 for x in range(w+2)] for y in range(h+2)]
    r = 1
    c = 1
    for row in values:
        for value in row.split():
            weights[r][c] = int(value)
            if c == w:
                r += 1
                c = 0
            c += 1
    return weights


def get_path(p):
        path = []
        cur = p[sea]
        path.append(cur)
        while cur is not None:
            cur = p[cur]
            if cur is not None:
                path.append(cur)
        return path


def get_heights(p, weights):
    hs = []
    for point in p:
        hs.append(weights[point[0]][point[1]])
    return hs


def update_weights(g, node, weights, new_weight):
    weights[node[0]][node[1]] = new_weight

    def apply_to_node(current, g, node, new_weight, weights):
        if current in g[node]:
            g[current][node] = max(0, new_weight - weights[current[0]][current[1]])

    apply_to_node(get_top(node), g, node, new_weight, weights)
    apply_to_node(get_left(node), g, node, new_weight, weights)
    apply_to_node(get_right(node), g, node, new_weight, weights)
    apply_to_node(get_bottom(node), g, node, new_weight, weights)


def fill_heap(w, h, weights):
    heap = []
    for r in range(1, h + 1):
        for c in range (1, w + 1):
            heapq.heappush(heap, (weights[r][c], (r, c)))
    return heap


def water(heights):
    m = max(heights)
    heights.append(m)

    r = len(heights) - 1
    volume = 0
    l = 0
    rm = 0
    lm = 0

    while l < r:
        lm = max(heights[l], lm)
        rm = max(heights[r], rm)
        if lm >= rm:
            volume += rm - heights[r]
            r -= 1
        else:
            volume += lm - heights[l]
            l += 1

    return volume


def find_left_max(hs):
    for i in range(0, len(hs) - 1):
        if hs[i + 1] < hs[i]:
            return hs[i]
    return hs[0]


def get_volume(w, h, values):
    weights = get_weights(w, h, values)
    g = create_graph(w, h, weights).convert()
    heap = fill_heap(w, h, weights)
    volume = 0

    node = heapq.heappop(heap)
    while len(heap) != 0:
        d, p = bellman_ford(g, node[1])
        if d[sea] != 0 and d[sea] != float('Inf'):
            p = get_path(p)
            hs = get_heights(p, weights)
            volume += water(hs)

            for node in p:
                update_weights(g, node, weights, find_left_max(hs))


        node = heapq.heappop(heap)

    return volume


if __name__ == '__main__':
    k = int(raw_input())
    widths = []
    heights = []
    values = []
    for island in range(0, k):
        size = raw_input().split()
        assert len(size) == 2, 'Wrong input: given more than 2 params (height, width)'

        heights.append(int(size[0]))
        widths.append(int(size[1]))
        current_value = []
        for h in range(0, heights[-1]):
            row = raw_input()
            assert len(row.split()) == widths[-1], 'Wrong input: required width ' + str(widths[-1])
            current_value.append(row)
        values.append(current_value)

    for w, h, value in zip(widths, heights, values):
        print get_volume(w, h, value)
