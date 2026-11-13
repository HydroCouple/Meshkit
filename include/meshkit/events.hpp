// SPDX-License-Identifier: Apache-2.0
// Apache-2.0 relicensing authorized by the project rights holder, 2026-10-29.
#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <deque>
#include <limits>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace meshkit
{

/** @brief Severity of an Event. */
enum class Severity { Info, Warning, Error };
/// @var Severity::Info
/// @brief Informational; the default.
/// @var Severity::Warning
/// @brief Warning-level notification.
/// @var Severity::Error
/// @brief Failure; runJob() uses it for a Completed event of a job that did not complete.

/** @brief Kind of an Event. */
enum class EventKind { Stage, Progress, Diagnostic, StageCompleted, Completed };
/// @var EventKind::Stage
/// @brief A stage has started; Event::stage names it.
/// @var EventKind::Progress
/// @brief Progress within a stage. EventChannel::publish() overwrites the newest pending
///        event instead of queueing another when that is a Progress event of the same stage.
/// @var EventKind::Diagnostic
/// @brief Informational diagnostic, e.g. a patch planning retry; the Event default.
/// @var EventKind::StageCompleted
/// @brief A stage has finished.
/// @var EventKind::Completed
/// @brief Final event of a job, carrying its final status; the channel ignores later events.

/** @brief Status of a running or finished operation (see statusName()). */
enum class RunStatus {
    Running,                ///< Still running; carried by intermediate events.
    Complete,               ///< Finished; every checked requirement holds.
    CompleteWithExceptions, ///< Finished with a result, but with recorded exceptions.
    ResourceLimit,          ///< Stopped at a resource cap or allocation failure; no result.
    Cancelled,              ///< Stopped because cancellation was requested; no result.
    Failed,                 ///< Stopped by an error such as an exception or I/O error; no result.
    Infeasible              ///< Input or policy cannot be satisfied; no result.
};

/**
 * @brief Stable lower-case name of a status, as used by the Python bindings.
 * @param[in] status Status to name.
 * @return Static string such as "complete" or "complete_with_exceptions"; "failed" for a value
 *         outside the enumeration. The caller must not free it.
 */
inline const char *statusName(RunStatus status) noexcept
{
    switch (status) {
    case RunStatus::Running:
        return "running";
    case RunStatus::Complete:
        return "complete";
    case RunStatus::CompleteWithExceptions:
        return "complete_with_exceptions";
    case RunStatus::ResourceLimit:
        return "resource_limit";
    case RunStatus::Cancelled:
        return "cancelled";
    case RunStatus::Failed:
        return "failed";
    case RunStatus::Infeasible:
        return "infeasible";
    }
    return "failed";
}

/** @brief Vertex and cell counters of an operation or event; -1 marks a value not available. */
struct Counts {
    // -1 means unavailable. Working vertices can include unused/dead slots;
    // delivered vertices are supplied only once the assembled output is known.
    /**
     * @var vertices
     * @brief Delivered vertex count, known once the output is assembled.
     * @var workingVertices
     * @brief Working vertex slots while meshing, unused and dead slots included.
     * @var triangles
     * @brief Triangle count (live triangles while refining, delivered triangles at the end).
     * @var quads
     * @brief Quadrilateral count.
     */
    std::int64_t vertices = -1, workingVertices = -1, triangles = -1, quads = -1;
    /**
     * @var inserted
     * @brief Vertices inserted by refinement so far.
     * @var removed
     * @brief Vertices removed by coarsening so far.
     */
    std::int64_t inserted = -1, removed = -1;
    /**
     * @brief Total cell count.
     * @return triangles + quads, or -1 when either is unavailable or the sum would overflow.
     */
    std::int64_t cells() const noexcept
    {
        return triangles < 0 || quads < 0 ||
                       triangles > std::numeric_limits<std::int64_t>::max() - quads
                   ? -1
                   : triangles + quads;
    }
};

/**
 * @brief One job notification passed through an EventChannel.
 *
 * The producer fills kind, severity, status, stage, code, message, counts and percent;
 * EventChannel::publish() sets job, sequence and elapsedMs and bounds the strings.
 */
struct Event {
    /**
     * @var job
     * @brief Id of the publishing channel (EventChannel::job()); set by publish().
     * @var sequence
     * @brief Per-channel number, from 1, set by publish(). Numbers of coalesced or evicted
     * events are never drained, so drained sequences can have gaps.
     */
    std::uint64_t job = 0, sequence = 0;
    /// Milliseconds since the channel was constructed (steady clock); set by publish().
    std::int64_t elapsedMs = 0;
    EventKind kind = EventKind::Diagnostic; ///< Kind of notification.
    Severity severity = Severity::Info;     ///< Severity of the notification.
    /** @brief RunStatus::Running for intermediate events; the final status on a Completed event. */
    RunStatus status = RunStatus::Running;
    /**
     * @var stage
     * @brief Short stage name such as "triangulation" or "refinement"; publish() cuts longer
     * text to at most 256 bytes plus "…".
     * @var code
     * @brief Machine-readable code (the job's report code on Completed); publish() cuts longer
     * text to at most 128 bytes plus "…".
     * @var message
     * @brief Human-readable UTF-8 text; publish() cuts longer text to at most 8192 bytes plus
     * "…".
     */
    std::string stage, code, message;
    Counts counts; ///< Counter snapshot when the event was produced.
    /**
     * @brief Overall progress, 0 to 100, or negative (default -1) when not given. publish() clamps
     * a given value to 0..100 and never lets it fall below an earlier value of the channel.
     */
    int percent = -1;
};

// A per-job bounded, thread-safe mailbox. Producers never wait for a UI/event
// loop and retain no QObject. Consumers poll/drain on their own thread.
// Progress is coalesced; overflow is explicitly counted, never hidden.
/**
 * @brief Bounded, thread-safe event queue between one job and its consumer.
 *
 * Producers never wait for the consumer; publish() and drain() only hold an internal mutex,
 * so they may be called from different threads. When the queue is full, the oldest pending
 * Progress event (or, if there is none, the oldest event) is evicted and counted in
 * Batch::dropped. After a Completed event, further events are ignored. The channel is neither
 * copyable nor movable.
 */
class EventChannel
{
public:
    /** @brief Events removed by one drain(). */
    struct Batch {
        std::vector<Event> events; ///< Pending events in publication order, oldest first.
        /** @brief Total events evicted for overflow since the channel was constructed. */
        std::uint64_t dropped = 0; // cumulative non-coalesced overflow
    };

    /**
     * @brief Create an empty channel with a new process-wide job id.
     * @param[in] capacity Most pending events kept before eviction; values below 2 become 2.
     *            Default 2048.
     */
    explicit EventChannel(std::size_t capacity = 2048)
        : capacity_(std::max<std::size_t>(capacity, 2)), job_(nextJob_.fetch_add(1)),
          started_(Clock::now())
    {
    }

    /**
     * @brief Id of this channel, unique among the channels created in the process.
     * @return Job id, counted from 1.
     */
    std::uint64_t job() const noexcept { return job_; }

    /**
     * @brief Queue one event without waiting for the consumer.
     *
     * Cuts message, stage and code longer than 8192, 256 and 128 bytes back to a UTF-8
     * boundary within that size and appends "…" (the caller supplies UTF-8); stamps job,
     * sequence and elapsedMs; and clamps a given percent (see Event::percent). A Progress
     * event overwrites the newest pending event when that is a Progress event of the same
     * stage. Does nothing after a Completed event has been published.
     *
     * @param[in] event Event to queue; taken by value and moved into the queue.
     */
    void publish(Event event)
    {
        // Bounded strings as well as bounded row count. Callers supply UTF-8.
        truncate(event.message, 8192);
        truncate(event.stage, 256);
        truncate(event.code, 128);
        std::lock_guard<std::mutex> guard(mutex_);
        if (finished_)
            return;
        event.job = job_;
        event.sequence = ++sequence_;
        event.elapsedMs =
            std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - started_).count();
        if (event.percent >= 0) {
            percent_ = std::max(percent_, std::clamp(event.percent, 0, 100));
            event.percent = percent_;
        }
        if (event.kind == EventKind::Progress && !pending_.empty() &&
            pending_.back().kind == EventKind::Progress && pending_.back().stage == event.stage) {
            pending_.back() = std::move(event);
            return;
        }
        if (pending_.size() == capacity_) {
            const auto progress =
                std::find_if(pending_.begin(), pending_.end(),
                             [](const Event &e) { return e.kind == EventKind::Progress; });
            if (progress != pending_.end())
                pending_.erase(progress);
            else
                pending_.pop_front();
            ++dropped_;
        }
        finished_ = event.kind == EventKind::Completed;
        pending_.push_back(std::move(event));
    }

    /**
     * @brief Remove and return every pending event.
     * @return The pending events, oldest first, and the cumulative dropped count (which
     *         drain() does not reset). Events are empty when nothing is pending.
     */
    Batch drain()
    {
        std::lock_guard<std::mutex> guard(mutex_);
        Batch batch;
        batch.dropped = dropped_;
        batch.events.reserve(pending_.size());
        while (!pending_.empty()) {
            batch.events.push_back(std::move(pending_.front()));
            pending_.pop_front();
        }
        return batch;
    }

private:
    static void truncate(std::string &s, std::size_t limit)
    {
        if (s.size() <= limit)
            return;
        while (limit > 0 && (static_cast<unsigned char>(s[limit]) & 0xc0) == 0x80)
            --limit;
        s.resize(limit);
        s += "…";
    }
    using Clock = std::chrono::steady_clock;
    const std::size_t capacity_;
    const std::uint64_t job_;
    const Clock::time_point started_;
    inline static std::atomic<std::uint64_t> nextJob_{1};
    std::mutex mutex_;
    std::deque<Event> pending_;
    std::uint64_t sequence_ = 0, dropped_ = 0;
    int percent_ = 0;
    bool finished_ = false;
};

} // namespace meshkit
