/** Coordinates in XY plane units (metres unless `metadata` says otherwise). Raster nodes run west to east, then south to north. */
export interface MeshRequest {
  points: ArrayLike<number>;
  /** kind: 0 outer ring, 1 hole, 2 line (breakline; `closed` makes it a ring), 3 region ring. */
  paths: {kind: 0 | 1 | 2 | 3; vertices: number[]; marker?: number; closed?: boolean}[];
  /** Global upper bound on nominal cell size; 0 means none. */
  maximumSize: number;
  /** Elevation provider; without it vertices take `pointElevations` or `options.flatElevation`. */
  raster?: {columns: number; rows: number; x: number; y: number; dx: number; dy: number; heights: ArrayLike<number>};
  /** One entry per point; NaN samples the raster (or flatElevation). */
  pointElevations?: ArrayLike<number>;
  /** Positive unique ids per point, delivered as `vertexIds`; empty means 1, 2, 3, ... */
  pointIds?: ArrayLike<number>;
  /** Per-point markers delivered as `vertexMarkers`. */
  pointMarkers?: ArrayLike<number>;
  /** Region names for the ids used by `regions`. */
  labels?: {id: number; name: string}[];
  /** Seeds giving a region id to every cell of the constrained component around the seed point. */
  regions?: {x: number; y: number; region: number}[];
  metadata?: {horizontalMetresPerUnit?: number; verticalMetresPerUnit?: number; crs?: string; description?: string};
  /** Meshing policy; the native job refuses out-of-range values with code "option". */
  options?: {
    minimumTarget?: number;        // advisory minimum nominal size, >= 0 (0 off)
    minimumEdge?: number;          // minimum edge length, >= 0 (0 off)
    minimumAngleDegrees?: number;  // 0..34, default 30
    adjacentRatio?: number;        // 0 off, else >= 1
    flatElevation?: number;        // Z for unauthored vertices without a raster
    coarseningReferenceSize?: number;
    terrainTolerance?: number;     // >= 0; positive enables terrain certification (raster required)
    smoothingPasses?: number;      // 0..10
    coarsenPasses?: number;        // 0..10
    sizingRepairPasses?: number;   // 0..3, default 2
    repairInsertions?: number;
    strictSizing?: boolean;        // default true: unresolved size conflicts fail the job
    requireTerrainCertification?: boolean;
    retainInputMapping?: boolean;  // default true: fills sourceVertex
    coarseningOrder?: "legacyFlatness" | "resolutionAware";
    cells?: "triangles" | "automaticMixed";
  };
  /** Resource caps; defaults 200 000 cells and vertices, 256 MB workspace. */
  limits?: {maxCells?: number; maxVertices?: number; maxWorkspaceMB?: number};
}
export interface MeshReport {
  algorithmVersion: string; constraintAuditPassed: boolean; constraintSegments: number;
  plannedPatches: number; planningRetries: number; inserted: number; removed: number;
  belowMinimum: number; edgeConflicts: number; sizeConflicts: number; transitionConflicts: number;
  terrainConflicts: number; terrainUnknown: number; largestAdjacentRatio: number; largestTerrainRatio: number;
  exampleCount: number;
  resolvedOptions: {maximumSize: number; minimumTarget: number; minimumEdge: number; minimumAngleDegrees: number;
    adjacentRatio: number; smoothingPasses: number; coarsenPasses: number; sizingRepairPasses: number;
    strictSizing: boolean; terrainTolerance: number};
}
export type MeshResult =
  | {ok: false; status: string; message: string; code?: string; report?: MeshReport}
  | {ok: true; status: string; message: string; code: string; constraintAuditPassed: boolean;
     schemaVersion: number; report: MeshReport;
     xyz: Float64Array; triangles: Uint32Array; triangleRegion: Int32Array;
     quads: Uint32Array; quadRegion: Int32Array;
     edgeVertices: Uint32Array; edgeMarkers: Int32Array;
     vertexMarkers: Int32Array; vertexIds: Float64Array; sourceVertex: Uint32Array;
     labels: {id: number; name: string}[];
     metadata: {horizontalMetresPerUnit: number; verticalMetresPerUnit: number; crs: string; description: string}};
export default function createMeshKit(options?: {locateFile?: (name: string) => string}):
  Promise<{runJob(request: MeshRequest): MeshResult}>;
