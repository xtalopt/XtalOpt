/**********************************************************************
  RandomDock - Scene: Wrapper for Avogadro::Molecule to hold the
  central molecule and matrix elements in a docking problem

  Copyright (C) 2009-2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <randomdock/structures/scene.h>

#include <QDebug>

using namespace std;
using namespace Avogadro;

namespace RandomDock {

Scene::Scene(QObject* parent) : Structure(parent)
{
}

Scene::~Scene()
{
}

} // end namespace RandomDock
