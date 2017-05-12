// Written by Patrick S. Avery -- 2015
// Hides the console input while the user inputs the password
// Returns the password

#include <iostream>
#include <string>

// Include Windows header for Windows
#ifdef _WIN32
#include <windows.h>
#else // Include Unix headers for Unix
#include <termios.h>
#include <unistd.h>
#endif

#include "passwordprompt.h"

std::string PasswordPrompt::getPassword(const std::string& prompt)
{
// For Windows
#ifdef _WIN32
  HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
  DWORD mode = 0;
  GetConsoleMode(hStdin, &mode);
  SetConsoleMode(hStdin, mode & (~ENABLE_ECHO_INPUT));
#else // For Unix
  termios oldt;
  tcgetattr(STDIN_FILENO, &oldt);
  termios newt = oldt;
  newt.c_lflag &= ~ECHO;
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
#endif

  std::string s;
  std::cout << prompt;
  getline(std::cin, s);
  std::cout << std::endl;

// Cleanup
#ifdef _WIN32
  SetConsoleMode(hStdin, mode);
#else
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
#endif

  return s;
}
