/**********************************************************************
  XtalOptDebug - Some helpful tools for debugging XtalOpt

  Copyright (C) 2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef XTALOPTDEBUG_H
#define XTALOPTDEBUG_H

class QString;

namespace XtalOpt {
class Xtal;
}

namespace XtalOptDebug {

void dumpPseudoPwscfOut(const XtalOpt::Xtal* xtal, const QString& filename);
}

#endif // XTALOPTDEBUG_H
