/**********************************************************************
  functionTracker.h - Class for tracking which functions are being called!
                      Recommended use: define a macro like the following:
                 #define START_FT FunctionTracker functionTracker(__FUNCTION__);
                      at the beginning of the .cpp file and just add the macro
                      at the beginning of each function call. It will print
                      print a message with the name of the function when it
                      is created, and it will print that the function is
                      ending when it is destroyed.

  Copyright (C) 2016 by Patrick S. Avery

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

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
