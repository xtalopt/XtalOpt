/**********************************************************************
  functionTracker.h - Class for tracking which functions are being called!
                      Recommended use: define a macro like the following:
                 #define START_FT FunctionTracker functionTracker(__FUNCTION__);
                      at the beginning of the .cpp file and just add the macro
                      at the beginning of each function call. It will print
                      print a message with the name of the function when it
                      is created, and it will print that the function is
                      ending when it is destroyed.

  Copyright (C) 2015 - 2016 by Patrick S. Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

 ***********************************************************************/

#ifndef FUNCTION_TRACKER_H
#define FUNCTION_TRACKER_H

#include <iostream>

// Define START_FT as such in the files in which you want to use this:
// #define START_FT FunctionTracker functionTracker(__FUNCTION__);

class FunctionTracker {
 public:
  FunctionTracker(std::string functionName) :
    funcName(functionName)
  {
    std::cout << funcName << " called!\n";
  };

  virtual ~FunctionTracker()
  {
    std::cout << funcName << " ending!\n";
  };
 private:
  std::string funcName;
};

#endif
