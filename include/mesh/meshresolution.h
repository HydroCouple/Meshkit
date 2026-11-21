// SPDX-License-Identifier: Apache-2.0

/**
 * @file      meshresolution.h
 * @brief     Shared mesh resolution settings and validation.
 *
 * @author    Caleb Buahin <caleb.buahin@gmail.com>
 * @copyright Copyright (c) 2026 Caleb Buahin. All rights reserved.
 * @license   Apache-2.0
 */

#ifndef OPENSWMM_MESH_RESOLUTION_H
#define OPENSWMM_MESH_RESOLUTION_H

#include <QJsonObject>
#include "terrainpolicy.h"
#include <QString>
#include <algorithm>
#include <cmath>
#include <optional>

namespace mesh
{

// Shared by project settings, Preferences and the generation dialog.
// Physical lengths are SI; absence means no maximum, inherit minimum,
// automatic tolerance, or match-minimum averaging respectively.
/**
 * @brief Unit-independent mesh resolution choices (lengths in SI meters) with JSON
 * round-trip, legacy migration and validation. Plain value type; validate() is never applied
 * implicitly.
 */
struct ResolutionSettings {
    /// Minimum target cell size, meters (default 6.096 m = 20 ft); finite in (0, 1e12].
    double minimumM = 6.096;
    /** @var maximumM
     *  Maximum target cell size, meters; absent = no limit, else >= minimumM. */
    /** @var featureM
     *  Target size near selected features when refineFeatures; absent = minimumM. With
     *  refineFeatures it must lie within [minimumM, maximumM]. */
    /** @var toleranceM
     *  Terrain elevation tolerance, vertical meters (JSON "elevationToleranceM"); absent =
     *  automatic estimate. */
    /** @var breaklineToleranceM
     *  Terrain break-line detection tolerance, vertical meters; absent = automatic. */
    /** @var averagingM
     *  DEM averaging scale for the Averaged reference, meters; absent = minimumM. */
    /** @var minimumBreaklineLengthM
     *  Shortest terrain break line to keep, meters; absent = the caller's default. */
    std::optional<double> maximumM, featureM, toleranceM, breaklineToleranceM, averagingM,
        minimumBreaklineLengthM;
    /// Cap on terrain break lines, 0..10,000,000; 0 = let the caller choose automatically.
    int maxBreaklines = 0;
    int coarsenPasses = 0; ///< Absent in older projects: retain their original mesh behavior.
    /// Size transition (growth) rate between neighboring targets; finite and >= 1.05.
    double transition = 2.0;
    /** @enum Sizing
     *  Sizing rules: Legacy targets; Verified sizes (report exceptions); Strict sizes (refuse
     *  unmet limits). */
    /** @var sizing
     *  Sizing rules in force (default Legacy). Non-Legacy is required by Unified planning,
     *  ResolutionAware coarsening, size-scaled terrain tolerance and minimumEdgeM. */
    enum class Sizing {
        Legacy,   ///< Legacy size targets.
        Verified, ///< Verified sizes; unmet limits are reported as exceptions.
        Strict    ///< Verified sizes; unmet limits refuse the mesh.
    } sizing = Sizing::Legacy;
    /** @enum CellPlanning
     *  Quad patch planning: Legacy per-region routing, or Unified joint planning. */
    /** @var cellPlanning
     *  Patch planning policy (default Legacy); Unified requires non-Legacy sizing. */
    enum class CellPlanning {
        Legacy, ///< Per-region quad routing.
        Unified ///< Joint planning of all quad patches.
    } cellPlanning = CellPlanning::Legacy;
    /// Separate floor on delivered edge length, meters; absent = none. Needs non-Legacy sizing.
    std::optional<double> minimumEdgeM;
    /// Coarsening removal order (default LegacyFlatness); ResolutionAware needs non-Legacy
    /// sizing.
    CoarseningOrder coarseningOrder=CoarseningOrder::LegacyFlatness;
    /// Terrain tolerance rule (constant or scaled with target size); its sizes and tolerances
    /// are meters. Must satisfy TerrainTolerancePolicy::valid() and stay <= 1e12.
    TerrainTolerancePolicy terrainTolerancePolicy; // SI meters, independent of display units.

    /** @var refineFeatures
     *  Refine to featureM near selected features (default false). */
    /** @var alignTerrain
     *  Detect terrain break lines and align mesh edges to them (default true). */
    bool refineFeatures = false, alignTerrain = true;
    /** @enum Terrain
     *  Terrain refinement method: Adaptive elevation error, Legacy block sizing, or Off. */
    /** @var terrain
     *  Terrain refinement method (default Adaptive); TargetSize tolerance needs Adaptive. */
    enum class Terrain {
        Adaptive, ///< Refine by elevation error against the terrain.
        Legacy,   ///< Legacy block-based terrain sizing.
        Off       ///< No terrain refinement.
    } terrain = Terrain::Adaptive;
    /** @enum Reference
     *  Terrain reference: Averaged DEM, Full DEM, or Direct (full DEM read without a local
     *  copy). */
    /** @var reference
     *  Terrain reference DEM (default Averaged). */
    enum class Reference {
        Averaged, ///< Averaged DEM.
        Full,     ///< Full-resolution DEM.
        Direct    ///< Full DEM read directly, without a local copy.
    } reference = Reference::Averaged;

    /**
     * @brief Check numeric mesh-resolution settings without modifying or normalizing them.
     * @return Empty string when accepted, otherwise the first English validation error. All
     * present lengths must be finite in (0,1e12] meters, maximum must not precede minimum, and
     * enabled feature refinement must lie within minimum/maximum. Coarsening passes must be
     * 0..10, breakline count 0..10000000, transition finite and >=1.05. Enum validity and
     * relationships between other tolerances are not checked. No project mutation or implicit
     * fallback application.
     */
    QString validate() const
    {
        auto positive = [](double v) { return std::isfinite(v) && v > 0 && v <= 1e12; };
        if (int(sizing) < 0 || int(sizing) > 2)
            return QStringLiteral("Invalid sizing rules version or mode.");
        if(int(cellPlanning)<0 || int(cellPlanning)>1)
            return QStringLiteral("Invalid cell planning policy.");
        if(cellPlanning==CellPlanning::Unified && sizing==Sizing::Legacy)
            return QStringLiteral("Unified cell planning requires verified or strict sizing rules.");
        if(coarseningOrder!=CoarseningOrder::LegacyFlatness && coarseningOrder!=CoarseningOrder::ResolutionAware)
            return QStringLiteral("Invalid coarsening policy version or order.");
        if(!terrainTolerancePolicy.valid() || terrainTolerancePolicy.referenceSize>1e12 || terrainTolerancePolicy.minimumTolerance>1e12 || terrainTolerancePolicy.maximumTolerance>1e12)
            return QStringLiteral("Invalid resolution-weighted terrain tolerance policy.");
        if((coarseningOrder==CoarseningOrder::ResolutionAware || terrainTolerancePolicy.mode==TerrainToleranceMode::TargetSize) && sizing==Sizing::Legacy)
            return QStringLiteral("Resolution-weighted terrain or coarsening requires verified sizing.");
        if(terrainTolerancePolicy.mode==TerrainToleranceMode::TargetSize) {
            if(terrain!=Terrain::Adaptive)return QStringLiteral("Size-weighted tolerance requires adaptive terrain refinement.");
            if(toleranceM && !std::isfinite(terrainTolerancePolicy.tolerance(*toleranceM,terrainTolerancePolicy.referenceSize)))
                return QStringLiteral("The terrain tolerance floor exceeds its resolved cap.");
        }
        if (minimumEdgeM && !positive(*minimumEdgeM))
            return QStringLiteral("The separate minimum edge requirement must be a positive finite length.");
        if (sizing == Sizing::Legacy && minimumEdgeM)
            return QStringLiteral("Choose verified or strict sizing to request a minimum edge length.");
        if (!positive(minimumM))
            return QStringLiteral("Minimum target cell size must be a positive finite length.");
        for (const auto &v : {maximumM, featureM, toleranceM, breaklineToleranceM, averagingM,
                              minimumBreaklineLengthM})
            if (v && !positive(*v))
                return QStringLiteral(
                    "Specified sizes and tolerances must be positive finite lengths.");
        if (coarsenPasses < 0 || coarsenPasses > 10)
            return QStringLiteral("Coarsening passes must be between 0 and 10.");
        if (maxBreaklines < 0 || maxBreaklines > 10000000)
            return QStringLiteral("Terrain breakline count is out of range.");
        if (maximumM && *maximumM < minimumM)
            return QStringLiteral(
                "Maximum target cell size must be at least the minimum target size.");
        if (refineFeatures && featureM && *featureM < minimumM)
            return QStringLiteral("Feature target size must be at least the minimum target size.");
        if (refineFeatures && featureM && maximumM && *featureM > *maximumM)
            return QStringLiteral(
                "Feature target size exceeds the maximum target size. Increase the maximum or reduce the feature target.");
        if (!std::isfinite(transition) || transition < 1.05)
            return QStringLiteral("Size transition rate must be finite and at least 1.05.");
        return {};
    }

    /**
     * @brief Serialize every resolution field into the current project JSON schema.
     * @return Owned object with SI meter length controls, null for absent optional lengths, and
     * textual terrain/reference modes. Unknown enum values serialize as off/direct respectively
     * through fallback branches. Does not call validate(), perform unit conversion or write a
     * file; this object is unchanged.
     */
    QJsonObject toJson() const
    {
        auto value = [](std::optional<double> v) {
            return v ? QJsonValue(*v) : QJsonValue(QJsonValue::Null);
        };
        return {{"coarseningPolicy",QJsonObject{{"version",1},{"order",int(coarseningOrder)}}},
                {"terrainTolerancePolicy",QJsonObject{{"version",1},{"mode",int(terrainTolerancePolicy.mode)},
                    {"referenceSizeM",terrainTolerancePolicy.referenceSize},{"minimumM",terrainTolerancePolicy.minimumTolerance},
                    {"maximumM",terrainTolerancePolicy.maximumTolerance}}}, {"sizingVersion", 1}, {"sizingMode", int(sizing)},
                {"cellPlanningVersion",1},{"cellPlanningMode",int(cellPlanning)},
                {"minimumEdgeM", value(minimumEdgeM)}, {"coarsenPasses", coarsenPasses},
                {"minimumM", minimumM},
                {"maximumM", value(maximumM)},
                {"featureM", value(featureM)},
                {"elevationToleranceM", value(toleranceM)},
                {"breaklineToleranceM", value(breaklineToleranceM)},
                {"averagingM", value(averagingM)},
                {"minimumBreaklineLengthM", value(minimumBreaklineLengthM)},
                {"maxBreaklines", maxBreaklines},
                {"transition", transition},
                {"refineFeatures", refineFeatures},
                {"alignTerrain", alignTerrain},
                {"terrain", terrain == Terrain::Adaptive ? "adaptive"
                            : terrain == Terrain::Legacy ? "legacy"
                                                         : "off"},
                {"reference", reference == Reference::Averaged ? "averaged"
                              : reference == Reference::Full   ? "full"
                                                               : "direct"}};
    }

    /**
     * @brief Decode current-schema resolution fields using defaults for missing or mismatched
     * values.
     * @param[in] o Borrowed object, unchanged; numeric optional lengths become present only for
     * JSON numbers.
     * @return Owned settings, distances in meters; unknown terrain/reference strings select
     * adaptive/averaged. Missing optional values remain absent. Does not validate ranges, clamp
     * values, convert project units or mutate project state; callers should call validate()
     * before using authoring inputs.
     */
    static ResolutionSettings fromJson(const QJsonObject &o)
    {
        ResolutionSettings r;
        auto value = [&](const char *key) -> std::optional<double> {
            const auto v = o.value(QLatin1String(key));
            return v.isDouble() ? std::optional<double>(v.toDouble()) : std::nullopt;
        };
        r.coarsenPasses = o.value("coarsenPasses").toInt(0);
        const auto coarsening=o.value("coarseningPolicy").toObject();
        r.coarseningOrder=CoarseningOrder(coarsening.value("order").toInt(0));
        if(!coarsening.isEmpty() && coarsening.value("version").toInt()!=1)r.coarseningOrder=CoarseningOrder(-1);
        const auto tolerancePolicy=o.value("terrainTolerancePolicy").toObject();
        r.terrainTolerancePolicy.mode=TerrainToleranceMode(tolerancePolicy.value("mode").toInt(0));
        if(!tolerancePolicy.isEmpty() && tolerancePolicy.value("version").toInt()!=1)r.terrainTolerancePolicy.mode=TerrainToleranceMode(-1);
        r.terrainTolerancePolicy.referenceSize=tolerancePolicy.value("referenceSizeM").toDouble(10);
        r.terrainTolerancePolicy.minimumTolerance=tolerancePolicy.value("minimumM").toDouble(0);
        r.terrainTolerancePolicy.maximumTolerance=tolerancePolicy.value("maximumM").toDouble(0);

        r.sizing = Sizing(o.value("sizingMode").toInt(0));
        r.cellPlanning=CellPlanning(o.value("cellPlanningMode").toInt(0));
        if(o.contains("cellPlanningVersion") && o.value("cellPlanningVersion").toInt()!=1)r.cellPlanning=CellPlanning(-1);
        if (o.contains("sizingVersion") && o.value("sizingVersion").toInt() != 1)
            r.sizing = Sizing(-1);
        r.minimumEdgeM = value("minimumEdgeM");
        r.minimumM = o.value("minimumM").toDouble(r.minimumM);
        r.maximumM = value("maximumM");
        r.featureM = value("featureM");
        r.toleranceM = value("elevationToleranceM");
        r.breaklineToleranceM = value("breaklineToleranceM");
        r.minimumBreaklineLengthM = value("minimumBreaklineLengthM");
        r.maxBreaklines = o.value("maxBreaklines").toInt(0);
        r.averagingM = value("averagingM");
        r.transition = o.value("transition").toDouble(r.transition);
        r.refineFeatures = o.value("refineFeatures").toBool(false);
        r.alignTerrain = o.value("alignTerrain").toBool(true);
        const QString mode = o.value("terrain").toString(), ref = o.value("reference").toString();
        r.terrain = mode == "off"      ? Terrain::Off
                    : mode == "legacy" ? Terrain::Legacy
                                       : Terrain::Adaptive;
        r.reference = ref == "direct" ? Reference::Direct
                      : ref == "full" ? Reference::Full
                                      : Reference::Averaged;
        return r;
    }

    // Version 1 stored distance controls in meters and combo choices as indices.
    /**
     * @brief Overlay version-1 generation-dialog controls onto a supplied settings value.
     * @param[in] c Borrowed legacy control object with meter distances and integer combo
     * selections, unchanged.
     * @param[in] r Initial settings copied by value; fields without legacy controls retain their
     * supplied values, except coarsenPasses resets to zero and maximum/breakline tolerance are
     * derived.
     * @param[in] extentM Domain extent in meters, used as extentM/100 when legacy feature size
     * is nonpositive; if not positive the fallback is 1 meter. Caller supplies finite meaningful
     * values.
     * @return Owned migrated settings. Nonpositive minimum becomes feature/4, maximum becomes
     * feature times coarsening factor, nonpositive elevation tolerance becomes absent and copies
     * to breakline tolerance. Legacy terrain without a positive tolerance switches off. No range
     * validation or file/project mutation.
     */
    static ResolutionSettings migrate(const QJsonObject &c, ResolutionSettings r, double extentM)
    {
        r.coarsenPasses = 0;
        r.coarseningOrder=CoarseningOrder::LegacyFlatness;
        r.terrainTolerancePolicy={};
        r.sizing = Sizing::Legacy;
        r.cellPlanning=CellPlanning::Legacy;
        r.minimumEdgeM.reset();
        double feature = c.value("meshCellSizeSpin").toDouble(r.featureM.value_or(r.minimumM));
        if (feature <= 0)
            feature = extentM > 0 ? extentM / 100.0 : 1.0;
        r.featureM = feature;
        r.minimumM = c.value("meshMinCellSizeSpin").toDouble(r.minimumM);
        if (r.minimumM <= 0)
            r.minimumM = feature / 4;
        r.maximumM = feature * c.value("meshCoarsenSpin").toDouble(1000.0);
        const double tol = c.value("meshTerrainTolSpin").toDouble(r.toleranceM.value_or(0));
        r.toleranceM = tol > 0 ? std::optional<double>(tol) : std::nullopt;
        r.breaklineToleranceM = r.toleranceM;
        r.refineFeatures = c.value("meshRefineFeaturesBox").toBool(r.refineFeatures);
        r.alignTerrain = c.value("meshTerrainBreaklinesBox").toBool(r.alignTerrain);
        r.maxBreaklines = c.value("meshMaxBreaklinesSpin").toInt(0);
        r.transition = c.value("meshSizeRatioSpin").toDouble(r.transition);
        const int mode = c.value("meshTerrainModeCombo").toInt(0),
                  ref = c.value("meshTerrainReferenceCombo").toInt(0);
        r.terrain = mode == 2 ? Terrain::Off : mode == 1 ? Terrain::Legacy : Terrain::Adaptive;
        r.reference = ref == 2   ? Reference::Direct
                      : ref == 1 ? Reference::Full
                                 : Reference::Averaged;
        // Legacy zero explicitly disabled refinement, unlike adaptive auto.
        if (r.terrain == Terrain::Legacy && !r.toleranceM)
            r.terrain = Terrain::Off;
        return r;
    }
};
}
#endif
