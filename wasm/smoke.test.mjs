import {test} from "node:test";
import assert from "node:assert/strict";
import create from "./dist/meshkit.mjs";

const base = {
  points: [0,0, 10,0, 10,10, 0,10, 3,3, 4,3, 4,4, 3,4, 6,2, 8,2],
  paths: [{kind:0,vertices:[0,1,2,3]}, {kind:1,vertices:[4,5,6,7]},
          {kind:2,marker:7,vertices:[8,9]}],
  maximumSize: 3,
  raster: {columns:2,rows:2,x:0,y:0,dx:10,dy:10,heights:[0,10,20,30]}
};
const triangleArea = (r) => {
  let area = 0;
  for (let i=0; i<r.triangles.length; i+=3) {
    const [a,b,c] = r.triangles.slice(i,i+3).map(v=>v*3);
    const p=r.xyz;
    area += ((p[b]-p[a])*(p[c+1]-p[a+1])-(p[b+1]-p[a+1])*(p[c]-p[a]))/2;
  }
  return area;
};

test("raster elevations, excluded hole and constrained breakline survive WASM", async () => {
  const mesh = await create();
  const result = mesh.runJob(base);
  assert.ok(result.ok, result.message);
  assert.ok(result.constraintAuditPassed);
  assert.ok(result.edgeMarkers.includes(7));
  assert.equal(result.sourceVertex.length, 10);
  assert.ok(Math.abs(triangleArea(result)-99)<1e-8);
  assert.equal(result.quads.length, 0);
  assert.equal(result.triangleRegion.length, result.triangles.length / 3);
  for (let i=0; i<result.xyz.length; i+=3)
    assert.ok(Math.abs(result.xyz[i+2]-result.xyz[i]-2*result.xyz[i+1])<1e-8);
  const invalid = mesh.runJob({...base,raster:{...base.raster,rows:3}});
  assert.equal(invalid.ok,false);
});

test("ids, markers, labels, regions, metadata and the report cross the bridge", async () => {
  const mesh = await create();
  const result = mesh.runJob({
    ...base,
    pointIds: [101,102,103,104,105,106,107,108,109,110],
    pointMarkers: [1,1,1,1,2,2,2,2,0,0],
    labels: [{id:5, name:"parking lot"}],
    regions: [{x:1, y:8, region:5}],
    metadata: {crs:"EPSG:32616", description:"smoke"},
    options: {minimumAngleDegrees: 25, smoothingPasses: 1, strictSizing: false},
  });
  assert.ok(result.ok, result.message);
  assert.equal(result.vertexIds[result.sourceVertex[0]], 101);
  assert.equal(result.vertexMarkers[result.sourceVertex[4]], 2);
  assert.ok(result.triangleRegion.every((r) => r === 5), "every cell lies in the seeded region");
  assert.deepEqual(result.labels, [{id:5, name:"parking lot"}]);
  assert.equal(result.metadata.crs, "EPSG:32616");
  assert.equal(result.report.resolvedOptions.minimumAngleDegrees, 25);
  assert.equal(result.report.resolvedOptions.smoothingPasses, 1);
  assert.equal(result.report.constraintAuditPassed, true);
  assert.ok(result.report.inserted >= 0);
  assert.equal(typeof result.report.algorithmVersion, "string");
});

test("authored heights and flatElevation work without a raster", async () => {
  const mesh = await create();
  const {raster, ...noRaster} = base;
  const heights = [1,2,3,4, NaN,NaN,NaN,NaN, 9,9];
  const result = mesh.runJob({...noRaster, pointElevations: heights, options: {flatElevation: 42}});
  assert.ok(result.ok, result.message);
  assert.equal(result.xyz[result.sourceVertex[0]*3+2], 1);
  assert.equal(result.xyz[result.sourceVertex[8]*3+2], 9);
  assert.equal(result.xyz[result.sourceVertex[4]*3+2], 42);
});

test("an out-of-range option and a stale request shape are refused, not crashed", async () => {
  const mesh = await create();
  const bad = mesh.runJob({...base, options: {minimumAngleDegrees: 40}});
  assert.equal(bad.ok, false);
  assert.ok(bad.message.length > 0);
  const badOrder = mesh.runJob({...base, options: {coarseningOrder: "random"}});
  assert.equal(badOrder.ok, false);
  const badIds = mesh.runJob({...base, pointIds: [1,2,3]});
  assert.equal(badIds.ok, false);
  const tooSmall = mesh.runJob({...base, limits: {maxCells: 4}});
  assert.equal(tooSmall.ok, false);
});
