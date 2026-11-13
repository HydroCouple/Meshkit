// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <algorithm>
#include <cstdint>
#include <vector>

namespace meshkit
{
// One live entry per vertex. Updating a neighbour cannot accumulate stale heap
// entries. Tile order retains locality; lower importance/density comes first,
// with vertex id as a deterministic final tie. Thread confined, no callbacks.
/**
 * @brief Indexed binary min-heap holding at most one entry per vertex for resolution-aware
 * coarsening.
 *
 * Entries are ordered by tile, then importance, then density, then vertex id, all ascending.
 */
class CoarseningQueue
{
public:
    /** @brief One vertex's removal priority. */
    struct Entry {
        /// Removal importance, lower first (see resolutionRemovalImportance()).
        float importance = 0, density = 0;
        /** @var density
         *  @brief Tie-break after importance, lower first; the caller passes the local
         *  actual/target size ratio. */
        /// Vertex id in [0, vertices) given to the constructor; not range-checked.
        int vertex = -1;
        /// Spatial tile key, compared first so that one tile drains before the next.
        std::uint32_t tile = 0;
    };
    /**
     * @brief Create an empty queue for vertex ids in [0, vertices).
     * @param[in] vertices Size of the vertex-to-slot index (one int per vertex).
     * @param[in] maximumEntries Heap capacity to reserve, capped at vertices; 0 reserves none.
     */
    explicit CoarseningQueue(std::size_t vertices, std::size_t maximumEntries = 0)
        : positions(vertices, -1)
    {
        heap.reserve(std::min(vertices, maximumEntries));
    }
    /** @brief True when no entry is queued. */
    bool empty() const { return heap.empty(); }
    /** @brief Number of queued entries (at most one per vertex). */
    std::size_t size() const { return heap.size(); }
    /** @brief Bytes of allocated heap and index storage (capacity, not size). */
    std::size_t capacityBytes() const
    {
        return heap.capacity() * sizeof(Entry) + positions.capacity() * sizeof(int);
    }
    /**
     * @brief Insert the vertex's entry, or replace its existing entry, and restore heap order.
     * @param[in] entry New priority; entry.vertex must lie in [0, vertices) (unchecked).
     */
    void update(Entry entry)
    {
        int at = positions[entry.vertex];
        if (at < 0) {
            at = int(heap.size());
            heap.push_back(entry);
            positions[entry.vertex] = at;
            up(at);
        } else {
            heap[at] = entry;
            at = up(at);
            down(at);
        }
    }
    /**
     * @brief Remove and return the first entry in queue order.
     * @return The removed entry; the queue must not be empty (unchecked).
     */
    Entry pop()
    {
        const auto result = heap.front();
        positions[result.vertex] = -1;
        if (heap.size() == 1) {
            heap.pop_back();
            return result;
        }
        heap.front() = heap.back();
        heap.pop_back();
        positions[heap.front().vertex] = 0;
        down(0);
        return result;
    }

private:
    std::vector<Entry> heap;
    std::vector<int> positions;
    static bool before(const Entry &a, const Entry &b)
    {
        if (a.tile != b.tile)
            return a.tile < b.tile;
        if (a.importance != b.importance)
            return a.importance < b.importance;
        if (a.density != b.density)
            return a.density < b.density;
        return a.vertex < b.vertex;
    }
    void exchange(int a, int b)
    {
        std::swap(heap[a], heap[b]);
        positions[heap[a].vertex] = a;
        positions[heap[b].vertex] = b;
    }
    int up(int at)
    {
        while (at > 0) {
            const int parent = (at - 1) / 2;
            if (!before(heap[at], heap[parent]))
                break;
            exchange(at, parent);
            at = parent;
        }
        return at;
    }
    void down(int at)
    {
        for (;;) {
            const int left = at * 2 + 1;
            if (left >= int(heap.size()))
                return;
            const int right = left + 1;
            const int next =
                right < int(heap.size()) && before(heap[right], heap[left]) ? right : left;
            if (!before(heap[next], heap[at]))
                return;
            exchange(at, next);
            at = next;
        }
    }
};
}
