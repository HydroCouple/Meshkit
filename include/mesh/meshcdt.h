// SPDX-License-Identifier: Apache-2.0
// Qt storage facade over the shared constrained triangulation implementation.
#pragma once
#include "meshkit/cdt.hpp"
#include "mesh/qtcdtstorage.h"
#include "mesh/terrainpolicy.h"
#include "mesh/meshprogress.h"
namespace mesh {
/// @brief The constrained Delaunay kernel on Qt storage (QVector, QPointF, QString); the same
/// implementation as meshkit::ConstrainedDelaunay, which uses standard containers.
using ConstrainedDelaunay=meshkit::BasicConstrainedDelaunay<meshkit::detail::QtStorage>;
}
