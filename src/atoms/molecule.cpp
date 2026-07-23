/**********************************************************************
  molecule - A basic molecule (0D geometries) module.

  Copyright (C) 2017 by Patrick S. Avery
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <atoms/molecule.h>

#include <common/constants.h>
#include <common/matrix.h>
#include <common/output.h>
#include <common/numericutils.h>
#include <common/stringutils.h>
#include <common/compatibility/platform_compat.h>
#include <common/compatibility/qt_compat.h>
#include <atoms/eleminfo.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <limits>
#include <map>
#include <QStringList>
#include <sstream>
#include <string>
#include <vector>

extern "C" {
#include <libmsym/src/msym.h>
}

namespace Atoms {

namespace {

struct MoleculeTemplate
{
  const char* name;
  const char* pointGroup;
  const char* description;
  unsigned int orbitCount;
  unsigned int orbitSizes[3];
  double orbitSeeds[3][3];
  unsigned int speciesCount;
  unsigned int orbitSpecies[3];
};

static const MoleculeTemplate moleculeTemplates[] = {
  {
    "linear_2_pair", "D0h",
    "two equivalent atoms related by inversion",
    1, { 2, 0, 0 },
    { { 0.0, 0.0, 1.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 } }
  },
  {
    "trigonal_planar_d3h_shell", "D3h",
    "three equivalent trigonal-planar shell atoms",
    1, { 3, 0, 0 },
    { { 1.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 } }
  },
  {
    "linear_d0h_homonuclear_center_shell", "D0h",
    "center and two linear shell atoms",
    2, { 1, 2, 0 },
    { { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 1.0 }, { 0.0, 0.0, 0.0 } },
    1, { 0, 0, 0 }
  },
  {
    "bent_c2v_homonuclear_center_shell", "C2v",
    "center and two bent shell atoms",
    2, { 1, 2, 0 },
    { { 0.0, 0.0, 0.0 }, { 1.0, 0.0, 0.5 }, { 0.0, 0.0, 0.0 } },
    1, { 0, 0, 0 }
  },
  {
    "t_shaped_c2v_homonuclear_shell", "C2v",
    "t-shaped shell atoms",
    2, { 1, 2, 0 },
    { { 0.0, 0.0, 1.0 }, { 1.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 } },
    1, { 0, 0, 0 }
  },
  {
    "tetrahedral_td_shell", "Td",
    "four equivalent tetrahedral shell atoms",
    1, { 4, 0, 0 },
    { { 1.0, 1.0, 1.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 } }
  },
  {
    "square_planar_d4h_shell", "D4h",
    "four equivalent square-planar shell atoms",
    1, { 4, 0, 0 },
    { { 1.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 } }
  },
  {
    "trigonal_pyramidal_c3v_homonuclear_center_shell", "C3v",
    "axial center and three trigonal-pyramidal atoms",
    2, { 1, 3, 0 },
    { { 0.0, 0.0, 0.0 }, { 1.0, 0.0, -0.5 }, { 0.0, 0.0, 0.0 } },
    1, { 0, 0, 0 }
  },
  {
    "trigonal_planar_d3h_homonuclear_center_shell", "D3h",
    "center and three trigonal-planar shell atoms",
    2, { 1, 3, 0 },
    { { 0.0, 0.0, 0.0 }, { 1.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 } },
    1, { 0, 0, 0 }
  },
  {
    "t_shaped_c2v_homonuclear", "C2v",
    "center and one axial and two side atoms",
    3, { 1, 1, 2 },
    { { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 1.0 }, { 1.0, 0.0, 0.0 } },
    1, { 0, 0, 0 }
  },
  {
    "see_saw_c2v_homonuclear_shell", "C2v",
    "two-orbit see-saw shell atoms",
    2, { 2, 2, 0 },
    { { 1.0, 0.0, 0.0 }, { 0.0, 0.8660254038, 0.5 }, { 0.0, 0.0, 0.0 } },
    1, { 0, 0, 0 }
  },
  {
    "pentagonal_planar_d5h_shell", "D5h",
    "five equivalent pentagonal-planar shell atoms",
    1, { 5, 0, 0 },
    { { 1.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 } }
  },
  {
    "tetrahedral_td_homonuclear_center_shell", "Td",
    "center and four tetrahedral shell atoms",
    2, { 1, 4, 0 },
    { { 0.0, 0.0, 0.0 }, { 1.0, 1.0, 1.0 }, { 0.0, 0.0, 0.0 } },
    1, { 0, 0, 0 }
  },
  {
    "square_pyramidal_c4v_homonuclear_center_shell", "C4v",
    "axial center and four square-pyramidal atoms",
    2, { 1, 4, 0 },
    { { 0.0, 0.0, 0.0 }, { 1.0, 0.0, -0.5 }, { 0.0, 0.0, 0.0 } },
    1, { 0, 0, 0 }
  },
  {
    "square_planar_d4h_homonuclear_center_shell", "D4h",
    "center and four square-planar atoms",
    2, { 1, 4, 0 },
    { { 0.0, 0.0, 0.0 }, { 1.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 } },
    1, { 0, 0, 0 }
  },
  {
    "see_saw_c2v_homonuclear", "C2v",
    "center and four see-saw atoms",
    3, { 1, 2, 2 },
    { { 0.0, 0.0, 0.0 }, { 1.0, 0.0, 0.0 }, { 0.0, 0.8660254038, 0.5 } },
    1, { 0, 0, 0 }
  },
  {
    "trigonal_bipyramidal_d3h_homonuclear_shell", "D3h",
    "trigonal-bipyramidal shell atoms",
    2, { 2, 3, 0 },
    { { 0.0, 0.0, 1.0 }, { 1.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 } },
    1, { 0, 0, 0 }
  },
  {
    "octahedral_oh_shell", "Oh",
    "six equivalent octahedral shell atoms",
    1, { 6, 0, 0 },
    { { 1.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 } }
  },
  {
    "hexagonal_planar_d6h_shell", "D6h",
    "six equivalent hexagonal-planar shell atoms",
    1, { 6, 0, 0 },
    { { 1.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 } }
  },
  {
    "pentagonal_pyramidal_c5v_homonuclear_center_shell", "C5v",
    "axial center and five pentagonal-pyramidal atoms",
    2, { 1, 5, 0 },
    { { 0.0, 0.0, 0.0 }, { 1.0, 0.0, -0.5 }, { 0.0, 0.0, 0.0 } },
    1, { 0, 0, 0 }
  },
  {
    "square_pyramidal_c4v_homonuclear", "C4v",
    "center and one axial and four basal atoms",
    3, { 1, 1, 4 },
    { { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 1.0 }, { 1.0, 0.0, 0.0 } },
    1, { 0, 0, 0 }
  },
  {
    "trigonal_bipyramidal_d3h_homonuclear", "D3h",
    "center and two axial and three equatorial atoms",
    3, { 1, 2, 3 },
    { { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 1.0 }, { 1.0, 0.0, 0.0 } },
    1, { 0, 0, 0 }
  },
  {
    "octahedral_oh_homonuclear_center_shell", "Oh",
    "center and six octahedral shell atoms",
    2, { 1, 6, 0 },
    { { 0.0, 0.0, 0.0 }, { 1.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 } },
    1, { 0, 0, 0 }
  },
  {
    "hexagonal_pyramidal_c6v_homonuclear_center_shell", "C6v",
    "axial center and six hexagonal-pyramidal atoms",
    2, { 1, 6, 0 },
    { { 0.0, 0.0, 0.0 }, { 1.0, 0.0, -0.5 }, { 0.0, 0.0, 0.0 } },
    1, { 0, 0, 0 }
  },
  {
    "cube_oh_shell", "Oh",
    "eight equivalent cube-corner shell atoms",
    1, { 8, 0, 0 },
    { { 1.0, 1.0, 1.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 } }
  },
  {
    "cube_oh_homonuclear_center_shell", "Oh",
    "center and eight cube-corner atoms",
    2, { 1, 8, 0 },
    { { 0.0, 0.0, 0.0 }, { 1.0, 1.0, 1.0 }, { 0.0, 0.0, 0.0 } },
    1, { 0, 0, 0 }
  },
  {
    "pentagonal_prismatic_d5h_shell", "D5h",
    "ten equivalent pentagonal-prismatic shell atoms",
    1, { 10, 0, 0 },
    { { 1.0, 0.0, 0.5 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 } }
  },
  {
    "pentagonal_antiprismatic_d5d_shell", "D5d",
    "ten equivalent pentagonal-antiprismatic shell atoms",
    1, { 10, 0, 0 },
    { { 0.9510565163, 0.3090169944, 0.5 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 } }
  },
  {
    "pentagonal_prismatic_d5h_homonuclear_center_shell", "D5h",
    "center and ten pentagonal-prismatic atoms",
    2, { 1, 10, 0 },
    { { 0.0, 0.0, 0.0 }, { 1.0, 0.0, 0.5 }, { 0.0, 0.0, 0.0 } },
    1, { 0, 0, 0 }
  },
  {
    "pentagonal_antiprismatic_d5d_homonuclear_center_shell", "D5d",
    "center and ten pentagonal-antiprismatic atoms",
    2, { 1, 10, 0 },
    { { 0.0, 0.0, 0.0 }, { 0.9510565163, 0.3090169944, 0.5 }, { 0.0, 0.0, 0.0 } },
    1, { 0, 0, 0 }
  },
  {
    "cuboctahedral_oh_shell", "Oh",
    "twelve equivalent cuboctahedral shell atoms",
    1, { 12, 0, 0 },
    { { 1.0, 1.0, 0.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 } }
  },
  {
    "icosahedral_ih_shell", "Ih",
    "twelve equivalent icosahedral shell atoms",
    1, { 12, 0, 0 },
    { { 1.6180339887, 1.0, 0.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 } }
  },
  {
    "cuboctahedral_oh_homonuclear_center_shell", "Oh",
    "center and twelve cuboctahedral atoms",
    2, { 1, 12, 0 },
    { { 0.0, 0.0, 0.0 }, { 1.0, 1.0, 0.0 }, { 0.0, 0.0, 0.0 } },
    1, { 0, 0, 0 }
  },
  {
    "icosahedral_ih_homonuclear_center_shell", "Ih",
    "center and twelve icosahedral atoms",
    2, { 1, 12, 0 },
    { { 0.0, 0.0, 0.0 }, { 1.6180339887, 1.0, 0.0 }, { 0.0, 0.0, 0.0 } },
    1, { 0, 0, 0 }
  },
  {
    "dodecahedral_ih_shell", "Ih",
    "twenty equivalent dodecahedral shell atoms",
    1, { 20, 0, 0 },
    { { 1.0, 1.0, 1.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 } }
  },
  {
    "dodecahedral_ih_homonuclear_center_shell", "Ih",
    "center and twenty dodecahedral atoms",
    2, { 1, 20, 0 },
    { { 0.0, 0.0, 0.0 }, { 1.0, 1.0, 1.0 }, { 0.0, 0.0, 0.0 } },
    1, { 0, 0, 0 }
  },
  {
    "icosidodecahedral_ih_shell", "Ih",
    "thirty equivalent icosidodecahedral shell atoms",
    1, { 30, 0, 0 },
    { { 0.0, 0.0, 1.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 } }
  },
  {
    "icosidodecahedral_ih_homonuclear_center_shell", "Ih",
    "center and thirty icosidodecahedral atoms",
    2, { 1, 30, 0 },
    { { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 1.0 }, { 0.0, 0.0, 0.0 } },
    1, { 0, 0, 0 }
  },
  {
    "linear_2_hetero", "C2v",
    "two distinct atoms fixed on the principal axis",
    2, { 1, 1, 0 },
    { { 0.0, 0.0, -1.0 }, { 0.0, 0.0, 1.0 }, { 0.0, 0.0, 0.0 } }
  },
  {
    "linear_d0h_center_shell", "D0h",
    "center + two equivalent linear shell atoms",
    2, { 1, 2, 0 },
    { { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 1.0 }, { 0.0, 0.0, 0.0 } }
  },
  {
    "bent_c2v_center_shell", "C2v",
    "center + two equivalent bent shell atoms",
    2, { 1, 2, 0 },
    { { 0.0, 0.0, 0.0 }, { 1.0, 0.0, 0.5 }, { 0.0, 0.0, 0.0 } }
  },
  {
    "t_shaped_c2v_shared_neighbors", "C2v",
    "center + one axial and two side atoms",
    3, { 1, 1, 2 },
    { { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 1.0 }, { 1.0, 0.0, 0.0 } },
    2, { 0, 1, 1 }
  },
  {
    "trigonal_pyramidal_c3v_center_shell", "C3v",
    "axial center + three trigonal-pyramidal shell atoms",
    2, { 1, 3, 0 },
    { { 0.0, 0.0, 0.0 }, { 1.0, 0.0, -0.5 }, { 0.0, 0.0, 0.0 } }
  },
  {
    "trigonal_planar_d3h_center_shell", "D3h",
    "center + three trigonal-planar shell atoms",
    2, { 1, 3, 0 },
    { { 0.0, 0.0, 0.0 }, { 1.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 } }
  },
  {
    "tetrahedral_td_center_shell", "Td",
    "center + four equivalent tetrahedral shell atoms",
    2, { 1, 4, 0 },
    { { 0.0, 0.0, 0.0 }, { 1.0, 1.0, 1.0 }, { 0.0, 0.0, 0.0 } }
  },
  {
    "square_pyramidal_c4v_center_shell", "C4v",
    "axial center + four square-pyramidal shell atoms",
    2, { 1, 4, 0 },
    { { 0.0, 0.0, 0.0 }, { 1.0, 0.0, -0.5 }, { 0.0, 0.0, 0.0 } }
  },
  {
    "square_planar_d4h_center_shell", "D4h",
    "center + four square-planar shell atoms",
    2, { 1, 4, 0 },
    { { 0.0, 0.0, 0.0 }, { 1.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 } }
  },
  {
    "trigonal_pyramidal_c3v_shared_neighbors", "C3v",
    "center + one axial and three basal atoms",
    3, { 1, 1, 3 },
    { { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 1.0 }, { 1.0, 0.0, 0.0 } },
    2, { 0, 1, 1 }
  },
  {
    "see_saw_c2v_shared_neighbors", "C2v",
    "center + four see-saw atoms",
    3, { 1, 2, 2 },
    { { 0.0, 0.0, 0.0 }, { 1.0, 0.0, 0.0 }, { 0.0, 0.8660254038, 0.5 } },
    2, { 0, 1, 1 }
  },
  {
    "pentagonal_pyramidal_c5v_center_shell", "C5v",
    "axial center + five pentagonal-pyramidal shell atoms",
    2, { 1, 5, 0 },
    { { 0.0, 0.0, 0.0 }, { 1.0, 0.0, -0.5 }, { 0.0, 0.0, 0.0 } }
  },
  {
    "square_pyramidal_c4v_shared_neighbors", "C4v",
    "center + one axial and four basal atoms",
    3, { 1, 1, 4 },
    { { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 1.0 }, { 1.0, 0.0, 0.0 } },
    2, { 0, 1, 1 }
  },
  {
    "trigonal_bipyramidal_d3h_shared_neighbors", "D3h",
    "center + two axial and three equatorial atoms",
    3, { 1, 2, 3 },
    { { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 1.0 }, { 1.0, 0.0, 0.0 } },
    2, { 0, 1, 1 }
  },
  {
    "octahedral_oh_center_shell", "Oh",
    "center + six equivalent octahedral shell atoms",
    2, { 1, 6, 0 },
    { { 0.0, 0.0, 0.0 }, { 1.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 } }
  },
  {
    "hexagonal_pyramidal_c6v_center_shell", "C6v",
    "axial center + six hexagonal-pyramidal shell atoms",
    2, { 1, 6, 0 },
    { { 0.0, 0.0, 0.0 }, { 1.0, 0.0, -0.5 }, { 0.0, 0.0, 0.0 } }
  },
  {
    "pentagonal_pyramidal_c5v_shared_neighbors", "C5v",
    "center + one axial and five basal atoms",
    3, { 1, 1, 5 },
    { { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 1.0 }, { 1.0, 0.0, 0.0 } },
    2, { 0, 1, 1 }
  },
  {
    "square_bipyramidal_d4h_shared_neighbors", "D4h",
    "center + two axial and four equatorial atoms",
    3, { 1, 2, 4 },
    { { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 1.0 }, { 1.0, 0.0, 0.0 } },
    2, { 0, 1, 1 }
  },
  {
    "hexagonal_pyramidal_c6v_shared_neighbors", "C6v",
    "center + one axial and six basal atoms",
    3, { 1, 1, 6 },
    { { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 1.0 }, { 1.0, 0.0, 0.0 } },
    2, { 0, 1, 1 }
  },
  {
    "pentagonal_bipyramidal_d5h_shared_neighbors", "D5h",
    "center + two axial and five equatorial atoms",
    3, { 1, 2, 5 },
    { { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 1.0 }, { 1.0, 0.0, 0.0 } },
    2, { 0, 1, 1 }
  },
  {
    "cube_oh_center_shell", "Oh",
    "center + eight cube-corner shell atoms",
    2, { 1, 8, 0 },
    { { 0.0, 0.0, 0.0 }, { 1.0, 1.0, 1.0 }, { 0.0, 0.0, 0.0 } }
  },
  {
    "hexagonal_bipyramidal_d6h_shared_neighbors", "D6h",
    "center + two axial and six equatorial atoms",
    3, { 1, 2, 6 },
    { { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 1.0 }, { 1.0, 0.0, 0.0 } },
    2, { 0, 1, 1 }
  },
  {
    "pentagonal_prismatic_d5h_center_shell", "D5h",
    "center + ten pentagonal-prismatic shell atoms",
    2, { 1, 10, 0 },
    { { 0.0, 0.0, 0.0 }, { 1.0, 0.0, 0.5 }, { 0.0, 0.0, 0.0 } }
  },
  {
    "pentagonal_antiprismatic_d5d_center_shell", "D5d",
    "center + ten pentagonal-antiprismatic shell atoms",
    2, { 1, 10, 0 },
    { { 0.0, 0.0, 0.0 }, { 0.9510565163, 0.3090169944, 0.5 }, { 0.0, 0.0, 0.0 } }
  },
  {
    "cuboctahedral_oh_center_shell", "Oh",
    "center + twelve cuboctahedral shell atoms",
    2, { 1, 12, 0 },
    { { 0.0, 0.0, 0.0 }, { 1.0, 1.0, 0.0 }, { 0.0, 0.0, 0.0 } }
  },
  {
    "icosahedral_ih_center_shell", "Ih",
    "center + twelve icosahedral shell atoms",
    2, { 1, 12, 0 },
    { { 0.0, 0.0, 0.0 }, { 1.6180339887, 1.0, 0.0 }, { 0.0, 0.0, 0.0 } }
  },
  {
    "dodecahedral_ih_center_shell", "Ih",
    "center + twenty dodecahedral shell atoms",
    2, { 1, 20, 0 },
    { { 0.0, 0.0, 0.0 }, { 1.0, 1.0, 1.0 }, { 0.0, 0.0, 0.0 } }
  },
  {
    "icosidodecahedral_ih_center_shell", "Ih",
    "center + thirty icosidodecahedral shell atoms",
    2, { 1, 30, 0 },
    { { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 1.0 }, { 0.0, 0.0, 0.0 } }
  },
  {
    "see_saw_c2v_shell", "C2v",
    "two-species two-orbit see-saw shell atoms",
    2, { 2, 2, 0 },
    { { 1.0, 0.0, 0.0 }, { 0.0, 0.8660254038, 0.5 }, { 0.0, 0.0, 0.0 } }
  },
  {
    "trigonal_bipyramidal_d3h_shell", "D3h",
    "two axial + three equatorial atoms",
    2, { 2, 3, 0 },
    { { 0.0, 0.0, 1.0 }, { 1.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 } }
  },
  {
    "square_bipyramidal_d4h_shell", "D4h",
    "two axial + four equatorial atoms",
    2, { 2, 4, 0 },
    { { 0.0, 0.0, 1.0 }, { 1.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 } }
  },
  {
    "pentagonal_bipyramidal_d5h_shell", "D5h",
    "two axial + five equatorial atoms",
    2, { 2, 5, 0 },
    { { 0.0, 0.0, 1.0 }, { 1.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 } }
  },
  {
    "hexagonal_bipyramidal_d6h_shell", "D6h",
    "two axial + six equatorial atoms",
    2, { 2, 6, 0 },
    { { 0.0, 0.0, 1.0 }, { 1.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 } }
  },
  {
    "t_shaped_c2v", "C2v",
    "center + one axial + two side atoms",
    3, { 1, 1, 2 },
    { { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 1.0 }, { 1.0, 0.0, 0.0 } }
  },
  {
    "trigonal_pyramidal_c3v", "C3v",
    "center + one axial + three equivalent basal atoms",
    3, { 1, 1, 3 },
    { { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 1.0 }, { 1.0, 0.0, 0.0 } }
  },
  {
    "square_pyramidal_c4v", "C4v",
    "center + one axial + four basal atoms",
    3, { 1, 1, 4 },
    { { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 1.0 }, { 1.0, 0.0, 0.0 } }
  },
  {
    "pentagonal_pyramidal_c5v", "C5v",
    "center + one axial + five basal atoms",
    3, { 1, 1, 5 },
    { { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 1.0 }, { 1.0, 0.0, 0.0 } }
  },
  {
    "hexagonal_pyramidal_c6v", "C6v",
    "center + one axial + six equivalent basal atoms",
    3, { 1, 1, 6 },
    { { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 1.0 }, { 1.0, 0.0, 0.0 } }
  },
  {
    "see_saw_c2v", "C2v",
    "center + two axial + two side atoms",
    3, { 1, 2, 2 },
    { { 0.0, 0.0, 0.0 }, { 1.0, 0.0, 0.0 }, { 0.0, 0.8660254038, 0.5 } }
  },
  {
    "trigonal_bipyramidal_d3h", "D3h",
    "center + two axial + three equatorial atoms",
    3, { 1, 2, 3 },
    { { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 1.0 }, { 1.0, 0.0, 0.0 } }
  },
  {
    "square_bipyramidal_d4h", "D4h",
    "center + two axial + four equatorial atoms",
    3, { 1, 2, 4 },
    { { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 1.0 }, { 1.0, 0.0, 0.0 } }
  },
  {
    "pentagonal_bipyramidal_d5h", "D5h",
    "center + two axial + five equatorial atoms",
    3, { 1, 2, 5 },
    { { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 1.0 }, { 1.0, 0.0, 0.0 } }
  },
  {
    "hexagonal_bipyramidal_d6h", "D6h",
    "center + two axial + six equatorial atoms",
    3, { 1, 2, 6 },
    { { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 1.0 }, { 1.0, 0.0, 0.0 } }
  },
};

unsigned int templateAtomCount(const MoleculeTemplate& moleculeTemplate)
{
  unsigned int count = 0;
  for (unsigned int i = 0; i < moleculeTemplate.orbitCount; ++i)
    count += moleculeTemplate.orbitSizes[i];
  return count;
}

unsigned int templateOrbitSpecies(const MoleculeTemplate& moleculeTemplate, unsigned int orbit)
{
  return moleculeTemplate.speciesCount == 0
           ? orbit
           : moleculeTemplate.orbitSpecies[orbit];
}

bool templateSpeciesAtomCounts(const MoleculeTemplate& moleculeTemplate,
                               std::vector<unsigned int>& counts)
{
  const unsigned int speciesCount = moleculeTemplate.speciesCount == 0 ? moleculeTemplate.orbitCount
                                       : moleculeTemplate.speciesCount;
  if (moleculeTemplate.orbitCount == 0 || moleculeTemplate.orbitCount > 3 ||
      speciesCount == 0 || speciesCount > 3) {
    return false;
  }

  counts.assign(speciesCount, 0);
  for (unsigned int orbit = 0; orbit < moleculeTemplate.orbitCount; ++orbit) {
    const unsigned int species = templateOrbitSpecies(moleculeTemplate, orbit);
    if (species >= speciesCount)
      return false;
    counts[species] += moleculeTemplate.orbitSizes[orbit];
  }
  return true;
}

QString speciesLabel(unsigned int species)
{
  const char label[2] = {
    static_cast<char>('A' + species),
    '\0'
  };
  return QString::fromLatin1(label);
}

QString templateSpeciesPattern(const MoleculeTemplate& moleculeTemplate)
{
  QStringList parts;
  for (unsigned int orbit = 0; orbit < moleculeTemplate.orbitCount; ++orbit) {
    parts << QString("%1%2")
               .arg(speciesLabel(templateOrbitSpecies(moleculeTemplate, orbit)))
               .arg(moleculeTemplate.orbitSizes[orbit]);
  }
  return QString("[%1]").arg(parts.join(","));
}

QString templateFormulaPattern(const MoleculeTemplate& moleculeTemplate)
{
  std::vector<unsigned int> counts;
  if (!templateSpeciesAtomCounts(moleculeTemplate, counts))
    return QString();

  QString formula;
  for (size_t i = 0; i < counts.size(); ++i)
    formula += QString("%1%2").arg(speciesLabel(static_cast<unsigned int>(i)))
                              .arg(counts[i]);
  return formula;
}

void setTemplateInfo(MoleculeTemplateInfo& info, const MoleculeTemplate& moleculeTemplate)
{
  info.name = QString::fromLatin1(moleculeTemplate.name);
  info.pointGroup = QString::fromLatin1(moleculeTemplate.pointGroup);
  info.speciesPattern = templateSpeciesPattern(moleculeTemplate);
  info.formulaPattern = templateFormulaPattern(moleculeTemplate);
  info.description = QString::fromLatin1(moleculeTemplate.description);
  for (unsigned int orbit = 0; orbit < moleculeTemplate.orbitCount; ++orbit)
    info.orbitSizes.push_back(moleculeTemplate.orbitSizes[orbit]);
}

void findTemplates(const std::map<unsigned int, unsigned int>& composition,
                   std::vector<const MoleculeTemplate*>& templates,
                   std::vector<std::vector<unsigned int> >& orbitAssignments)
{
  if (composition.empty() || composition.size() > 3)
    return;

  for (size_t i = 0; i < sizeof(moleculeTemplates) / sizeof(moleculeTemplates[0]); ++i) {
    const MoleculeTemplate& candidate = moleculeTemplates[i];
    std::vector<unsigned int> speciesAtomCounts;

    if (!templateSpeciesAtomCounts(candidate, speciesAtomCounts))
      continue;
    if (composition.size() != speciesAtomCounts.size())
      continue;

    bool used[3] = { false, false, false };
    std::vector<unsigned int> speciesAtomicNums(speciesAtomCounts.size(), 0);
    for (std::map<unsigned int, unsigned int>::const_iterator it = composition.begin();
         it != composition.end(); ++it) {
      bool matched = false;
      for (size_t species = 0; species < speciesAtomCounts.size(); ++species) {
        if (!used[species] && it->second == speciesAtomCounts[species]) {
          used[species] = true;
          speciesAtomicNums[species] = it->first;
          matched = true;
          break;
        }
      }
      if (!matched) {
        speciesAtomicNums.clear();
        break;
      }
    }

    if (!speciesAtomicNums.empty()) {
      std::vector<unsigned int> assignment(candidate.orbitCount, 0);
      for (unsigned int orbit = 0; orbit < candidate.orbitCount; ++orbit) {
        assignment[orbit] = speciesAtomicNums[templateOrbitSpecies(candidate, orbit)];
      }
      templates.push_back(&candidate);
      orbitAssignments.push_back(assignment);
    }
  }
}

QString compositionString(const std::map<unsigned int, unsigned int>& composition)
{
  QString text;
  for (std::map<unsigned int, unsigned int>::const_iterator it = composition.begin();
       it != composition.end(); ++it) {
    text += QString::fromStdString(ElementInfo::getAtomicSymbol(it->first));
    text += QString::number(it->second);
  }
  return text;
}

bool readCompositionString(const std::string& formula,
                           std::map<unsigned int, unsigned int>& composition, QString& error)
{
  const QString text = QString::fromLocal8Bit(formula.c_str()).simplified();
  if (text.isEmpty()) {
    error = "Molecule composition cannot be empty.";
    return false;
  }
  if (text.split(' ', QtCompat::SkipEmptyParts).size() != 1) {
    error = "Molecule composition expects a single formula.";
    return false;
  }
  if (!ElementInfo::readComposition(text.toStdString(), composition)) {
    error = QString("Invalid molecule composition: %1").arg(text);
    return false;
  }
  return true;
}

void setMsymElement(msym_element_t& element, unsigned int atomicNum, const double position[3])
{
  const std::string symbol = ElementInfo::getAtomicSymbol(atomicNum);
  std::memset(&element, 0, sizeof(element));
  element.n = static_cast<int>(atomicNum);
  element.m = ElementInfo::getAtomicMass(atomicNum);
  element.v[0] = position[0];
  element.v[1] = position[1];
  element.v[2] = position[2];
  std::snprintf(element.name, sizeof(element.name), "%s", symbol.c_str());
}

QString libmsymError(msym_error_t error)
{
  const char* details = msymGetErrorDetails();
  if (details && details[0] != '\0')
    return QString::fromLocal8Bit(details);

  const char* errorString = msymErrorString(error);
  return errorString && errorString[0] != '\0'
           ? QString::fromLocal8Bit(errorString)
           : QString("Unknown libmsym error");
}

double covalentScaleFactor(const std::vector<msym_element_t>& elements, double scaleFactor)
{
  if (elements.size() < 2)
    return 1.0;

  double maxPairScale = 0.0;
  for (size_t i = 0; i < elements.size(); ++i) {
    double nearestDistance = std::numeric_limits<double>::infinity();
    std::vector<size_t> nearestNeighbors;
    const Common::Vector3 posI(elements[i].v[0], elements[i].v[1], elements[i].v[2]);

    for (size_t j = 0; j < elements.size(); ++j) {
      if (i == j)
        continue;

      const Common::Vector3 posJ(elements[j].v[0], elements[j].v[1], elements[j].v[2]);
      const double distance = (posI - posJ).norm();
      if (distance <= ZERO08)
        continue;

      if (distance < nearestDistance - ZERO08) {
        nearestDistance = distance;
        nearestNeighbors.clear();
        nearestNeighbors.push_back(j);
      } else if (Common::eq(distance, nearestDistance, ZERO08)) {
        nearestNeighbors.push_back(j);
      }
    }

    if (!GS_ISFINITE(nearestDistance))
      continue;

    for (size_t neighborIndex = 0; neighborIndex < nearestNeighbors.size(); ++neighborIndex) {
      const size_t j = nearestNeighbors[neighborIndex];
      const double targetDistance = scaleFactor *
        (ElementInfo::getCovalentRadius(static_cast<unsigned int>(elements[i].n)) +
         ElementInfo::getCovalentRadius(static_cast<unsigned int>(elements[j].n)));
      if (targetDistance > 0.0)
        maxPairScale = std::max(maxPairScale, targetDistance / nearestDistance);
    }
  }

  return maxPairScale > 0.0 ? maxPairScale : 1.0;
}

void warnIfPointGroupMismatch(const MoleculeTemplate& moleculeTemplate, int length,
                              msym_element_t* elements)
{
  msym_context checkCtx = msymCreateContext();
  if (!checkCtx) {
    Common::warning(
      QString("Molecule template %1 point-group check failed.")
        .arg(QString::fromLatin1(moleculeTemplate.name)));
    return;
  }

  msym_error_t ret = msymSetElements(checkCtx, length, elements);
  if (ret == MSYM_SUCCESS)
    ret = msymFindSymmetry(checkCtx);

  char detectedName[32] = { '\0' };
  if (ret == MSYM_SUCCESS)
    ret = msymGetPointGroupName(checkCtx, sizeof(detectedName), detectedName);

  if (ret != MSYM_SUCCESS) {
    Common::warning(
      QString("Molecule template %1 point-group check failed: %2")
        .arg(QString::fromLatin1(moleculeTemplate.name))
        .arg(libmsymError(ret)));
    msymReleaseContext(checkCtx);
    return;
  }

  const QString detected = QString::fromLatin1(detectedName);
  const QString requested = QString::fromLatin1(moleculeTemplate.pointGroup);
  if (detected != requested) {
    Common::warning(
      QString("Molecule template %1 requested %2; libmsym detected %3.")
        .arg(QString::fromLatin1(moleculeTemplate.name))
        .arg(requested)
        .arg(detected));
  }

  msymReleaseContext(checkCtx);
}


} // namespace

static bool buildMoleculeFromTemplate(const std::map<unsigned int, unsigned int>& composition,
  const QString& selectedTemplate, Geometry& molecule, QString& error, double scaleFactor)
{
  // Make a molecule from a template.

  molecule = Geometry();
  if (composition.empty()) {
    error = "Molecule composition cannot be empty.";
    return false;
  }
  if (selectedTemplate.isEmpty()) {
    error = "Molecule template cannot be empty.";
    return false;
  }
  if (scaleFactor <= 0.0) {
    error = "Molecule scale factor must be positive.";
    return false;
  }

  std::vector<const MoleculeTemplate*> templates;
  std::vector<std::vector<unsigned int> > orbitAssignments;
  findTemplates(composition, templates, orbitAssignments);
  if (templates.empty()) {
    error = QString("No molecule template supports composition: %1")
              .arg(compositionString(composition));
    return false;
  }

  for (size_t templateIndex = 0; templateIndex < templates.size(); ++templateIndex) {
    const MoleculeTemplate* moleculeTemplate = templates[templateIndex];
    if (QString::fromLatin1(moleculeTemplate->name) != selectedTemplate)
      continue;

    const std::vector<unsigned int>& orbitAtomicNums = orbitAssignments[templateIndex];

    msym_context ctx = msymCreateContext();
    if (!ctx) {
      error = "Failed to create libmsym context.";
      return false;
    }

    msym_error_t ret = MSYM_SUCCESS;
    std::vector<msym_element_t> seed(moleculeTemplate->orbitCount);
    double origin[3] = { 0.0, 0.0, 0.0 };

    for (unsigned int orbit = 0; orbit < moleculeTemplate->orbitCount; ++orbit)
      setMsymElement(seed[orbit], orbitAtomicNums[orbit], moleculeTemplate->orbitSeeds[orbit]);

    if (MSYM_SUCCESS != (ret = msymSetElements(ctx, static_cast<int>(seed.size()), seed.data()))) {
      error = libmsymError(ret);
      msymReleaseContext(ctx);
      return false;
    }
    if (MSYM_SUCCESS != (ret = msymSetCenterOfMass(ctx, origin))) {
      error = libmsymError(ret);
      msymReleaseContext(ctx);
      return false;
    }
    if (MSYM_SUCCESS != (ret = msymSetPointGroupByName(ctx, moleculeTemplate->pointGroup))) {
      error = libmsymError(ret);
      msymReleaseContext(ctx);
      return false;
    }
    if (MSYM_SUCCESS != (ret = msymGenerateElements(ctx, static_cast<int>(seed.size()),
                                                    seed.data()))) {
      error = libmsymError(ret);
      msymReleaseContext(ctx);
      return false;
    }

    int length = 0;
    msym_element_t* elements = nullptr;
    if (MSYM_SUCCESS != (ret = msymGetElements(ctx, &length, &elements))) {
      error = libmsymError(ret);
      msymReleaseContext(ctx);
      return false;
    }

    if (length != static_cast<int>(templateAtomCount(*moleculeTemplate))) {
      error = QString("Molecule template %1 generated an unexpected atom count.")
                .arg(QString::fromLatin1(moleculeTemplate->name));
      msymReleaseContext(ctx);
      return false;
    }
    warnIfPointGroupMismatch(*moleculeTemplate, length, elements);

    std::vector<msym_element_t> generated(elements, elements + static_cast<size_t>(length));
    const double internalScale = covalentScaleFactor(generated, scaleFactor);

    std::vector<Atom> moleculeAtoms;
    moleculeAtoms.reserve(static_cast<size_t>(length));
    for (int i = 0; i < length; ++i) {
      const Common::Vector3 position = internalScale * Common::Vector3(elements[i].v[0],
                                        elements[i].v[1], elements[i].v[2]);
      moleculeAtoms.push_back(Atom(static_cast<unsigned short>(elements[i].n), position));
    }
    molecule.setAtoms(moleculeAtoms);

    msymReleaseContext(ctx);
    error.clear();
    Q_ASSERT(molecule.is0D());
    return true;
  }

  error = QString("Molecule template '%1' does not match composition: %2")
            .arg(selectedTemplate)
            .arg(compositionString(composition));
  return false;
}

bool buildMoleculeFromFormula(const std::string& formula, const std::string& templateName,
                              Geometry& molecule, QString& error, double scaleFactor)
{
  // Make a molecule from a formula and template.

  error.clear();
  molecule = Geometry();

  const QString selectedTemplate = QString::fromLocal8Bit(templateName.c_str()).simplified();
  if (selectedTemplate.isEmpty()) {
    error = "Molecule template cannot be empty.";
    return false;
  }
  if (selectedTemplate.split(' ', QtCompat::SkipEmptyParts).size() != 1) {
    error = "Molecule template expects a single template name.";
    return false;
  }

  std::map<unsigned int, unsigned int> composition;
  if (!readCompositionString(formula, composition, error))
    return false;

  return buildMoleculeFromTemplate(composition, selectedTemplate, molecule, error, scaleFactor);
}

bool buildMoleculeFromCartesianString(const std::string& text, Geometry& molecule, QString& error)
{
  // Read the atom symbols and positions.

  error.clear();
  molecule = Geometry();

  std::vector<Atom> moleculeAtoms;
  for (const std::string& s : Common::split(text, ',', false)) {
    std::istringstream atomStream(s);
    std::string element;
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    if (!(atomStream >> element >> x >> y >> z)) {
      error = QString("Cannot parse molecule atom entry: '%1'.")
                .arg(QString::fromLocal8Bit(s.c_str()));
      return false;
    }

    const unsigned int atomicNumber = ElementInfo::getAtomicNum(element);
    if (atomicNumber == 0) {
      error = QString("Unknown atomic symbol in molecule: '%1'.")
                .arg(QString::fromLocal8Bit(element.c_str()));
      return false;
    }

    moleculeAtoms.push_back(
      Atom(static_cast<unsigned short>(atomicNumber), Common::Vector3(x, y, z)));
  }

  if (moleculeAtoms.empty()) {
    error = "No atoms were parsed from the molecule string.";
    return false;
  }

  molecule.setAtoms(moleculeAtoms);
  Q_ASSERT(molecule.is0D());
  return true;
}

std::vector<MoleculeTemplateInfo> moleculeTemplatesForFormula(const std::string& formula)
{
  QString error;
  std::map<unsigned int, unsigned int> composition;
  if (!readCompositionString(formula, composition, error))
    return std::vector<MoleculeTemplateInfo>();

  std::vector<const MoleculeTemplate*> templates;
  std::vector<std::vector<unsigned int> > orbitAssignments;
  findTemplates(composition, templates, orbitAssignments);

  std::vector<MoleculeTemplateInfo> infos;
  infos.reserve(templates.size());
  for (size_t i = 0; i < templates.size(); ++i) {
    const MoleculeTemplate* moleculeTemplate = templates[i];
    MoleculeTemplateInfo info;
    setTemplateInfo(info, *moleculeTemplate);
    infos.push_back(info);
  }
  return infos;
}

std::vector<MoleculeTemplateInfo> moleculeTemplatesCatalog()
{
  std::vector<MoleculeTemplateInfo> templates;
  const size_t templateCount = sizeof(moleculeTemplates) / sizeof(moleculeTemplates[0]);
  templates.reserve(templateCount);
  for (size_t i = 0; i < templateCount; ++i) {
    MoleculeTemplateInfo info;
    setTemplateInfo(info, moleculeTemplates[i]);
    templates.push_back(info);
  }
  return templates;
}

QString moleculeTemplateCatalogText()
{
  const std::vector<MoleculeTemplateInfo> templates = moleculeTemplatesCatalog();

  QString text;
  QTextStream out(&text);
  int templateWidth = QString("Template").size();
  int speciesWidth = QString("Species").size();
  int formulaWidth = QString("Formula").size();
  for (size_t i = 0; i < templates.size(); ++i)
    templateWidth = std::max<int>(templateWidth, templates[i].name.size());
  for (size_t i = 0; i < templates.size(); ++i) {
    speciesWidth = std::max<int>(speciesWidth, templates[i].speciesPattern.size());
    formulaWidth = std::max<int>(formulaWidth, templates[i].formulaPattern.size());
  }

  // Set the column order.
  out << QString("%1 %2  %3  %4  %5\n")
           .arg("Formula", -formulaWidth)
           .arg("Template", -templateWidth)
           .arg("Species", -speciesWidth)
           .arg("PG", -3)
           .arg("Description");
  out << QString(templateWidth + 3 + speciesWidth + formulaWidth + 18, QChar('-')) << "\n";
  for (size_t i = 0; i < templates.size(); ++i) {
    out << QString("%1 %2  %3  %4  %5\n")
             .arg(templates[i].formulaPattern, -formulaWidth)
             .arg(templates[i].name, -templateWidth)
             .arg(templates[i].speciesPattern, -speciesWidth)
             .arg(templates[i].pointGroup, -3)
             .arg(templates[i].description);
  }

  return text;
}

} // namespace Atoms
