// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "meshkit/geometry.hpp"
#include <algorithm>
#include <chrono>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace meshkit::detail
{
// Small internal storage vocabulary: QVector-style containers behind the standard kernel
// specialization (StandardStorage) and the plan model. The Qt specialization (QtStorage)
// supplies QVector/QHash/QSet with the same member names instead. Public jobs own ordinary
// standard C++ arrays. No GUI mirror or per-access conversion of the kernel's
// multi-million-entry buffers.
/**
 * @brief `std::vector<T>` with the QVector-style member names the CDT kernel and plan model use.
 *
 * Lets the same kernel code compile against StandardStorage and against QVector in the Qt
 * storage. Storage, iterator and reference invalidation are those of std::vector, and there
 * is no internal synchronization. size() hides `std::vector::size()` and returns a signed
 * count. The element helpers (first(), last(), takeFirst(), takeLast(), removeLast(),
 * removeAt()) keep std::vector's preconditions: they do not check for an empty vector or an
 * out-of-range index.
 * @tparam T Element type.
 */
template <class T> class Vector : public std::vector<T>
{
    using Base = std::vector<T>;

public:
    using Base::Base;
    /** @brief Empty vector. */
    Vector() = default;
    /**
     * @brief Copy the elements of a std::vector.
     * @param[in] v Source; copied, not retained.
     */
    Vector(const std::vector<T> &v) : Base(v) {};
    /**
     * @brief Take over the buffer of a std::vector without copying elements.
     * @param[in,out] v Source; left empty.
     */
    Vector(std::vector<T> &&v) : Base(std::move(v)) {}
    /** @brief Reference to the first element (front()); the vector must not be empty. */
    decltype(auto) first() { return this->front(); }
    /** @brief Read-only reference to the first element; the vector must not be empty. */
    decltype(auto) first() const { return this->front(); }
    /** @brief Reference to the last element (back()); the vector must not be empty. */
    decltype(auto) last() { return this->back(); }
    /** @brief Read-only reference to the last element; the vector must not be empty. */
    decltype(auto) last() const { return this->back(); }
    /**
     * @brief Read-only pointer to the contiguous elements (data()).
     * @return Pointer invalidated by any reallocation; may be null when the vector is empty.
     */
    const T *constData() const { return this->data(); }
    /**
     * @brief Copy of a contiguous run of elements.
     * @param[in] at Zero-based start index; must be in [0, size()] (not clamped).
     * @param[in] count Number of elements; negative (the default -1) means up to the end. A
     * run past the end is cut at size().
     * @return New vector holding elements [at, min(size(), at + count)), or [at, size())
     * when @p count is negative.
     */
    Vector mid(std::ptrdiff_t at, std::ptrdiff_t count = -1) const
    {
        auto end = count < 0 ? this->size() : std::min(this->size(), at + count);
        return {this->begin() + at, this->begin() + end};
    }
    /**
     * @brief Insert a copy of @p v before the first element; linear in size().
     * @param[in] v Element to copy.
     */
    void prepend(const T &v) { this->insert(this->begin(), v); }
    /**
     * @brief Append copies of all elements of @p v.
     * @param[in] v Source vector; must not be this vector (self-insertion through
     * `std::vector::insert` is undefined behaviour).
     */
    void append(const Vector &v) { this->insert(this->end(), v.begin(), v.end()); }
    /**
     * @brief Append copies of all elements of @p v; same as append(const Vector &).
     * @param[in] v Source vector; must not be this vector.
     * @return Reference to this vector.
     */
    Vector &operator+=(const Vector &v)
    {
        append(v);
        return *this;
    }
    /**
     * @brief Append one element.
     * @param[in] v Element to copy.
     * @return Reference to this vector, for chaining.
     */
    Vector &operator<<(const T &v)
    {
        append(v);
        return *this;
    }

    /** @brief Number of elements as a signed std::ptrdiff_t. */
    std::ptrdiff_t size() const noexcept { return static_cast<std::ptrdiff_t>(Base::size()); }
    /** @brief True when the vector holds no elements. */
    bool isEmpty() const { return this->empty(); }
    /**
     * @brief Append a copy of @p v (push_back).
     * @param[in] v Element to copy.
     */
    void append(const T &v) { this->push_back(v); }
    /**
     * @brief Append @p v by move (push_back).
     * @param[in,out] v Element to move from.
     */
    void append(T &&v) { this->push_back(std::move(v)); }
    /**
     * @brief Remove the last element and return it by move; the vector must not be empty.
     * @return The former last element.
     */
    T takeLast()
    {
        T result = std::move(this->back());
        this->pop_back();
        return result;
    }
    /**
     * @brief Remove the first element and return it by move; linear in size().
     *
     * The vector must not be empty.
     * @return The former first element.
     */
    T takeFirst()
    {
        T result = std::move(this->front());
        this->erase(this->begin());
        return result;
    }
    /** @brief Remove the last element (pop_back); the vector must not be empty. */
    void removeLast() { this->pop_back(); }
    /**
     * @brief Erase one element; the elements after it shift down by one.
     * @param[in] at Zero-based index in [0, size()); not checked.
     */
    void removeAt(int at) { this->erase(this->begin() + at); }
    /**
     * @brief Assign @p v to every existing element; the size is unchanged.
     * @param[in] v Value to copy.
     */
    void fill(const T &v) { std::fill(this->begin(), this->end(), v); }
    /**
     * @brief Replace the contents with @p size copies of @p v (assign()).
     * @param[in] v Value to copy; must not refer to an element of this vector.
     * @param[in] size New element count.
     */
    void fill(const T &v, std::size_t size) { this->assign(size, v); }
    /**
     * @brief Linear search with operator==.
     * @param[in] v Value to look for.
     * @return True when some element compares equal to @p v.
     */
    bool contains(const T &v) const
    {
        return std::find(this->begin(), this->end(), v) != this->end();
    }
};
/**
 * @brief Hash functor of Map, MultiMap and Set; the primary template is `std::hash<K>`.
 * @tparam K Key type; needs a std::hash specialization unless Hash is specialized for it.
 */
template <class K> struct Hash : std::hash<K> {
};
/**
 * @brief Hash for `std::pair` keys that combines `Hash<A>` and `Hash<B>` of the two members.
 *
 * Uses the boost::hash_combine formula `a ^ (b + 0x9e3779b97f4a7c15 + (a << 6) + (a >> 2))`
 * with the constant converted to std::size_t; nested pairs hash recursively. The result
 * depends on member order. The CDT kernel keys its exact duplicate-point detection by the
 * pair of coordinate bit patterns.
 * @tparam A Type of the first member.
 * @tparam B Type of the second member.
 */
template <class A, class B> struct Hash<std::pair<A, B>> {
    /**
     * @brief Hash a pair.
     * @param[in] p Pair to hash.
     * @return Combined hash of `p.first` and `p.second`.
     */
    std::size_t operator()(const std::pair<A, B> &p) const
    {
        const auto a = Hash<A>{}(p.first), b = Hash<B>{}(p.second);
        return a ^ (b + std::size_t(0x9e3779b97f4a7c15ull) + (a << 6) + (a >> 2));
    }
};
/**
 * @brief Hash map with the QHash-style API the CDT kernel uses, over
 * `std::unordered_map<K, V, Hash<K>>`.
 *
 * Unlike std::unordered_map, its iterators dereference to the mapped value, not to a
 * key/value pair (Iterator::key() gives the key), so a range-for visits values. Iteration
 * order is unspecified. Invalidation follows std::unordered_map: an insertion may rehash and
 * invalidate iterators but not references to values, and remove() invalidates only the
 * removed entry. No internal synchronization.
 * @tparam K Key type, hashed by `Hash<K>` and compared with operator==.
 * @tparam V Mapped type.
 */
template <class K, class V> class Map
{
    using Base = std::unordered_map<K, V, Hash<K>>;
    Base data_;

public:
    /**
     * @brief Forward iterator over a Map whose dereference yields the mapped value.
     *
     * Only prefix increment is provided.
     * @tparam Const True for read-only access (ConstIterator), false for MutableIterator.
     */
    template <bool Const> class Iterator
    {
        using It =
            std::conditional_t<Const, typename Base::const_iterator, typename Base::iterator>;
        It it_;

    public:
        /**
         * @brief Wrap an underlying std::unordered_map iterator.
         * @param[in] it Position to wrap.
         */
        explicit Iterator(It it) : it_(it) {}
        /**
         * @brief Mapped value at this position: `V &` for MutableIterator, `const V &` for
         * ConstIterator. Must not be called on an end iterator.
         */
        decltype(auto) value() const { return (it_->second); }
        /** @brief Key at this position; must not be called on an end iterator. */
        const K &key() const { return it_->first; }
        /** @brief Mapped value at this position, same as value(). */
        decltype(auto) operator*() const { return (it_->second); }
        /** @brief Pointer to the mapped value (not to a key/value pair). */
        auto operator->() const { return &it_->second; }
        /**
         * @brief Advance to the next entry.
         * @return Reference to this iterator.
         */
        Iterator &operator++()
        {
            ++it_;
            return *this;
        }
        /**
         * @brief Position equality.
         * @param[in] b Iterator into the same map.
         * @return True when both iterators are at the same position.
         */
        bool operator==(const Iterator &b) const { return it_ == b.it_; }
        /**
         * @brief Position inequality.
         * @param[in] b Iterator into the same map.
         * @return True when the iterators are at different positions.
         */
        bool operator!=(const Iterator &b) const { return it_ != b.it_; }
    };
    using ConstIterator = Iterator<true>;    ///< Read-only iterator; value() is `const V &`.
    using MutableIterator = Iterator<false>; ///< Iterator whose value() is a modifiable `V &`.
    /** @brief Remove all entries. */
    void clear() { data_.clear(); }
    /**
     * @brief Prepare buckets for at least @p n entries (std::unordered_map::reserve).
     * @param[in] n Expected number of entries.
     */
    void reserve(std::size_t n) { data_.reserve(n); }
    /** @brief True when the map has no entries. */
    bool isEmpty() const { return data_.empty(); }
    /** @brief Number of entries, as a signed count. */
    std::ptrdiff_t size() const { return data_.size(); }
    /**
     * @brief Key lookup.
     * @param[in] k Key to look for.
     * @return True when @p k has an entry.
     */
    bool contains(const K &k) const { return data_.find(k) != data_.end(); }
    /**
     * @brief Store @p v under @p k, overwriting any existing value (insert_or_assign).
     * @param[in] k Key.
     * @param[in] v Value to copy.
     * @return Iterator to the entry.
     */
    MutableIterator insert(const K &k, const V &v)
    {
        return MutableIterator(data_.insert_or_assign(k, v).first);
    }
    /**
     * @brief Erase the entry for @p k.
     * @param[in] k Key.
     * @return True when an entry was removed, false when @p k was absent.
     */
    bool remove(const K &k) { return data_.erase(k) != 0; }
    /**
     * @brief Modifiable value for @p k, inserting a value-initialized `V{}` first when absent.
     * @param[in] k Key.
     * @return Reference that stays valid until the entry is removed or the map is cleared or
     * destroyed; rehashing does not move it.
     */
    V &operator[](const K &k) { return data_[k]; }
    /**
     * @brief Copy of the value for @p k, without inserting.
     * @param[in] k Key.
     * @param[in] fallback Returned when @p k is absent; defaults to `V{}`.
     * @return The stored value, or @p fallback.
     */
    V value(const K &k, const V &fallback = V{}) const
    {
        auto it = data_.find(k);
        return it == data_.end() ? fallback : it->second;
    }
    /**
     * @brief Mutable iterator to the entry for @p k.
     * @param[in] k Key.
     * @return Iterator to the entry, or end() when @p k is absent.
     */
    MutableIterator find(const K &k) { return MutableIterator(data_.find(k)); }
    /** @brief Mutable iterator to the first entry (order unspecified). */
    MutableIterator begin() { return MutableIterator(data_.begin()); }
    /** @brief Mutable past-the-end iterator. */
    MutableIterator end() { return MutableIterator(data_.end()); }
    /**
     * @brief Read-only iterator to the entry for @p k.
     * @param[in] k Key.
     * @return Iterator to the entry, or constEnd() when @p k is absent.
     */
    ConstIterator constFind(const K &k) const { return ConstIterator(data_.find(k)); }
    /** @brief Read-only past-the-end iterator, to compare with constFind() results. */
    ConstIterator constEnd() const { return ConstIterator(data_.end()); }
    /** @brief Read-only iterator to the first entry (order unspecified). */
    ConstIterator cbegin() const { return ConstIterator(data_.cbegin()); }
    /** @brief Read-only past-the-end iterator. */
    ConstIterator cend() const { return ConstIterator(data_.cend()); }
    /** @brief Read-only iterator to the first entry; same as cbegin(). */
    ConstIterator begin() const { return cbegin(); }
    /** @brief Read-only past-the-end iterator; same as cend(). */
    ConstIterator end() const { return cend(); }
};
/**
 * @brief Multi-valued hash map with the QMultiHash subset the CDT kernel uses.
 *
 * Each key holds a `Vector<V>` of values, newest first: insert() puts the value at the
 * front and keeps duplicates. insert() and contains() are linear in the number of values
 * under the key. There is no iteration interface and no internal synchronization.
 * @tparam K Key type, hashed by `Hash<K>`.
 * @tparam V Value type; contains() compares values with operator==.
 */
template <class K, class V> class MultiMap
{
    std::unordered_map<K, Vector<V>, Hash<K>> data_;

public:
    /** @brief Remove all keys and values. */
    void clear() { data_.clear(); }
    /**
     * @brief Remove every value stored under @p key.
     * @param[in] key Key; an absent key is ignored.
     */
    void remove(const K &key) { data_.erase(key); }
    /**
     * @brief Add @p value under @p key, ahead of the values already there.
     * @param[in] key Key.
     * @param[in] value Value to copy; duplicates are kept.
     */
    void insert(const K &key, const V &value)
    {
        auto &values = data_[key];
        values.insert(values.begin(), value);
    }
    /**
     * @brief Key/value lookup by linear search of the key's values.
     * @param[in] key Key.
     * @param[in] value Value to look for.
     * @return True when @p key holds a value equal to @p value.
     */
    bool contains(const K &key, const V &value) const
    {
        auto it = data_.find(key);
        return it != data_.end() && it->second.contains(value);
    }
    /**
     * @brief Copy of the values under @p key, most recently inserted first.
     * @param[in] key Key.
     * @return The values, or an empty vector when @p key is absent.
     */
    Vector<V> values(const K &key) const
    {
        auto it = data_.find(key);
        return it == data_.end() ? Vector<V>{} : it->second;
    }
};
/**
 * @brief `std::unordered_set<K, Hash<K>>` with the QSet-style names the CDT kernel uses.
 * @tparam K Element type.
 */
template <class K> class Set : public std::unordered_set<K, Hash<K>>
{
public:
    /**
     * @brief Erase @p k.
     * @param[in] k Element to erase.
     * @return True when @p k was present.
     */
    bool remove(const K &k) { return this->erase(k) != 0; }
    /** @brief True when the set has no elements. */
    bool isEmpty() const { return this->empty(); }
    /**
     * @brief Membership test.
     * @param[in] key Element to look for.
     * @return True when @p key is in the set.
     */
    bool contains(const K &key) const { return this->find(key) != this->end(); }
};
/**
 * @brief Storage policy that instantiates BasicConstrainedDelaunay on standard C++ types.
 *
 * meshkit::ConstrainedDelaunay is `BasicConstrainedDelaunay<StandardStorage>`. The Qt build
 * supplies detail::QtStorage (include/mesh/qtcdtstorage.h) with the same members mapped to
 * QPointF, QRectF, QString and the Qt containers.
 */
struct StandardStorage {
    using Point = Point2;                       ///< Vertex and input point type (XY units).
    using Box = Box2;                           ///< Axis-aligned box type.
    using String = std::string;                 ///< Error message type (errorMsg()).
    using PointInput = std::vector<Point2>;     ///< Input point array type taken by build().
    template <class T> using Vector = detail::Vector<T>;          ///< Sequence container.
    template <class K, class V> using Hash = Map<K, V>;           ///< Hash map.
    template <class K, class V> using MultiHash = MultiMap<K, V>; ///< Multi-valued hash map.
    template <class K> using Set = detail::Set<K>;                ///< Hash set.
    /**
     * @brief Convert a fixed message to String.
     * @param[in] s NUL-terminated text; must not be null. Copied, not retained.
     * @return Copy of @p s.
     */
    static String message(const char *s) { return s; }
    /**
     * @brief Error text for a non-finite input point.
     * @param[in] i Zero-based index of the first input point whose x or y is not finite.
     * @return "cdt: non-finite input point " followed by @p i in decimal.
     */
    static String invalidPointMessage(int i)
    {
        return "cdt: non-finite input point " + std::to_string(i);
    }
};
/**
 * @brief Scope guard that calls a callable when it is destroyed.
 *
 * Create it with scopeExit(). The guard is neither copyable nor movable (the deleted copy
 * constructor also suppresses the move constructor) and cannot be dismissed, so the callable
 * runs exactly once, when the guard leaves scope, on normal exit or during stack unwinding.
 * The destructor is implicitly noexcept: an exception escaping the callable calls
 * std::terminate.
 * @tparam F Callable taking no arguments; its result is discarded.
 */
template <class F> class ScopeExit
{
    F f_;

public:
    /**
     * @brief Store the callable.
     * @param[in] f Callable, moved into the guard; anything it captures by reference must
     * outlive the guard.
     */
    explicit ScopeExit(F f) : f_(std::move(f)) {}
    /** @brief Invoke the stored callable. */
    ~ScopeExit() { f_(); }
    /** @brief Not copyable (and therefore not movable). */
    ScopeExit(const ScopeExit &) = delete;
};
/**
 * @brief Make a ScopeExit guard for @p f.
 *
 * Bind the result to a named local (`auto guard = scopeExit(...)`); C++17 guaranteed copy
 * elision lets the non-movable guard be returned. An unnamed temporary would call @p f at
 * the end of the full-expression.
 * @param[in] f Callable, moved into the guard.
 * @return Guard that calls @p f when destroyed.
 */
template <class F> ScopeExit<F> scopeExit(F f)
{
    return ScopeExit<F>(std::move(f));
}
/**
 * @brief Minimal std::chrono::steady_clock stopwatch with QElapsedTimer-style names.
 *
 * No internal synchronization.
 */
class Timer
{
    std::chrono::steady_clock::time_point start_;

public:
    /** @brief Record the current steady_clock time as the reference; call again to restart. */
    void start() { start_ = std::chrono::steady_clock::now(); }
    /**
     * @brief Time since the last start().
     * @return Elapsed steady_clock time in nanoseconds. Before any start() the reference is
     * the clock's epoch, so the value is meaningless.
     */
    std::int64_t nsecsElapsed() const
    {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
                   std::chrono::steady_clock::now() - start_)
            .count();
    }
};
}
