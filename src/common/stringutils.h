/**********************************************************************
  stringutils - Various string utility functions

  Copyright (C) 2015 - 2017 by Patrick S. Avery
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef COMMON_STRINGUTILS_H
#define COMMON_STRINGUTILS_H

#include <common/compatibility/qt_compat.h>
#include <common/compatibility/platform_compat.h>

#include <QDateTime>
#include <QString>

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <istream>
#include <limits>
#include <set>
#include <sstream>
#include <string>
#include <vector>

// Clang also defines __GNUC__ (as 4.2), so exclude it explicitly: this
//   check is only about real GCC releases older than 4.9.
#if defined(__GNUC__) && !defined(__clang__) && \
  (__GNUC__ < 4 || (__GNUC__ == 4 && (__GNUC_MINOR__ < 9)))
#define GNUC_LESS_THAN_4_9_0
#include <QRegularExpression>
#include <QStringList>
#else
#include <regex>
#endif

namespace Common {

inline QString boolToText(bool value)
{
  return value ? "true" : "false";
}

inline QString doubleToText(double value)
{
  return QString::number(value, 'g', 15);
}

// Convert between values and text. A bad value leaves its output unchanged.
inline QString valueToText(bool value) { return boolToText(value); }
inline QString valueToText(int value) { return QString::number(value); }
inline QString valueToText(unsigned int value) { return QString::number(value); }
inline QString valueToText(double value) { return doubleToText(value); }
inline QString valueToText(const QString& value) { return value; }

inline bool textToValue(const QString& text, bool& out)
{
  const QString value = text.trimmed();
  if (value.compare("true", Qt::CaseInsensitive) == 0 ||
      value.compare("yes", Qt::CaseInsensitive) == 0) {
    out = true;
    return true;
  }
  if (value.compare("false", Qt::CaseInsensitive) == 0 ||
      value.compare("no", Qt::CaseInsensitive) == 0) {
    out = false;
    return true;
  }
  return false;
}
inline bool textToValue(const QString& text, int& out)
{
  bool ok = false;
  const int value = text.trimmed().toInt(&ok);
  if (!ok)
    return false;
  out = value;
  return true;
}
inline bool textToValue(const QString& text, unsigned int& out)
{
  bool ok = false;
  const unsigned int value = text.trimmed().toUInt(&ok);
  if (!ok)
    return false;
  out = value;
  return true;
}
inline bool textToValue(const QString& text, double& out)
{
  bool ok = false;
  const double value = text.trimmed().toDouble(&ok);
  if (!ok || !GS_ISFINITE(value))
    return false;
  out = value;
  return true;
}
inline bool textToValue(const QString& text, QString& out)
{
  out = text;
  return true;
}

inline QString uniqueTimestampString(const QString& id = QString())
{
  // Return a "unique" string with current date and time
  //   (millisecond resolution), e.g., for distinct file names.
  // The user can "optionally" add an extra identifier with "id".
  // Format codes:
  //   yyyy = 4-digit year
  //   MM   = 2-digit month (01-12)
  //   dd   = 2-digit day (01-31)
  //   HH   = 2-digit hour (00-23)
  //   mm   = 2-digit minute (00-59)
  //   ss   = 2-digit second (00-59)
  //   zzz  = 3-digit millisecond (000-999)
  QString ts = QDateTime::currentDateTime()
      .toString(QStringLiteral("yyyyMMdd_HHmmss_zzz"));

  if (!id.isEmpty())
    ts += QLatin1Char('_') + id;

  return ts;
}

// Replace every occurrence of a string with another string
inline void replaceAll(std::string& str, const std::string& from, const std::string& to)
{
  if (from.empty())
    return;
  size_t start_pos = 0;
  while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
    str.replace(start_pos, from.length(), to);
    start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
  }
}

inline std::string removeSpaces(std::string str)
{
  str.erase(std::remove_if(str.begin(), str.end(), [](char c) {
                             return std::isspace(static_cast<unsigned char>(c));
                           }),
            str.end());
  return str;
}

inline bool containsOnlySpaces(const std::string& str)
{
  return std::all_of(str.begin(), str.end(), [](char c) {
                       return std::isspace(static_cast<unsigned char>(c));
                     });
}

inline void removeEmptyStrings(std::vector<std::string>& v)
{
  v.erase(std::remove_if(v.begin(), v.end(), containsOnlySpaces), v.end());
}

template <typename T>
inline std::vector<T> removeDuplicates(const std::vector<T>& v)
{
  std::vector<T> ret;
  std::set<T> s(v.begin(), v.end());
  ret.assign(s.begin(), s.end());
  return ret;
}

inline void removeChar(std::string& s, char c)
{
  s.erase(std::remove(s.begin(), s.end(), c), s.end());
}

inline bool contains(const std::string& s, char c)
{
  return s.find_first_of(c) != std::string::npos;
}

inline bool contains(const std::string& s1, const std::string& s2)
{
  return s1.find(s2) != std::string::npos;
}

// Returns a string with leading and trailing whitespace removed
inline std::string trim(const std::string& str, const std::string& whitespace = " \t")
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
  return false;
}

inline std::string getFileExt(const std::string& s)
{
  size_t i = s.rfind('.', s.length());
  if (i != std::string::npos)
    return s.substr(i + 1, s.length() - i);
  return "";
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
inline std::vector<std::string> split(const std::string& s, char delim, bool skipEmpty = true)
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

inline std::vector<std::string> reSplit(const std::string& s, const std::string& regex,
                                        bool skipEmpty = true)
{
// Unfortunately, regex was not defined in GNU until GNU 4.9.0, so if
// we are les than 4.9.0, we have to use Qt to do the regex operations
#ifdef GNUC_LESS_THAN_4_9_0
  QStringList list = QString(s.c_str()).split(QRegularExpression(regex.c_str()),
    skipEmpty ? QtCompat::SkipEmptyParts : QtCompat::KeepEmptyParts);
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
inline std::vector<std::string> splitAndRemoveParenthesis(const std::string& s)
{
  std::vector<std::string> ret = split(s, '(');
  // Remove any empty strings
  removeEmptyStrings(ret);
  // Remove all other parenthesis
  for (size_t i = 0; i < ret.size(); i++)
    removeChar(ret[i], ')');
  return ret;
}

// Basic check to see if a string is an integer
// Includes positive and negative numbers
inline bool isInteger(const std::string& s)
{
  if (s.empty() || ((!std::isdigit(static_cast<unsigned char>(s[0]))) &&
                    (s[0] != '-') && (s[0] != '+')))
    return false;

  char* p;
  std::strtol(s.c_str(), &p, 10);

  return *p == 0;
}

// Parse an unsigned size_t string with range checking.
inline bool parseSizeString(const std::string& str, size_t& value)
{
  // Adjust for whitespaces
  const std::string tok = trim(str, " \t\r\n");
  if (tok.empty() || tok[0] == '-')
    return false;

  char* end = nullptr;
  errno = 0;
  const unsigned long long parsed = std::strtoull(tok.c_str(), &end, 10);
  if (errno == ERANGE || end == tok.c_str() || *end != '\0')
    return false;
  if (parsed > static_cast<unsigned long long>(std::numeric_limits<size_t>::max()))
    return false;

  value = static_cast<size_t>(parsed);
  return true;
}

// Parse an int input with checking a range
inline bool parseIntString(const std::string& str, int& value)
{
  // Remove surrounding whitespace.
  const std::string tok = trim(str, " \t\r\n");
  if (tok.empty())
    return false;

  char* end = nullptr;
  errno = 0;
  const long parsed = std::strtol(tok.c_str(), &end, 10);
  if (errno == ERANGE || end == tok.c_str() || *end != '\0')
    return false;
  if (parsed < std::numeric_limits<int>::min() || parsed > std::numeric_limits<int>::max())
    return false;

  value = static_cast<int>(parsed);
  return true;
}

// Parse a double input with range checking
inline bool parseDoubleString(const std::string& str, double& value)
{
  if (str.empty())
    return false;

  // This is to make sure C locale is handled
  bool ok = false;
  const double parsed = QString::fromStdString(str).toDouble(&ok);
  if (!ok)
    return false;

  value = parsed;
  return true;
}

} // namespace Common

#endif // COMMON_STRINGUTILS_H
