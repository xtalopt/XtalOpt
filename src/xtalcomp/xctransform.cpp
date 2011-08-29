/**********************************************************************
  XcTransform - Transformation class for transforming XcVectors

  WARNING: This is not your typical transform class -- it has been
  specialized for XtalComp. It stores the rotation and translation
  separately, and applies the translation followed by the rotation
  when multiplied by a vector. It may not work for you needs.

  Copyright (C) 2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

// This is where the XcTransform statics live:

#include "xctransform.h"

XcVector XcTransform::ZeroVector (0.0, 0.0, 0.0);

XcMatrix XcTransform::IdentityMatrix (1.0, 0.0, 0.0,
                                      0.0, 1.0, 0.0,
                                      0.0, 0.0, 1.0);
