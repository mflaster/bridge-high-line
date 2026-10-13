#ifndef BRIDGE_STRING_UTIL_H_
#define BRIDGE_STRING_UTIL_H_

#include <string>

// Trims leading whitespace (spaces, tabs, carriage returns, newlines) from string s.
std::string TrimLeft(const std::string &s);

// Trims trailing whitespace (spaces, tabs, carriage returns, newlines) from string s.
std::string TrimRight(const std::string &s);

// Trims both leading and trailing whitespace from string s.
std::string Trim(const std::string &s);

// Converts string s to lowercase.
std::string ToLower(const std::string &s);

#endif  // BRIDGE_STRING_UTIL_H_
