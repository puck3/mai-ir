#pragma once
#include <cctype>
#include <string>
#include <vector>

inline bool is_utf8_start(unsigned char c) { return (c & 0xC0) != 0x80; }

inline void to_lower_utf8(std::string& s) {
  std::string result;
  for (size_t i = 0; i < s.size();) {
    unsigned char c = s[i];

    if (c < 128) {
      result += static_cast<char>(std::tolower(c));
      i++;
    } else if (c == 0xD0 && i + 1 < s.size()) {
      unsigned char c2 = s[i + 1];

      if (c2 >= 0x90 && c2 <= 0x9F) {
        result += static_cast<char>(c);
        result += static_cast<char>(c2 + 0x20);
      } else if (c2 >= 0xA0 && c2 <= 0xAF) {
        result += static_cast<char>(0xD1);
        result += static_cast<char>(c2 - 0x20);
      } else if (c2 == 0x81) {
        result += static_cast<char>(0xD1);
        result += static_cast<char>(0x91);
      } else {
        result += static_cast<char>(c);
        result += static_cast<char>(c2);
      }
      i += 2;
    } else if (c == 0xD1 && i + 1 < s.size()) {
      result += static_cast<char>(c);
      result += static_cast<char>(s[i + 1]);
      i += 2;
    } else {
      result += static_cast<char>(c);
      i++;
    }
  }
  s = result;
}

inline void tokenize(const std::string& text,
                     std::vector<std::string>& tokens) {
  std::string buf;
  bool in_word = false;

  for (size_t i = 0; i < text.size();) {
    unsigned char c = text[i];

    bool is_letter = false;

    if (std::isalpha(c)) {
      is_letter = true;
      buf += c;
      i++;
    } else if ((c & 0xE0) == 0xC0 && i + 1 < text.size()) {
      unsigned char c2 = text[i + 1];
      if ((c == 0xD0 && ((c2 >= 0x90 && c2 <= 0xBF) || c2 == 0x81)) ||
          (c == 0xD1 && ((c2 >= 0x80 && c2 <= 0x8F) || c2 == 0x91))) {
        is_letter = true;
        buf += c;
        buf += c2;
      }
      i += 2;
    } else {
      i++;
    }

    if (is_letter) {
      in_word = true;
    } else {
      if (in_word && buf.size() >= 2) {
        tokens.push_back(buf);
        buf.clear();
      }
      in_word = false;
    }
  }

  if (in_word && buf.size() >= 2) {
    tokens.push_back(buf);
  }
}