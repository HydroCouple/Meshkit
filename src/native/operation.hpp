// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "meshkit/mesh.hpp"
#include <chrono>
#include <stdexcept>
#include <cmath>
#include <limits>
#include <initializer_list>
#include <utility>

namespace meshkit::native_detail
{
/**
 * @brief Exception object thrown by fail() to abort a native operation.
 *
 * It does not derive from std::exception; the public entry points catch it and copy it into
 * their report (see setError()).
 */
struct Stop {
    RunStatus status; ///< Report status such as Failed, Infeasible, ResourceLimit or Cancelled.
    /// Short machine-readable failure code such as `work_limit`.
    std::string code, message;
    /** @var message
     *  @brief Human-readable English explanation. */
};
/**
 * @brief Abort the current native operation by throwing a Stop.
 * @param[in] code Machine-readable failure code, moved into the exception.
 * @param[in] message English explanation, moved into the exception.
 * @param[in] status Report status; Infeasible by default.
 */
[[noreturn]] inline void fail(std::string code, std::string message,
                              RunStatus status = RunStatus::Infeasible)
{
    throw Stop{status, std::move(code), std::move(message)};
}
/**
 * @brief Work-unit budget, workspace-estimate gate and cancellation poll for one operation.
 *
 * Holds references to the caller's limits and callback, which must outlive it. Not
 * thread-safe: one operation thread updates it.
 */
struct Work {
    const Limits &limits; ///< Borrowed limits; maxWorkspaceBytes gates memory().
    const std::function<bool()> &cancelled; ///< Borrowed cancellation poll; may be empty.
    /// Work units consumed so far by tick().
    std::uint64_t used = 0, maximum = 0;
    /** @var maximum
     *  @brief Work-unit allowance: Limits::maxWork, or 20 billion when that is 0. */
    /**
     * @brief Bind the limits and poll, set the allowance, and poll cancellation once.
     * @param[in] l Limits, borrowed for the object's lifetime.
     * @param[in] c Cancellation callback, borrowed for the object's lifetime; may be empty.
     * @throws Stop `cancelled` (status Cancelled) when the poll already returns true.
     */
    Work(const Limits &l, const std::function<bool()> &c) : limits(l), cancelled(c)
    {
        maximum = l.maxWork ? l.maxWork : 20000000000ull;
        check();
    }
    /**
     * @brief Poll the cancellation callback.
     * @throws Stop `cancelled` (status Cancelled) when the callback is set and returns true.
     */
    void check() const
    {
        if (cancelled && cancelled())
            fail("cancelled", "Cancelled.", RunStatus::Cancelled);
    }
    /**
     * @brief Charge work units; cancellation is polled whenever the updated total is an exact
     * multiple of 4096.
     * @param[in] count Units to add.
     * @throws Stop `work_limit` (status ResourceLimit), leaving used unchanged, when the charge
     * would exceed maximum; Stop `cancelled` from check().
     */
    void tick(std::uint64_t count = 1)
    {
        if (count > maximum - used)
            fail("work_limit", "Operation work limit exceeded.", RunStatus::ResourceLimit);
        used += count;
        if ((used & 4095) == 0)
            check();
    }
    /**
     * @brief Gate a workspace estimate given as (count, bytes per item) terms.
     * @param[in] terms Pairs whose products are summed; nothing is allocated or recorded.
     * @throws Stop `workspace_limit` (status ResourceLimit) when the sum overflows 64 bits or
     * exceeds Limits::maxWorkspaceBytes.
     */
    void memoryFor(std::initializer_list<std::pair<std::uint64_t, std::uint64_t>> terms) const
    {
        std::uint64_t total = 0;
        for (auto term : terms) {
            if (term.second &&
                term.first > (std::numeric_limits<std::uint64_t>::max() - total) / term.second)
                fail("workspace_limit", "Workspace estimate overflows.", RunStatus::ResourceLimit);
            total += term.first * term.second;
        }
        memory(total);
    }
    /**
     * @brief Gate one workspace estimate; nothing is allocated or recorded.
     * @param[in] estimate Estimated bytes.
     * @throws Stop `workspace_limit` (status ResourceLimit) when estimate exceeds
     * Limits::maxWorkspaceBytes.
     */
    void memory(std::uint64_t estimate) const
    {
        if (estimate > limits.maxWorkspaceBytes)
            fail("workspace_limit", "Estimated workspace exceeds the configured limit.",
                 RunStatus::ResourceLimit);
    }
};
/**
 * @brief Order-independent 64-bit key of an undirected edge.
 * @param[in] a One endpoint index.
 * @param[in] b The other endpoint index.
 * @return `min(a, b) << 32 | max(a, b)`.
 */
inline std::uint64_t edgeKey(Index a, Index b)
{
    return std::uint64_t(std::min(a, b)) << 32 | std::max(a, b);
}
/**
 * @brief Axis-aligned bounding box of a point array.
 * @param[in] p Borrowed array of at least n points (p[0] is always read).
 * @param[in] n Point count, >= 1.
 * @return Normalized box from the minimum to the maximum coordinates; no finite check.
 */
inline Box2 box(const Point2 *p, int n)
{
    double x0 = p[0].x(), x1 = x0, y0 = p[0].y(), y1 = y0;
    for (int k = 1; k < n; ++k) {
        x0 = std::min(x0, p[k].x());
        x1 = std::max(x1, p[k].x());
        y0 = std::min(y0, p[k].y());
        y1 = std::max(y1, p[k].y());
    }
    return {{x0, y0}, {x1, y1}};
}
/**
 * @brief Closed overlap test of two normalized boxes; touching edges or corners count.
 * @param[in] a First box (nonnegative width and height).
 * @param[in] b Second box (nonnegative width and height).
 * @return True when the boxes share at least one point.
 */
inline bool meets(const Box2 &a, const Box2 &b)
{
    return a.left() <= b.right() && a.right() >= b.left() && a.top() <= b.bottom() &&
           a.bottom() >= b.top();
}
/**
 * @brief Test whether both coordinates are finite.
 * @param[in] p Point to test.
 * @return False when either coordinate is NaN or infinite.
 */
inline bool finite(Point2 p)
{
    return std::isfinite(p.x()) && std::isfinite(p.y());
}
/**
 * @brief Check that a byte string is well-formed UTF-8.
 * @param[in] s Bytes to check; may be empty or contain NUL.
 * @return False for an invalid lead or continuation byte, a truncated sequence, an overlong
 * encoding, a surrogate (U+D800 to U+DFFF) or a code point above U+10FFFF.
 */
inline bool utf8(const std::string &s)
{
    for (std::size_t i = 0; i < s.size();) {
        const auto first = static_cast<unsigned char>(s[i++]);
        if (first < 128)
            continue;
        int rest = 0;
        std::uint32_t cp = 0, minimum = 0;
        if (first >= 0xc2 && first <= 0xdf) {
            rest = 1;
            cp = first & 31;
            minimum = 0x80;
        } else if (first >= 0xe0 && first <= 0xef) {
            rest = 2;
            cp = first & 15;
            minimum = 0x800;
        } else if (first >= 0xf0 && first <= 0xf4) {
            rest = 3;
            cp = first & 7;
            minimum = 0x10000;
        } else
            return false;
        if (i + rest > s.size())
            return false;
        while (rest--) {
            const auto c = static_cast<unsigned char>(s[i++]);
            if ((c & 0xc0) != 0x80)
                return false;
            cp = cp << 6 | (c & 63);
        }
        if (cp < minimum || cp > 0x10ffff || (cp >= 0xd800 && cp <= 0xdfff))
            return false;
    }
    return true;
}
/**
 * @brief Elapsed steady-clock time.
 * @param[in] start Earlier steady_clock reading.
 * @return Seconds from start to now.
 */
inline double seconds(std::chrono::steady_clock::time_point start)
{
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
}
}
