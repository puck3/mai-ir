#pragma once
#include <string>
#include <vector>

inline bool is_cyrillic_word(const std::string& word) {
  if (word.empty()) return false;
  unsigned char first = word[0];
  return (first == 0xD0 || first == 0xD1);
}

inline void english_stem(std::string& word) {
  static const std::vector<std::string> suffixes = {
      "ing", "ed",   "s",    "es",   "er",   "est",
      "ly",  "ment", "ness", "tion", "able", "ible"};

  for (const auto& suf : suffixes) {
    if (word.length() > suf.length() + 1 &&
        word.substr(word.length() - suf.length()) == suf) {
      word.erase(word.length() - suf.length());
      break;
    }
  }
}

inline void russian_stem(std::string& word) {
  static const std::vector<std::string> suffixes = {
      "ов", "ев", "ин", "ын", "ая", "яя", "ое",  "ее",  "ые", "ие",
      "ам", "ям", "ом", "ем", "ах", "ях", "ами", "ями", "у",  "ю",
      "ем", "ом", "ой", "ей", "а",  "я",  "о",   "е",   "ы",  "и"};

  for (const auto& suf : suffixes) {
    if (word.length() > suf.length() + 2 &&
        word.substr(word.length() - suf.length()) == suf) {
      word.erase(word.length() - suf.length());
      break;
    }
  }
}

inline void stem(std::string& word) {
  if (word.empty()) return;

  if (is_cyrillic_word(word)) {
    russian_stem(word);
  } else {
    english_stem(word);
  }
}