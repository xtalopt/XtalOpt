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

#include <xtalopt/legacy/legacy_helpers.h>

namespace XtalOpt {
namespace Legacy {

QString normalizedGeometry(const QString& geometry)
{
  QString g = geometry.simplified().toLower();
  g.replace('_', ' ');
  g.replace('-', ' ');
  return g.simplified();
}

LegacyMolUnitGeometry parseLegacyGeometry(const QString& geometry)
{
  const QString g = normalizedGeometry(geometry);
  if (g == "linear")
    return LegacyGeomLinear;
  if (g == "bent")
    return LegacyGeomBent;
  if (g == "trigonal planar")
    return LegacyGeomTrigonalPlanar;
  if (g == "trigonal pyramidal")
    return LegacyGeomTrigonalPyramidal;
  if (g == "t shaped")
    return LegacyGeomTShaped;
  if (g == "tetrahedral")
    return LegacyGeomTetrahedral;
  if (g == "see saw")
    return LegacyGeomSeeSaw;
  if (g == "square planar")
    return LegacyGeomSquarePlanar;
  if (g == "trigonal bipyramidal")
    return LegacyGeomTrigonalBipyramidal;
  if (g == "square pyramidal")
    return LegacyGeomSquarePyramidal;
  if (g == "octahedral")
    return LegacyGeomOctahedral;
  return LegacyGeomInvalid;
}

bool geometryFitsNeighborCount(int numNeighbors, LegacyMolUnitGeometry geometry)
{
  switch (numNeighbors) {
    case 1:
      return geometry == LegacyGeomLinear;
    case 2:
      return geometry == LegacyGeomLinear || geometry == LegacyGeomBent;
    case 3:
      return geometry == LegacyGeomTrigonalPlanar || geometry == LegacyGeomTrigonalPyramidal ||
             geometry == LegacyGeomTShaped;
    case 4:
      return geometry == LegacyGeomTetrahedral || geometry == LegacyGeomSeeSaw ||
             geometry == LegacyGeomSquarePlanar;
    case 5:
      return geometry == LegacyGeomTrigonalBipyramidal || geometry == LegacyGeomSquarePyramidal;
    case 6:
      return geometry == LegacyGeomOctahedral;
    default:
      return false;
  }
}

bool isNoCenterSymbol(const QString& symbol)
{
  const QString s = symbol.trimmed();
  return s.isEmpty() || s == "0" || s.compare("none", Qt::CaseInsensitive) == 0;
}

QString formulaEntry(const QString& symbol, int count)
{
  return QString("%1%2").arg(symbol).arg(count);
}

QString centeredHeteroTemplate(int numNeighbors, LegacyMolUnitGeometry geometry)
{
  switch (numNeighbors) {
    case 1:
      return "linear_2_hetero";
    case 2:
      return geometry == LegacyGeomLinear
               ? "linear_d0h_center_shell"
               : "bent_c2v_center_shell";
    case 3:
      if (geometry == LegacyGeomTrigonalPlanar)
        return "trigonal_planar_d3h_center_shell";
      if (geometry == LegacyGeomTrigonalPyramidal)
        return "trigonal_pyramidal_c3v_center_shell";
      return "t_shaped_c2v_shared_neighbors";
    case 4:
      if (geometry == LegacyGeomTetrahedral)
        return "tetrahedral_td_center_shell";
      if (geometry == LegacyGeomSeeSaw)
        return "see_saw_c2v_shared_neighbors";
      return "square_planar_d4h_center_shell";
    case 5:
      return geometry == LegacyGeomTrigonalBipyramidal
               ? "trigonal_bipyramidal_d3h_shared_neighbors"
               : "square_pyramidal_c4v_shared_neighbors";
    case 6:
      return "octahedral_oh_center_shell";
    default:
      return QString();
  }
}

QString centeredHomonuclearTemplate(int numNeighbors, LegacyMolUnitGeometry geometry)
{
  switch (numNeighbors) {
    case 1:
      return "linear_2_pair";
    case 2:
      return geometry == LegacyGeomLinear
               ? "linear_d0h_homonuclear_center_shell"
               : "bent_c2v_homonuclear_center_shell";
    case 3:
      if (geometry == LegacyGeomTrigonalPlanar)
        return "trigonal_planar_d3h_homonuclear_center_shell";
      if (geometry == LegacyGeomTrigonalPyramidal)
        return "trigonal_pyramidal_c3v_homonuclear_center_shell";
      return "t_shaped_c2v_homonuclear";
    case 4:
      if (geometry == LegacyGeomTetrahedral)
        return "tetrahedral_td_homonuclear_center_shell";
      if (geometry == LegacyGeomSeeSaw)
        return "see_saw_c2v_homonuclear";
      return "square_planar_d4h_homonuclear_center_shell";
    case 5:
      return geometry == LegacyGeomTrigonalBipyramidal
               ? "trigonal_bipyramidal_d3h_homonuclear"
               : "square_pyramidal_c4v_homonuclear";
    case 6:
      return "octahedral_oh_homonuclear_center_shell";
    default:
      return QString();
  }
}

QString shellOnlyTemplate(int numNeighbors, LegacyMolUnitGeometry geometry)
{
  switch (numNeighbors) {
    case 1:
      return QString();
    case 2:
      return "linear_2_pair";
    case 3:
      if (geometry == LegacyGeomTShaped)
        return "t_shaped_c2v_homonuclear_shell";
      return "trigonal_planar_d3h_shell";
    case 4:
      if (geometry == LegacyGeomTetrahedral)
        return "tetrahedral_td_shell";
      if (geometry == LegacyGeomSeeSaw)
        return "see_saw_c2v_homonuclear_shell";
      return "square_planar_d4h_shell";
    case 5:
      return geometry == LegacyGeomTrigonalBipyramidal
               ? "trigonal_bipyramidal_d3h_homonuclear_shell"
               : "square_pyramidal_c4v_homonuclear_center_shell";
    case 6:
      return "octahedral_oh_shell";
    default:
      return QString();
  }
}

} // namespace Legacy
} // namespace XtalOpt
