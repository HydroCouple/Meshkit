// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "mesh.hpp"
#include <filesystem>
#include <functional>
#include <istream>

namespace meshkit
{
/**
 * @brief How read2dm() treats a card other than MESH2D, ND, E3T, E4Q or NS.
 *
 * Blank lines and lines whose first token starts with `#` or `;;` are always skipped. The MKB
 * functions ignore this setting.
 */
enum class UnknownRecords { Reject, IgnoreWithReport };
/** @var UnknownRecords::Reject
 *  @brief Fail the read with code `unsupported_card` (the default). */
/** @var UnknownRecords::IgnoreWithReport
 *  @brief Skip the card, count it in IOReport::ignoredRecords, record a bounded
 *  `ignored_card:<card>` code in IOReport::losses and finish `CompleteWithExceptions`. */
/**
 * @brief Options shared by the MKB and 2DM readers and writers.
 *
 * The cancellation callback runs synchronously on the calling thread.
 */
struct IOOptions {
    /// Count, workspace-estimate, work, metadata-byte and 2DM line-length limits.
    Limits limits;
    /// Optional poll; returning true aborts with status `Cancelled` (code `cancelled`). Empty
    /// never cancels. Polled when the operation starts and periodically while it runs.
    std::function<bool()> cancelled;
    /// Writes only. False (default) refuses to replace an existing file (code `publish`); true
    /// replaces it atomically.
    bool overwrite = false, durable = false, allowAttributeLoss = false;
    /** @var durable
     *  @brief Writes only. True flushes the temporary file to storage (fsync/_commit) before
     *  publication; directory-entry durability is not promised. */
    /** @var allowAttributeLoss
     *  @brief write2dm() only. False (default) refuses a mesh whose attributes 2DM cannot hold
     *  (code `attribute_loss`); true writes it and lists the lost categories in
     *  IOReport::losses. */
    bool legacy2dm =
        false; ///< Compatibility ids/optional materials; still rejects malformed geometry.
    /// read2dm() only: handling of unsupported cards.
    UnknownRecords unknownRecords = UnknownRecords::Reject;
};
/** @brief Outcome of one read or write: status, code, message, counts, timing and byte total. */
struct IOReport : OperationReport {
    /// Bytes written or read (MKB includes the checksum footer; a 2DM read reports the file
    /// size measured by its first pass). Set only on success, otherwise 0.
    std::uint64_t bytes = 0, ignoredRecords = 0, reorientedCells = 0;
    /** @var ignoredRecords
     *  @brief read2dm() only: cards skipped under UnknownRecords::IgnoreWithReport. */
    /** @var reorientedCells
     *  @brief read2dm() only: clockwise cells whose corner order was reversed. */
    std::vector<std::string> losses; ///< Bounded codes, with total ignored-record count above.
};
/** @brief Mesh and report returned by the readers. */
struct ReadResult {
    /// The validated mesh; default-constructed (empty) whenever the read fails.
    Mesh mesh;
    /// Status, code, message, counts and byte total of the read.
    IOReport report;
};
// Version-1 MKB is lossless for the generic model, with counted little-endian
// records and a trailing content checksum. No host struct/padding is serialized.
// Read failures expose no partial mesh. Writes use an exclusive same-directory
// temporary and atomic publication; cancellation/failure preserves an old file.
/**
 * @brief Read a version-1 MeshKit binary (MKB) mesh from a stream.
 *
 * The stream (borrowed, binary mode, need not be seekable) is read in 64 KiB blocks through to
 * its end: bytes after the checksum footer fail with `trailing_data`. The options supply the
 * count, workspace, work and metadata-byte limits and the cancellation poll. Counts are checked
 * against the limits before bulk allocation; the checksum and validateMesh() are checked last.
 * Failures are returned in the report, not thrown (exceptions derived from std::exception,
 * including allocation failure, are converted); the mesh is then empty.
 * @return The mesh and a report with status `Complete`, code `read`, vertex/triangle/quad
 * counts, bytes consumed and the number of records read.
 */
ReadResult readBinary(std::istream &, const IOOptions & = {});
/**
 * @brief Open a file in binary mode and read it as MKB, as the stream overload does.
 *
 * An empty path or one with an embedded NUL returns code `path`, and a file that cannot be
 * opened returns `open_input`; both keep the default status `Failed` and an empty mesh.
 * @return The stream overload's result, or one of the failures above.
 */
ReadResult readBinary(const std::filesystem::path &, const IOOptions & = {});
/**
 * @brief Validate a mesh and atomically publish it to a path as version-1 MKB.
 *
 * The mesh (borrowed, unchanged) is checked by validateMesh() under the options' limits before
 * any file is created. Records stream to an exclusive temporary file in the destination's
 * directory, which is then published under the overwrite and durable options. Cancellation or
 * failure removes the temporary and leaves any existing file unchanged. Failures are returned
 * in the report, not thrown.
 * @return Status `Complete` with code `written`, bytes written and work used; otherwise the
 * failure status, code and message.
 */
IOReport writeBinary(const std::filesystem::path &, const Mesh &, const IOOptions & = {});
// Bounded-record, two-pass seekable file reader: elements may precede nodes.
// E3T/E4Q/ND/standard NS supported. Unsupported cards are never silently lost.
// 2DM does not declare physical units; callers must supply them before meshing.
/**
 * @brief Read an SMS 2DM file in two passes (nodes, then elements and node strings).
 *
 * The path must name a seekable file that does not change between the passes (`source_changed`
 * otherwise). Lines longer than Limits::maxRecordBytes fail. IDs must be positive and material
 * IDs int32 unless IOOptions::legacy2dm; unsupported cards follow IOOptions::unknownRecords.
 * Clockwise cells are reversed and counted; degenerate or nonconvex cells fail. A node string
 * of more than two nodes whose last ID repeats its first becomes a closed path. The result has
 * unknown units (Metadata::unitsKnown is false). Failures are returned in the report, not
 * thrown.
 * @return The validated mesh with status `Complete`, or `CompleteWithExceptions` when cards
 * were ignored; on failure an empty mesh and the failure status, code and message.
 */
ReadResult read2dm(const std::filesystem::path &, const IOOptions & = {});
/**
 * @brief Validate a mesh and atomically publish it to a path as SMS 2DM.
 *
 * The mesh (borrowed, unchanged) must pass validateMesh(), have nodes and cells, and positive
 * external node and element IDs. Cell regions become materials; paths become NS records (ten
 * IDs per line, last ID negated, a closed path repeats its first node). Coordinates use 17
 * significant digits. Units/CRS/description, vertex markers, labels, aliases, edges and
 * non-default path attributes cannot be written: they are refused (code `attribute_loss`)
 * unless IOOptions::allowAttributeLoss. Publication follows writeBinary().
 * @return Status `Complete` (code `written`), or `CompleteWithExceptions` with the loss codes
 * in IOReport::losses; otherwise the failure status, code and message.
 */
IOReport write2dm(const std::filesystem::path &, const Mesh &, const IOOptions & = {});
}
