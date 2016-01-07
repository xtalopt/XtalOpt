
#ifndef UTILITY_FUNCTIONS_H
#define UTILITY_FUNCTIONS_H

using namespace std;

// Basic split of a string based upon a delimiter.
static inline vector<string> split(const string& s, char delim)
{
  vector<string> elems;
  stringstream ss(s);
  string item;
  while (getline(ss, item, delim)) {
    elems.push_back(item);
  }
  return elems;
}

// Used to replace ')' with '', for instance
static inline void replace(string& s, char oldChar, char newChar)
{
  std::replace(s.begin(), s.end(), oldChar, newChar);
}

static inline void removeEmptyStrings(vector<string>& v)
{
  for (size_t i = 0; i < v.size(); i++) {
    if (v.at(i).size() == 0) v.erase(v.begin() + i);
  }
}

static inline void removeChar(string& s, char c)
{
  s.erase(std::remove(s.begin(), s.end(), c), s.end());
}

static inline vector<string> splitAndRemoveParenthesis(const string& s)
{
  vector<string> ret = split(s, '(');
  // Remove any empty strings
  removeEmptyStrings(ret);
  // Remove all other parenthesis
  for (size_t i = 0; i < ret.size(); i++) removeChar(ret[i], ')');
  return ret;
}

// Basic check to see if a string is a number
// Includes negative numbers
// If it runs into an "x", "y", or "z", it should return false
static inline bool isNumber(const string& s)
{
  std::string::const_iterator it = s.begin();
  while (it != s.end() && (isdigit(*it) || *it == '-' || *it == '.')) ++it;
  return !s.empty() && it == s.end();
}

// A simple function used in the std::sort in the function below
static inline bool greaterThan(const pair<uint, uint>& a,
                               const pair<uint, uint>& b)
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
#endif
