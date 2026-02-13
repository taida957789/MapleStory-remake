#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <vector>

namespace ms
{

/**
 * @brief R*-Tree spatial index
 *
 * Based on TRSTree from the original MapleStory client (v1029).
 * Used for spatial queries on footholds (GetCrossCandidate).
 *
 * @tparam KeyT    Coordinate type (e.g. int32_t)
 * @tparam DataT   Data stored in leaf entries (e.g. shared_ptr<StaticFoothold>)
 * @tparam Dim     Number of dimensions (default 2)
 * @tparam MaxE    Maximum entries per node (default 4)
 * @tparam MinE    Minimum entries per node (default 2)
 */
template <typename KeyT, typename DataT,
          std::size_t Dim = 2, std::size_t MaxE = 4, std::size_t MinE = 2>
class TRSTree
{
    static_assert(Dim >= 1, "Dim must be >= 1");
    static_assert(MinE >= 1, "MinE must be >= 1");
    static_assert(MaxE + 1 >= 2 * MinE, "MaxE+1 must be >= 2*MinE for split");

public:
    // ========== Bounding Box ==========

    struct Bounds
    {
        std::array<KeyT, Dim> lo{};
        std::array<KeyT, Dim> hi{};

        [[nodiscard]] auto Intersects(const Bounds& o) const noexcept -> bool
        {
            for (std::size_t d = 0; d < Dim; ++d)
                if (hi[d] < o.lo[d] || lo[d] > o.hi[d])
                    return false;
            return true;
        }

        void Extend(const Bounds& o) noexcept
        {
            for (std::size_t d = 0; d < Dim; ++d)
            {
                if (o.lo[d] < lo[d]) lo[d] = o.lo[d];
                if (o.hi[d] > hi[d]) hi[d] = o.hi[d];
            }
        }

        [[nodiscard]] auto Area() const noexcept -> KeyT
        {
            KeyT a = hi[0] - lo[0];
            for (std::size_t d = 1; d < Dim; ++d)
                a *= (hi[d] - lo[d]);
            return a;
        }

        [[nodiscard]] auto EnlargedArea(const Bounds& o) const noexcept -> KeyT
        {
            KeyT a = std::max(hi[0], o.hi[0]) - std::min(lo[0], o.lo[0]);
            for (std::size_t d = 1; d < Dim; ++d)
                a *= (std::max(hi[d], o.hi[d]) - std::min(lo[d], o.lo[d]));
            return a;
        }
    };

    /// Create 2D bounds (only valid when Dim == 2)
    static auto MakeBounds2D(KeyT x1, KeyT y1, KeyT x2, KeyT y2) -> Bounds
    {
        static_assert(Dim == 2, "MakeBounds2D requires Dim == 2");
        Bounds b;
        b.lo[0] = std::min(x1, x2);
        b.hi[0] = std::max(x1, x2);
        b.lo[1] = std::min(y1, y2);
        b.hi[1] = std::max(y1, y2);
        return b;
    }

    // ========== Lifecycle ==========

    TRSTree() = default;
    ~TRSTree() { Clear(); }

    TRSTree(const TRSTree&) = delete;
    auto operator=(const TRSTree&) -> TRSTree& = delete;

    TRSTree(TRSTree&& o) noexcept : m_pRoot(o.m_pRoot) { o.m_pRoot = nullptr; }
    auto operator=(TRSTree&& o) noexcept -> TRSTree&
    {
        if (this != &o) { Clear(); m_pRoot = o.m_pRoot; o.m_pRoot = nullptr; }
        return *this;
    }

    // ========== Operations ==========

    /// Search: find all entries whose bbox intersects query
    template <typename Container>
    void Search(const Bounds& query, Container& results) const
    {
        if (m_pRoot)
            SearchImpl(m_pRoot, query, results);
    }

    /// Insert a data entry with given bounding box
    void Insert(const Bounds& bbox, const DataT& data)
    {
        Entry e{};
        e.bbox = bbox;
        e.data = data;

        if (!m_pRoot)
        {
            m_pRoot = NewNode(true);
            m_pRoot->entries[0] = std::move(e);
            m_pRoot->nCount = 1;
            return;
        }

        Entry splitEntry{};
        bool didSplit = InsertImpl(m_pRoot, std::move(e), 0, splitEntry);

        if (didSplit)
        {
            auto* newRoot = NewNode(false);
            Entry e0{};
            e0.bbox = ComputeBounds(m_pRoot);
            e0.child = m_pRoot;
            newRoot->entries[0] = std::move(e0);
            newRoot->entries[1] = std::move(splitEntry);
            newRoot->nCount = 2;
            m_pRoot = newRoot;
        }
    }

    /// Remove all entries
    void Clear()
    {
        if (m_pRoot)
        {
            DestroyNode(m_pRoot);
            m_pRoot = nullptr;
        }
    }

    [[nodiscard]] auto IsEmpty() const noexcept -> bool { return m_pRoot == nullptr; }

private:
    // ========== Internal Types ==========

    struct Node;

    struct Entry
    {
        Bounds bbox{};
        Node* child{};   // non-null for internal nodes
        DataT data{};    // valid for leaf nodes
    };

    struct Node
    {
        bool bLeaf{true};
        std::size_t nCount{};
        std::array<Entry, MaxE + 1> entries{};  // +1 for temporary overflow
    };

    Node* m_pRoot{};

    // ========== Node Allocation ==========

    static auto NewNode(bool leaf) -> Node*
    {
        auto* n = new Node();
        n->bLeaf = leaf;
        return n;
    }

    // ========== Search ==========

    template <typename Container>
    static void SearchImpl(const Node* node, const Bounds& query, Container& results)
    {
        for (std::size_t i = 0; i < node->nCount; ++i)
        {
            const auto& e = node->entries[i];
            if (!e.bbox.Intersects(query))
                continue;

            if (node->bLeaf)
                results.push_back(e.data);
            else
                SearchImpl(e.child, query, results);
        }
    }

    // ========== Insert ==========

    /// Recursive insert. Returns true if node was split; splitOut is the new sibling entry.
    auto InsertImpl(Node* node, Entry entry, int depth, Entry& splitOut) -> bool
    {
        if (node->bLeaf)
        {
            // Insert directly into leaf
            node->entries[node->nCount++] = std::move(entry);
        }
        else
        {
            // Choose best child (minimum area enlargement)
            auto idx = ChooseBestChild(node, entry.bbox);
            Entry childSplit{};
            bool childDidSplit = InsertImpl(node->entries[idx].child, std::move(entry),
                                            depth + 1, childSplit);

            // Update child's bbox
            node->entries[idx].bbox = ComputeBounds(node->entries[idx].child);

            if (childDidSplit)
                node->entries[node->nCount++] = std::move(childSplit);
        }

        // Handle overflow
        if (node->nCount > MaxE)
        {
            auto* sibling = NewNode(node->bLeaf);
            QuadraticSplit(node, sibling);
            splitOut.bbox = ComputeBounds(sibling);
            splitOut.child = sibling;
            splitOut.data = DataT{};
            return true;
        }
        return false;
    }

    /// Choose child whose bbox requires minimum area enlargement
    static auto ChooseBestChild(const Node* node, const Bounds& bbox) -> std::size_t
    {
        std::size_t best = 0;
        KeyT bestEnlargement = std::numeric_limits<KeyT>::max();
        KeyT bestArea = std::numeric_limits<KeyT>::max();

        for (std::size_t i = 0; i < node->nCount; ++i)
        {
            const auto& e = node->entries[i];
            KeyT enlarged = e.bbox.EnlargedArea(bbox);
            KeyT enlargement = enlarged - e.bbox.Area();

            if (enlargement < bestEnlargement ||
                (enlargement == bestEnlargement && e.bbox.Area() < bestArea))
            {
                best = i;
                bestEnlargement = enlargement;
                bestArea = e.bbox.Area();
            }
        }
        return best;
    }

    // ========== Quadratic Split (Guttman) ==========

    static void QuadraticSplit(Node* node, Node* sibling)
    {
        const auto total = node->nCount;  // MaxE + 1
        std::array<Entry, MaxE + 1> all;
        for (std::size_t i = 0; i < total; ++i)
            all[i] = std::move(node->entries[i]);

        // Pick seeds: pair with maximum wasted area
        std::size_t seed0 = 0, seed1 = 1;
        KeyT worstWaste = std::numeric_limits<KeyT>::min();

        for (std::size_t i = 0; i < total; ++i)
        {
            for (std::size_t j = i + 1; j < total; ++j)
            {
                KeyT combined = all[i].bbox.EnlargedArea(all[j].bbox);
                KeyT waste = combined - all[i].bbox.Area() - all[j].bbox.Area();
                if (waste > worstWaste)
                {
                    worstWaste = waste;
                    seed0 = i;
                    seed1 = j;
                }
            }
        }

        // Reset both nodes
        node->nCount = 0;
        sibling->nCount = 0;

        // Assign seeds
        std::array<bool, MaxE + 1> assigned{};
        node->entries[node->nCount++] = std::move(all[seed0]);
        assigned[seed0] = true;
        sibling->entries[sibling->nCount++] = std::move(all[seed1]);
        assigned[seed1] = true;

        Bounds bbox0 = node->entries[0].bbox;
        Bounds bbox1 = sibling->entries[0].bbox;

        // Assign remaining entries
        for (std::size_t remaining = total - 2; remaining > 0; --remaining)
        {
            // If one group needs all remaining to satisfy MinE
            if (node->nCount + remaining == MinE)
            {
                for (std::size_t i = 0; i < total; ++i)
                    if (!assigned[i])
                    {
                        bbox0.Extend(all[i].bbox);
                        node->entries[node->nCount++] = std::move(all[i]);
                        assigned[i] = true;
                    }
                break;
            }
            if (sibling->nCount + remaining == MinE)
            {
                for (std::size_t i = 0; i < total; ++i)
                    if (!assigned[i])
                    {
                        bbox1.Extend(all[i].bbox);
                        sibling->entries[sibling->nCount++] = std::move(all[i]);
                        assigned[i] = true;
                    }
                break;
            }

            // PickNext: entry with max preference difference
            std::size_t pick = 0;
            KeyT maxDiff = std::numeric_limits<KeyT>::min();
            for (std::size_t i = 0; i < total; ++i)
            {
                if (assigned[i])
                    continue;
                KeyT d0 = bbox0.EnlargedArea(all[i].bbox) - bbox0.Area();
                KeyT d1 = bbox1.EnlargedArea(all[i].bbox) - bbox1.Area();
                KeyT diff = (d0 > d1) ? (d0 - d1) : (d1 - d0);
                if (diff > maxDiff)
                {
                    maxDiff = diff;
                    pick = i;
                }
            }

            // Assign to group needing less enlargement
            KeyT enlarge0 = bbox0.EnlargedArea(all[pick].bbox) - bbox0.Area();
            KeyT enlarge1 = bbox1.EnlargedArea(all[pick].bbox) - bbox1.Area();

            if (enlarge0 < enlarge1 ||
                (enlarge0 == enlarge1 && bbox0.Area() <= bbox1.Area()))
            {
                bbox0.Extend(all[pick].bbox);
                node->entries[node->nCount++] = std::move(all[pick]);
            }
            else
            {
                bbox1.Extend(all[pick].bbox);
                sibling->entries[sibling->nCount++] = std::move(all[pick]);
            }
            assigned[pick] = true;
        }
    }

    // ========== Utilities ==========

    static auto ComputeBounds(const Node* node) -> Bounds
    {
        if (node->nCount == 0)
            return {};
        Bounds b = node->entries[0].bbox;
        for (std::size_t i = 1; i < node->nCount; ++i)
            b.Extend(node->entries[i].bbox);
        return b;
    }

    static void DestroyNode(Node* node)
    {
        if (!node->bLeaf)
        {
            for (std::size_t i = 0; i < node->nCount; ++i)
                if (node->entries[i].child)
                    DestroyNode(node->entries[i].child);
        }
        delete node;
    }
};

} // namespace ms
