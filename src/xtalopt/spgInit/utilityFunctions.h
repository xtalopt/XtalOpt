/**********************************************************************
  utilityFunctions.h - Various utility functions for spgInit

  Copyright (C) 2016 by Patrick S. Avery

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#ifndef UTILITY_FUNCTIONS_H
#define UTILITY_FUNCTIONS_H

// Basic split of a string based upon a delimiter.
static inline std::vector<std::string> split(const std::string& s, char delim)
{
  std::vector<std::string> elems;
  std::stringstream ss(s);
  std::string item;
  while (getline(ss, item, delim)) {
    elems.push_back(item);
  }
  return elems;
}

// Used to replace ')' with ' ', for instance
static inline void replace(std::string& s, char oldChar, char newChar)
{
  std::replace(s.begin(), s.end(), oldChar, newChar);
}

static inline void removeEmptyStrings(std::vector<std::string>& v)
{
  for (size_t i = 0; i < v.size(); i++) {
    if (v.at(i).size() == 0) v.erase(v.begin() + i);
  }
}

static inline void removeChar(std::string& s, char c)
{
  s.erase(std::remove(s.begin(), s.end(), c), s.end());
}

// Used to change something like "(0,0,0)(0.5,0,0)" to {"0,0,0","0.5,0,0"}
static inline std::vector<std::string>
splitAndRemoveParenthesis(const std::string& s)
{
  std::vector<std::string> ret = split(s, '(');
  // Remove any empty strings
  removeEmptyStrings(ret);
  // Remove all other parenthesis
  for (size_t i = 0; i < ret.size(); i++) removeChar(ret[i], ')');
  return ret;
}

// Basic check to see if a string is a number
// Includes negative numbers
// If it runs into an "x", "y", or "z", it should return false
static inline bool isNumber(const std::string& s)
{
  std::string::const_iterator it = s.begin();
  while (it != s.end() && (isdigit(*it) || *it == '-' || *it == '.')) ++it;
  return !s.empty() && it == s.end();
}

// A simple function used in the std::sort in the function below
static inline bool greaterThan(const std::pair<uint, uint>& a,
                               const std::pair<uint, uint>& b)
{
  return a.first > b.first;
}

static inline bool numIsEven(int num)
{
  if (num % 2 == 0) return true;
  return false;
}

static inline bool numIsOdd(int num)
{
  return !numIsEven(num);
}

static inline bool isDigit(char d)
{
  if (d != '0' && d != '1' && d != '2' && d != '3' && d != '4' &&
      d != '5' && d != '6' && d != '7' && d != '8' && d != '9') return false;
  return true;
}

static const double PI = 3.14159265;

inline double deg2rad(double a)
{
  return a * PI / 180.0;
}

inline double rad2deg(double a)
{
  return a * 180.0 / PI;
}

#endif
