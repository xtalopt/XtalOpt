/**********************************************************************
  QueueInterface - Base queue interface class implementation

  Copyright (C) 2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <globalsearch/queueinterface.h>

#include <globalsearch/optbase.h>
#include <globalsearch/optimizer.h>
#include <globalsearch/structure.h>
#include <globalsearch/structure.h>

#include <QString>
#include <QStringList>

namespace GlobalSearch {

bool QueueInterface::writeInputFiles(Structure* s) const
{
  return writeFiles(
    s, m_opt->optimizer(s->getCurrentOptStep())->getInterpretedTemplates(s));
}

} // end namespace GlobalSearch
