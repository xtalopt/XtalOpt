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

#ifndef XCTRANSFORM_H
#define XCTRANSFORM_H

#include "xcvector.h"
#include "xcmatrix.h"

#include <stdio.h>

class XcTransform
{
 public:
  XcTransform() {};
  XcTransform(const XcTransform &other) : rot(other.rot), trans(other.trans) {}

  XcTransform & setIdentity()
  {
    this->rot.fillFromScalar(1.0);
    this->trans.fill(0.0);
    return *this;
  }

  static XcVector ZeroVector;
  static XcMatrix IdentityMatrix;

  XcTransform &    rotate(const XcMatrix &mat) {return this->multiplyByTransform(mat, ZeroVector);}
  XcTransform & prerotate(const XcMatrix &mat) {return this->premultiplyByTransform(mat, ZeroVector);}

  XcTransform &    translate(const XcVector &vec) {return this->multiplyByTransform(IdentityMatrix, vec);}
  XcTransform & pretranslate(const XcVector &vec) {return this->premultiplyByTransform(IdentityMatrix, vec);}

  XcTransform & multiplyByTransform(const XcMatrix & otherRot, const XcVector & otherTrans)
  {
    XcMatrix newRot;
    XcVector newTrans;
    newRot[0][0] = this->rot[0][0]*otherRot[0][0] + this->rot[0][1]*otherRot[1][0] + this->rot[0][2]*otherRot[2][0];
    newRot[0][1] = this->rot[0][0]*otherRot[0][1] + this->rot[0][1]*otherRot[1][1] + this->rot[0][2]*otherRot[2][1];
    newRot[0][2] = this->rot[0][0]*otherRot[0][2] + this->rot[0][1]*otherRot[1][2] + this->rot[0][2]*otherRot[2][2];
    newTrans[0]  = this->rot[0][0]*otherTrans[0] + this->rot[0][1]*otherTrans[1] + this->rot[0][2]*otherTrans[2] + this->trans[0];

    newRot[1][0] = this->rot[1][0]*otherRot[0][0] + this->rot[1][1]*otherRot[1][0] + this->rot[1][2]*otherRot[2][0];
    newRot[1][1] = this->rot[1][0]*otherRot[0][1] + this->rot[1][1]*otherRot[1][1] + this->rot[1][2]*otherRot[2][1];
    newRot[1][2] = this->rot[1][0]*otherRot[0][2] + this->rot[1][1]*otherRot[1][2] + this->rot[1][2]*otherRot[2][2];
    newTrans[1]  = this->rot[1][0]*otherTrans[0] + this->rot[1][1]*otherTrans[1] + this->rot[1][2]*otherTrans[2] + this->trans[1];

    newRot[2][0] = this->rot[2][0]*otherRot[0][0] + this->rot[2][1]*otherRot[1][0] + this->rot[2][2]*otherRot[2][0];
    newRot[2][1] = this->rot[2][0]*otherRot[0][1] + this->rot[2][1]*otherRot[1][1] + this->rot[2][2]*otherRot[2][1];
    newRot[2][2] = this->rot[2][0]*otherRot[0][2] + this->rot[2][1]*otherRot[1][2] + this->rot[2][2]*otherRot[2][2];
    newTrans[2]  = this->rot[2][0]*otherTrans[0] + this->rot[2][1]*otherTrans[1] + this->rot[2][2]*otherTrans[2] + this->trans[2];
    this->rot = newRot;
    this->trans = newTrans;
    return *this;
  }

  XcTransform & premultiplyByTransform(const XcMatrix & otherRot, const XcVector & otherTrans)
  {
    XcMatrix newRot;
    XcVector newTrans;
    newRot[0][0] = otherRot[0][0]*this->rot[0][0] + otherRot[0][1]*this->rot[1][0] + otherRot[0][2]*this->rot[2][0];
    newRot[0][1] = otherRot[0][0]*this->rot[0][1] + otherRot[0][1]*this->rot[1][1] + otherRot[0][2]*this->rot[2][1];
    newRot[0][2] = otherRot[0][0]*this->rot[0][2] + otherRot[0][1]*this->rot[1][2] + otherRot[0][2]*this->rot[2][2];
    newTrans[0]  = otherRot[0][0]*this->trans[0] + otherRot[0][1]*this->trans[1] + otherRot[0][2]*this->trans[2] + otherTrans[0];

    newRot[1][0] = otherRot[1][0]*this->rot[0][0] + otherRot[1][1]*this->rot[1][0] + otherRot[1][2]*this->rot[2][0];
    newRot[1][1] = otherRot[1][0]*this->rot[0][1] + otherRot[1][1]*this->rot[1][1] + otherRot[1][2]*this->rot[2][1];
    newRot[1][2] = otherRot[1][0]*this->rot[0][2] + otherRot[1][1]*this->rot[1][2] + otherRot[1][2]*this->rot[2][2];
    newTrans[1]  = otherRot[1][0]*this->trans[0] + otherRot[1][1]*this->trans[1] + otherRot[1][2]*this->trans[2] + otherTrans[1];

    newRot[2][0] = otherRot[2][0]*this->rot[0][0] + otherRot[2][1]*this->rot[1][0] + otherRot[2][2]*this->rot[2][0];
    newRot[2][1] = otherRot[2][0]*this->rot[0][1] + otherRot[2][1]*this->rot[1][1] + otherRot[2][2]*this->rot[2][1];
    newRot[2][2] = otherRot[2][0]*this->rot[0][2] + otherRot[2][1]*this->rot[1][2] + otherRot[2][2]*this->rot[2][2];
    newTrans[2]  = otherRot[2][0]*this->trans[0] + otherRot[2][1]*this->trans[1] + otherRot[2][2]*this->trans[2] + otherTrans[2];
    this->rot = newRot;
    this->trans = newTrans;
    return *this;
  }

  XcVector & translation() {return this->trans;}
  const XcVector & translation() const {return this->trans;}

  XcMatrix & rotation() {return this->rot;}
  const XcMatrix & rotation() const {return this->rot;}

  XcVector operator*(const XcVector & vec) const
  {
    XcVector ret;
    ret[0] = this->rot[0][0]*vec[0] + this->rot[0][1]*vec[1] + this->rot[0][2]*vec[2] + trans[0];
    ret[1] = this->rot[1][0]*vec[0] + this->rot[1][1]*vec[1] + this->rot[1][2]*vec[2] + trans[1];
    ret[2] = this->rot[2][0]*vec[0] + this->rot[2][1]*vec[1] + this->rot[2][2]*vec[2] + trans[2];
    return ret;
  }

 private:
  XcMatrix rot;
  XcVector trans;
};

#endif
