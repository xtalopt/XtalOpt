/**********************************************************************
  MtpFormat -- A simple reader for MTP output.

  Copyright (C) 2025 by Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef MTPFORMAT_H
#define MTPFORMAT_H

// Forward declaration
class QString;

namespace GlobalSearch {

// Forward declaration
class Structure;

/**
 * @class The MTP potential format.
 */
class MtpFormat
{
public:
  static bool read(Structure* s, const QString& filename);
};
}

#endif // MTPFORMAT_H
