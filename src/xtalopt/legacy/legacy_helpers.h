/**********************************************************************
  legacy_helpers - Shared helpers for legacy (pre-v5) compatibility.

  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef XTALOPT_LEGACY_LEGACY_HELPERS_H
#define XTALOPT_LEGACY_LEGACY_HELPERS_H

#include <QString>

namespace XtalOpt {
namespace Legacy {

enum LegacyMolUnitGeometry
{
  LegacyGeomInvalid,
  LegacyGeomLinear,
  LegacyGeomBent,
  LegacyGeomTrigonalPlanar,
  LegacyGeomTrigonalPyramidal,
  LegacyGeomTShaped,
  LegacyGeomTetrahedral,
  LegacyGeomSeeSaw,
  LegacyGeomSquarePlanar,
  LegacyGeomTrigonalBipyramidal,
  LegacyGeomSquarePyramidal,
  LegacyGeomOctahedral
};

struct LegacyMolUnitFields
{
  QString centerSymbol;
  int numCenters;
  QString neighborSymbol;
  int numNeighbors;
  QString geometry;
};

QString normalizedGeometry(const QString& geometry);

LegacyMolUnitGeometry parseGeometry(const QString& geometry);

bool geometryFitsNeighborCount(int numNeighbors, LegacyMolUnitGeometry geometry);

bool isNoCenterSymbol(const QString& symbol);

QString formulaEntry(const QString& symbol, int count);

QString centeredHeteroTemplate(int numNeighbors, LegacyMolUnitGeometry geometry);

QString centeredHomonuclearTemplate(int numNeighbors, LegacyMolUnitGeometry geometry);

QString shellOnlyTemplate(int numNeighbors, LegacyMolUnitGeometry geometry);

} // namespace Legacy
} // namespace XtalOpt

#endif // XTALOPT_LEGACY_LEGACY_HELPERS_H
