/**********************************************************************
  utilityFunctions.h - Various utility functions

  Copyright (C) 2015 - 2017 by Patrick S. Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

 ***********************************************************************/

#ifndef UTILITY_FUNCTIONS_H
#define UTILITY_FUNCTIONS_H

#include <algorithm>
#include <cctype>
#include <globalsearch/constants.h>

// Unfortunately, GCC < 4.9.0 did not include regex, so we have
// to use the Qt libraries if are using GCC < 4.9.0
#if defined(__GNUC__) && __GNUC__ < 4 || (__GNUC__ == 4 && (__GNUC_MINOR__ < 9))
#define GNUC_LESS_THAN_4_9_0
#include <QRegExp>
#include <QStringList>
#else
#include <regex>
#endif

#include <set>
#include <sstream>
#include <vector>

inline bool containsOnlySpaces(const std::string& str)
{
  return std::all_of(str.begin(), str.end(), [](char c) {
    return std::isspace(static_cast<unsigned char>(c));
  });
}

// Used to replace ')' with ' ', for instance
static inline void replace(std::string& s, char oldChar, char newChar)
{
  std::replace(s.begin(), s.end(), oldChar, newChar);
}

// Replace every occurrence of a string with another string
static void replaceAll(std::string& str, const std::string& from,
                       const std::string& to)
{
  if (from.empty())
    return;
  size_t start_pos = 0;
  while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
    str.replace(start_pos, from.length(), to);
    start_pos +=
      to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
  }
}

static inline bool hasEnding(const std::string& fullString,
                             const std::string& ending)
{
  if (fullString.length() >= ending.length())
    return (0 ==
            fullString.compare(fullString.length() - ending.length(),
                               ending.length(), ending));
  else
    return false;
}

// Replaces all occurrences of "\n" with "<br>" in a string
static inline std::string useHTMLReturns(const std::string& str)
{
  std::string ret = str;
  replaceAll(ret, "\n", "<br>");
  return ret;
}

static inline std::string removeSpaces(std::string str)
{
  str.erase(std::remove_if(str.begin(), str.end(),
                           [](char c) {
                             return std::isspace(static_cast<unsigned char>(c));
                           }),
            str.end());
  return str;
}

static inline void removeEmptyStrings(std::vector<std::string>& v)
{
  v.erase(std::remove_if(v.begin(), v.end(), containsOnlySpaces), v.end());
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

// Basic check to see if a string is a number
// Includes negative numbers
// If it runs into an "x", "y", or "z", it should return false
static inline bool isNumber(const std::string& s)
{
  // Make sure there is at most one period first
  if (std::count(s.begin(), s.end(), '.') > 1)
    return false;
  std::string::const_iterator it = s.begin();
  while (it != s.end() &&
         (isdigit(*it) ||
          (*it == '-' && it == s.begin()) || // Hyphen must be at beginning
          *it == '.'))
    ++it;
  return !s.empty() && it == s.end();
}

// Basic check to see if a string is an integer
// Includes positive and negative numbers
static inline bool isInteger(const std::string& s)
{
  if (s.empty() || ((!isdigit(s[0])) && (s[0] != '-') && (s[0] != '+')))
    return false;

  char* p;
  strtol(s.c_str(), &p, 10);

  return (*p == 0);
}

static inline bool contains(const std::string& s, char c)
{
  std::size_t found = s.find_first_of(c);
  if (found != std::string::npos)
    return true;
  return false;
}

static inline bool contains(const std::string& s1, const std::string& s2)
{
  if (s1.find(s2) != std::string::npos)
    return true;
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
  if (num % 2 == 0)
    return true;
  return false;
}

static inline bool numIsOdd(int num)
{
  return !numIsEven(num);
}

static inline bool isDigit(char d)
{
  if (d != '0' && d != '1' && d != '2' && d != '3' && d != '4' && d != '5' &&
      d != '6' && d != '7' && d != '8' && d != '9')
    return false;
  return true;
}


inline double deg2rad(double a)
{
  return a * DEG2RAD;
}

inline double rad2deg(double a)
{
  return a * RAD2DEG;
}

// Returns a string with leading and trailing whitespace removed
inline std::string trim(const std::string& str,
                        const std::string& whitespace = " \t")
{
  const auto strBegin = str.find_first_not_of(whitespace);
  if (strBegin == std::string::npos)
    return ""; // no content

  const auto strEnd = str.find_last_not_of(whitespace);
  const auto strRange = strEnd - strBegin + 1;

  return str.substr(strBegin, strRange);
}

// Returns a 'reduced' string. In a reduced string, every series of repeated
// spaces is reduced to 1 space
inline std::string reduce(const std::string& str, const std::string& fill = " ",
                          const std::string& whitespace = " \t")
{
  // trim first
  auto result = trim(str, whitespace);

  // replace sub ranges
  auto beginSpace = result.find_first_of(whitespace);
  while (beginSpace != std::string::npos) {
    const auto endSpace = result.find_first_not_of(whitespace, beginSpace);
    const auto range = endSpace - beginSpace;

    result.replace(beginSpace, range, fill);

    const auto newStart = beginSpace + fill.length();
    beginSpace = result.find_first_of(whitespace, newStart);
  }

  return result;
}

// Case insensitive comparison of characters
inline bool caseInsensitiveCompareC(unsigned char a, unsigned char b)
{
  return std::tolower(a) == std::tolower(b);
}

// Case insensitive comparison of strings
inline bool caseInsensitiveCompare(std::string const& a, std::string const& b)
{
  if (a.size() == b.size())
    return std::equal(b.begin(), b.end(), a.begin(), caseInsensitiveCompareC);
  else
    return false;
}

inline std::string getFileExt(const std::string& s)
{
  size_t i = s.rfind('.', s.length());
  if (i != std::string::npos)
    return (s.substr(i + 1, s.length() - i));
  else
    return ("");
}

// Reads a line in reverse from the ifstream and sets the ifstream to
// be at the position where it ended.
inline std::istream& reverseGetline(std::istream& in, std::string& line)
{
  line.clear();
  // First check to see if we are at EOF. If we are, move back one character.
  if (in.peek() == EOF) {
    in.clear();
    in.seekg(-1, std::ios::cur);
  }
  // If in.tellg() becomes negative, we know we are at the end
  while (in.tellg() >= 0) {
    char c = in.peek();
    in.seekg(-1, std::ios::cur);
    if (c != '\n')
      line.insert(line.begin(), c);
    else
      break;
  }
  return in;
}

// Basic split of a string based upon a delimiter.
static inline std::vector<std::string> split(const std::string& s, char delim,
                                             bool skipEmpty = true)
{
  std::vector<std::string> elems;
  std::istringstream ss(s); // istringstream is faster to use than stringstream
  std::string item;
  while (getline(ss, item, delim)) {
    if (!skipEmpty)
      elems.push_back(item);
    else if (!containsOnlySpaces(item))
      elems.push_back(item);
  }
  return elems;
}

static inline std::vector<std::string> reSplit(const std::string& s,
                                               const std::string& regex,
                                               bool skipEmpty = true)
{
// Unfortunately, regex was not defined in GNU until GNU 4.9.0, so if
// we are les than 4.9.0, we have to use Qt to do the regex operations
#ifdef GNUC_LESS_THAN_4_9_0
  QStringList list = QString(s.c_str()).split(
    QRegExp(regex.c_str()),
    skipEmpty ? QString::SkipEmptyParts : QString::KeepEmptyParts);
  std::vector<std::string> ret;
  std::for_each(list.begin(), list.end(),
                [&ret](const QString& s) { ret.push_back(s.toStdString()); });
  return ret;
#else
  std::regex re(regex);
  std::sregex_token_iterator first(s.begin(), s.end(), re, -1), last;
  std::vector<std::string> ret({ first, last });
  if (skipEmpty)
    removeEmptyStrings(ret);
  return ret;
#endif
}

// Used to change something like "(0,0,0)(0.5,0,0)" to {"0,0,0","0.5,0,0"}
static inline std::vector<std::string> splitAndRemoveParenthesis(
  const std::string& s)
{
  std::vector<std::string> ret = split(s, '(');
  // Remove any empty strings
  removeEmptyStrings(ret);
  // Remove all other parenthesis
  for (size_t i = 0; i < ret.size(); i++)
    removeChar(ret[i], ')');
  return ret;
}

#endif
