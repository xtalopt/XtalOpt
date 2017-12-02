/**********************************************************************
  utilityFunctions.h - Various utility functions for randSpg

  Copyright (C) 2015 - 2016 by Patrick S. Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

 ***********************************************************************/

#ifndef UTILITY_FUNCTIONS_H
#define UTILITY_FUNCTIONS_H

#include <set>
#include <sstream>
#include <algorithm>

// Basic split of a string based upon a delimiter.
static inline std::vector<std::string> split(const std::string& s, char delim)
{
  std::vector<std::string> elems;
  std::istringstream ss(s); // istringstream is faster to use than stringstream
  std::string item;
  while (getline(ss, item, delim)) elems.push_back(item);
  return elems;
}

// Used to replace ')' with ' ', for instance
static inline void replace(std::string& s, char oldChar, char newChar)
{
  std::replace(s.begin(), s.end(), oldChar, newChar);
}

// Replace every occurrence of a string with another string
static void replaceAll(std::string& str,
                       const std::string& from,
                       const std::string& to) {
  if(from.empty()) return;
  size_t start_pos = 0;
  while((start_pos = str.find(from, start_pos)) != std::string::npos) {
    str.replace(start_pos, from.length(), to);
    start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
  }
}

static inline bool hasEnding(const std::string& fullString,
                             const std::string& ending)
{
  if (fullString.length() >= ending.length())
    return (0 == fullString.compare(fullString.length() - ending.length(),
                                    ending.length(), ending));
  else return false;
}

// Replaces all occurrences of "\n" with "<br>" in a string
static inline std::string useHTMLReturns(const std::string& str)
{
  std::string ret = str;
  replaceAll(ret, "\n", "<br>");
  return ret;
}

static inline std::string removeSpacesAndReturns(const std::string& str)
{
  std::string ret = str;
  ret.erase(std::remove(ret.begin(), ret.end(), '\n'), ret.end());
  ret.erase(std::remove(ret.begin(), ret.end(), '\r'), ret.end());
  ret.erase(std::remove(ret.begin(), ret.end(), ' '), ret.end());
  return ret;
}

static inline void removeEmptyStrings(std::vector<std::string>& v)
{
  for (size_t i = 0; i < v.size(); i++) {
    if (v[i].size() == 0) v.erase(v.begin() + i);
  }
}

template <typename T>
static inline std::vector<T> removeDuplicates(const std::vector<T>& v)
{
  std::vector<T> ret;
  std::set<T> s(v.begin(), v.end());
  ret.assign(s.begin(), s.end());
  return ret;
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
  while (it != s.end() && (isdigit(*it) ||
         (*it == '-' && it == s.begin()) || // Hyphen must be at beginning
         *it == '.')) ++it;
  return !s.empty() && it == s.end();
}

static inline bool contains(const std::string& s, char c)
{
  std::size_t found = s.find_first_of(c);
  if (found != std::string::npos) return true;
  return false;
}

static inline bool contains(const std::string& s1, const std::string& s2)
{
  if (s1.find(s2) != std::string::npos) return true;
  return false;
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

inline bool containsOnlySpaces(const std::string& str)
{
  if (str.find_first_not_of(' ') != std::string::npos) return false;
  return true;
}

// Returns a string with leading and trailing whitespace removed
static std::string trim(const std::string& str,
                        const std::string& whitespace = " \t")
{
  const auto strBegin = str.find_first_not_of(whitespace);
  if (strBegin == std::string::npos) return ""; // no content

  const auto strEnd = str.find_last_not_of(whitespace);
  const auto strRange = strEnd - strBegin + 1;

  return str.substr(strBegin, strRange);
}

// Returns a 'reduced' string. In a reduced string, every series of repeated
// spaces is reduced to 1 space
static std::string reduce(const std::string& str,
                          const std::string& fill = " ",
                          const std::string& whitespace = " \t")
{
  // trim first
  auto result = trim(str, whitespace);

  // replace sub ranges
  auto beginSpace = result.find_first_of(whitespace);
  while (beginSpace != std::string::npos)
  {
    const auto endSpace = result.find_first_not_of(whitespace, beginSpace);
    const auto range = endSpace - beginSpace;

    result.replace(beginSpace, range, fill);

    const auto newStart = beginSpace + fill.length();
    beginSpace = result.find_first_of(whitespace, newStart);
  }

  return result;
}

#endif
