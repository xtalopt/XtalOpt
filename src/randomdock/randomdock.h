/**********************************************************************
  RandomDock

  Copyright (C) 2009-2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef RANDOMDOCK_H
#define RANDOMDOCK_H

#include <globalsearch/optbase.h>

#include <Eigen/Geometry>

#include <QDebug>
#include <QReadWriteLock>
#include <QStringList>

#include <QInputDialog>

namespace GlobalSearch {
class SlottedWaitCondition;
class Structure;
}

namespace RandomDock {
class RandomDockDialog;
class RandomDockParams;
class Scene;
class Substrate;
class Matrix;

class RandomDock : public GlobalSearch::OptBase
{
  Q_OBJECT

public:
  explicit RandomDock(RandomDockDialog* parent);
  virtual ~RandomDock();

  enum OptTypes
  {
    OT_GAMESS = 0,
    OT_ADF,
    OT_MOPAC,
    OT_GAUSSIAN
  };

  enum QueueInterfaces
  {
    QI_LOCAL = 0
#ifdef ENABLE_SSH
    ,
    QI_PBS,
    QI_SLURM,
    QI_SGE
#endif // ENABLE_SSH
  };

  Scene* generateRandomScene();
  GlobalSearch::Structure* replaceWithRandom(GlobalSearch::Structure* s,
                                             const QString& reason = "");
  bool checkLimits();

  bool checkScene(Scene* scene);
  static void sortAndRankByEnergy(QList<Scene*>* scenes);

  // TODO move to structure-derived classes, or incorporate into scene
  // generation
  static void centerCoordinatesAtOrigin(QList<Eigen::Vector3d>& coords);
  static void randomlyRotateCoordinates(QList<Eigen::Vector3d>& coords);
  static void randomlyDisplaceCoordinates(QList<Eigen::Vector3d>& coords,
                                          double radiusMin, double radiusMax);
  static void DRotateCoordinates(QList<Eigen::Vector3d>& coords);
  static void DDisplaceCoordinates(QList<Eigen::Vector3d>& coords,
                                   double radiusMin, double radiusMax);

  QString substrateFile;     // Filename of the substrate
  Substrate* substrate;      // Pointer to the substrate
  QStringList matrixFiles;   // List of filenames
  QList<Matrix*> matrixList; // List of pointers to the matrix molecules
  QList<int> matrixStoich;   // Stoichiometry of the matrix elements
  uint numMatrixMol; // Number of matrix molecules to be placed around the
                     // substrate
  double IAD_min;    // Minimum allowed interatomic distance
  double IAD_max;    // Maximum allowed interatomic distance
  double radius_min; // Minimum distance from origin to place matrix molecules
  double radius_max; // Maximum distance from origin to place matrix molecules
  bool radius_auto;  // Whether to automatically calculate the matrix radius
  bool cluster_mode;
  bool strictHBonds;
  bool build2DNetwork; // Make a 2D Network keeping the Z-coordinate constant

  QMutex* sceneInitMutex;

public slots:
  void startSearch();
  void generateNewStructure();
  void initializeAndAddScene(Scene* scene);

private:
  GlobalSearch::SlottedWaitCondition* m_initWC;
};

} // end namespace RandomDock

#endif
