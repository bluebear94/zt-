#pragma once

namespace sca {
  bool isASCII(unsigned char c) {
    return c < 128;
  }
  bool isContinuation(unsigned char c) {
    return c >= 128 && c < 192;
  }
  bool is2ByteStarter(unsigned char c) {
    return c >= 192 && c < 224;
  }
  bool is3ByteStarter(unsigned char c) {
    return c >= 224 && c < 240;
  }
  bool is4ByteStarter(unsigned char c) {
    return c >= 240 && c < 248;
  }
  size_t expectedContinuationBytes(unsigned char c) {
    return
      is2ByteStarter(c) ? 1 :
      is3ByteStarter(c) ? 2 : 3;
  }
  constexpr int starterOffsets[3] = {192, 224, 240};

  template<typename S = std::string>
  class UTF8Iterator {
  public:
    UTF8Iterator(const UTF8Iterator& other)
      : s(other.s), i(other.i) {}
    UTF8Iterator(S& s, bool end = false)
      : s(s), i(end ? s.length() : 0) {}
    UTF8Iterator(S& s, size_t i)
      : s(s), i(i) {}
    int get() const {
      return const_cast<UTF8Iterator*>(this)->get2<false>();
    }
    int getLength() const {
      return const_cast<UTF8Iterator*>(this)->get2<false, true>();
    }
    int getAndAdvance() {
      return get2<true>();
    }
    UTF8Iterator& operator++() {
      (void) get2<true>();
      return *this;
    }
    UTF8Iterator operator++(int dummy) {
      (void) dummy;
      UTF8Iterator old(*this);
      (void) get2<true>();
      return old;
    }
    UTF8Iterator& operator--() {
      recede();
      return *this;
    }
    UTF8Iterator operator--(int dummy) {
      (void) dummy;
      UTF8Iterator old(*this);
      recede();
      return old;
    }
    size_t position() const {
      return i;
    }
    bool operator==(const UTF8Iterator& other) {
      return &(s) == &(other.s) && i == other.i;
    }
    bool operator!=(const UTF8Iterator& other) {
      return !(*this == other);
    }
  private:
    template<bool advance = false, bool lmode = false>
    int get2() {
      size_t oldI = i;
      size_t j = i + 1;
      int codepoint;
      // We do our operations with unsigned char's to simplify things.
      unsigned char curr = (unsigned char) s[i];
      if (isASCII(curr)) {
        codepoint = curr;
      } else if (isContinuation(curr) || curr >= 248) {
        // Invalid values are mapped to negative integers.
        // This is so that files that somehow have malformed text
        // can still be edited.
        codepoint = -curr;
      } else  {
        size_t expectedContinuations = expectedContinuationBytes(curr);
        codepoint = curr - starterOffsets[expectedContinuations - 1];
        bool ok = true;
        if (j + expectedContinuations <= s.length()) {
          for (size_t k = 1; k <= expectedContinuations; ++k) {
            unsigned char cont = s[i + k];
            if (!isContinuation(cont)) {
              ok = false;
              break;
            }
            codepoint = (codepoint << 6) | (cont & 0x7f);
          }
        } else ok = false;
        if (ok)
          j += expectedContinuations;
        else
          codepoint = -curr;
      }
      if constexpr (advance) i = j;
      if constexpr (lmode)
        return j - oldI;
      else
        return codepoint;
    }
    void recede() {
      // Go back a byte until we encounter an ASCII character or a starter
      size_t oldI = i;
      size_t j = i;
      do {
        --j;
      } while (j > 0 && (isContinuation(s[j]) || ((unsigned char) s[j] >= 248)));
      // Now verify that this is a valid UTF-8 sequence
      // and spans all the way to the old position
      i = j;
      int codepoint = get2<true>();
      // Not a valid UTF-8 sequence or it doesn't span all the way
      // Recede only one byte.
      if (codepoint < 0 || position() != oldI) {
        i = oldI - 1;
      } else {
        i = j;
      }
    }
    S& s;
    size_t i;
  };

  inline std::string utf8CodepointToChar(int code) {
    if (code < 0) return std::string(1, (char) -code);
    if (code < 128) return std::string(1, (char) code);
    std::string s;
    if (code < 0x800) { // 2 bytes
      s += (char) (0xC0 | (code >> 6));
      s += (char) (0x80 | (code & 63));
    } else if (code < 0x10000) { // 3 bytes
      s += (char) (0xE0 | (code >> 12));
      s += (char) (0x80 | ((code >> 6) & 63));
      s += (char) (0x80 | (code & 63));
    } else if (code < 0x101000) { // 4 bytes
      s += (char) (0xF0 | (code >> 18));
      s += (char) (0x80 | ((code >> 12) & 63));
      s += (char) (0x80 | ((code >> 6) & 63));
      s += (char) (0x80 | (code & 63));
    }
    return s;
  }

  // My personal favourite
  constexpr size_t TAB_WIDTH = 2;

  inline size_t wcwidthp(int codepoint) {
    // Tab width is configurable
    if (codepoint == '\t') return TAB_WIDTH;
    // Invalid byte characters are drawn as their hex in reverse video
    // Control characters are drawn as ^ plus another character
    if (codepoint < 32 || codepoint == 127) return 2;
    return wcwidth(codepoint);
  }
}