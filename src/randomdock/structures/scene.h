/**********************************************************************
  Scene - Contains a docked substrate in a matrix

  Copyright (C) 2009-2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef SCENEMOL_H
#define SCENEMOL_H

#include <globalsearch/structure.h>

#include <QDebug>

namespace RandomDock {
class Scene : public GlobalSearch::Structure
{
  Q_OBJECT

public:
  Scene(QObject* parent = 0);
  virtual ~Scene();

signals:

public slots:

private slots:

private:
};

} // end namespace RandomDock

#endif
