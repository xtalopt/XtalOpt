/**********************************************************************
  SlottedWaitCondition - Simple wrapper around QWaitCondition with
  wake slots.

  Copyright (C) 2010-2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <globalsearch/slottedwaitcondition.h>

namespace GlobalSearch {

SlottedWaitCondition::SlottedWaitCondition(QObject* parent)
  : QObject(parent), QWaitCondition(), m_mutex()
{
}

SlottedWaitCondition::~SlottedWaitCondition()
{
}
}
