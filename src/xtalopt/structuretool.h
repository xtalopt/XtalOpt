/**********************************************************************
  structuretool - Collection of structure and molecule command-line tools

  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef STRUCTURETOOL_RUNNER_H
#define STRUCTURETOOL_RUNNER_H

class QTextStream;

namespace StructureTool {

// Return true if argv selects the structuretool invocation.
bool isInvocation(int argc, char* argv[]);

void printHelp(QTextStream& out);

int run(int argc, char* argv[]);

} // namespace StructureTool

#endif
