#pragma once

#include <stdint.h>
#include <string>
#include <string_view>
#include <vector>

namespace ustr {

// Returns the byte length of a UTF-8 character given its leading byte.
inline int CharLen(uint8_t c) {
    if ((c & 0x80) == 0) return 1;
    if ((c & 0xE0) == 0xC0) return 2;
    if ((c & 0xF0) == 0xE0) return 3;
    if ((c & 0xF8) == 0xF0) return 4;
    return 1;
}

// Returns true if the string is valid UTF-8 and not empty/whitespace-only.
bool IsValidWord(const std::string& s);

inline uint32_t DecodeUTF8(std::string_view ch) {
    if (ch.empty()) return 0;
    const unsigned char* p = reinterpret_cast<const unsigned char*>(ch.data());
    if (ch.size() == 1) return p[0];
    if (ch.size() == 2) {
        return ((p[0] & 0x1F) << 6) | (p[1] & 0x3F);
    }
    if (ch.size() == 3) {
        return ((p[0] & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F);
    }
    if (ch.size() == 4) {
        return ((p[0] & 0x07) << 18) |
               ((p[1] & 0x3F) << 12) |
               ((p[2] & 0x3F) << 6) |
               (p[3] & 0x3F);
    }
    return 0;
}

inline bool IsHan(std::string_view ch) {
    const uint32_t cp = DecodeUTF8(ch);
    return (cp >= 0x3400 && cp <= 0x4DBF) ||
           (cp >= 0x4E00 && cp <= 0x9FFF) ||
           (cp >= 0xF900 && cp <= 0xFAFF) ||
           (cp >= 0x20000 && cp <= 0x2A6DF) ||
           (cp >= 0x2A700 && cp <= 0x2B73F) ||
           (cp >= 0x2B740 && cp <= 0x2B81F) ||
           (cp >= 0x2B820 && cp <= 0x2CEAF) ||
           (cp >= 0x2CEB0 && cp <= 0x2EBEF) ||
           (cp >= 0x30000 && cp <= 0x3134F);
}

// Returns true if the UTF-8 character is a punctuation or whitespace delimiter.
inline bool IsPunct(const std::string& ch) {
    if (ch.size() == 1) {
        unsigned char c = ch[0];
        // ASCII punctuation and whitespace
        return c <= 0x7F && !((c >= '0' && c <= '9') ||
                              (c >= 'A' && c <= 'Z') ||
                              (c >= 'a' && c <= 'z'));
    }
    if (ch.size() == 3) {
        const unsigned char* p = reinterpret_cast<const unsigned char*>(ch.data());
        int cp = ((p[0] & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F);
        // CJK punctuation & symbols
        if (cp >= 0x3000 && cp <= 0x303F) return true;
        // Fullwidth punctuation
        if (cp >= 0xFF01 && cp <= 0xFF0F) return true;
        if (cp >= 0xFF1A && cp <= 0xFF20) return true;
        if (cp >= 0xFF3B && cp <= 0xFF40) return true;
        if (cp >= 0xFF5B && cp <= 0xFF65) return true;
        // General punctuation
        if (cp >= 0x2000 && cp <= 0x206F) return true;
    }
    return false;
}

// Split a string into segments by punctuation/whitespace delimiters.
// Returns pairs of (substring, is_punct).
inline std::vector<std::pair<std::string, bool>> SplitByPunct(const std::string& s) {
    std::vector<std::pair<std::string, bool>> result;
    int n = static_cast<int>(s.size());
    int i = 0;
    int seg_start = 0;
    bool seg_is_punct = false;

    if (n == 0) return result;

    // Determine type of first char
    int first_len = CharLen(static_cast<uint8_t>(s[0]));
    seg_is_punct = IsPunct(s.substr(0, first_len));

    while (i < n) {
        int len = CharLen(static_cast<uint8_t>(s[i]));
        if (i + len > n) len = 1;
        bool is_punct = IsPunct(s.substr(i, len));

        if (is_punct) {
            // Each punctuation character is its own segment
            if (seg_start < i && !seg_is_punct) {
                result.emplace_back(s.substr(seg_start, i - seg_start), false);
            }
            result.emplace_back(s.substr(i, len), true);
            seg_start = i + len;
            seg_is_punct = true;
        } else if (seg_is_punct) {
            seg_start = i;
            seg_is_punct = false;
        }
        i += len;
    }
    if (seg_start < n && !seg_is_punct) {
        result.emplace_back(s.substr(seg_start, n - seg_start), false);
    }
    return result;
}

// Split a non-punctuation segment into contiguous Han / non-Han runs.
inline std::vector<std::pair<std::string, bool>> SplitByHan(const std::string& s) {
    std::vector<std::pair<std::string, bool>> result;
    const int n = static_cast<int>(s.size());
    if (n == 0) return result;

    int i = 0;
    int seg_start = 0;
    int first_len = CharLen(static_cast<uint8_t>(s[0]));
    if (first_len <= 0 || first_len > n) first_len = 1;
    bool seg_is_han = IsHan(std::string_view(s.data(), first_len));

    while (i < n) {
        int len = CharLen(static_cast<uint8_t>(s[i]));
        if (len <= 0 || i + len > n) len = 1;
        bool is_han = IsHan(std::string_view(s.data() + i, len));

        if (i > seg_start && is_han != seg_is_han) {
            result.emplace_back(s.substr(seg_start, i - seg_start), seg_is_han);
            seg_start = i;
            seg_is_han = is_han;
        }
        i += len;
    }

    if (seg_start < n) {
        result.emplace_back(s.substr(seg_start, n - seg_start), seg_is_han);
    }
    return result;
}

} // namespace ustr
