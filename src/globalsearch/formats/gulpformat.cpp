/**********************************************************************
  GulpFormat -- A simple reader for GULP output.

  Copyright (C) 2016 by Patrick Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <globalsearch/formats/gulpformat.h>
#include <globalsearch/structure.h>

#include <QtCore/QDebug>
#include <QtCore/QString>

namespace GlobalSearch {

  bool GulpFormat::read(Structure* s, const QString& filename)
  {

    return false;
    // We need to do something like the following section to update
    // the structure once we have read all of the information.
    /*
    if (m_opt->isStarting) {
      structure->updateAndSkipHistory(atomicNums, coords,
                                      energy, enthalpy, cellMat);
    }
    else {
      structure->updateAndAddToHistory(atomicNums, coords,
                                       energy, enthalpy, cellMat);
    }
    */
  }

}
