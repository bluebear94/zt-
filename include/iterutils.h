#pragma once

namespace sca {
  template<typename It>
  struct IRev {
    static auto get(It it) {
      return std::reverse_iterator<It>(it);
    }
    template<typename T>
    static It begin(const std::vector<T>& v) {
      return v.begin();
    }
    template<typename T>
    static It end(const std::vector<T>& v) {
      return v.end();
    }
    template<typename T>
    static auto cbegin(const std::vector<T>& v) {
      return v.cbegin();
    }
    template<typename T>
    static auto cend(const std::vector<T>& v) {
      return v.cend();
    }
    static inline bool shouldSwapLR = false;
  };
  template<typename It>
  struct IRev<std::reverse_iterator<It>> {
    static auto get(std::reverse_iterator<It> it) {
      return it.base();
    }
    template<typename T>
    static std::reverse_iterator<It> begin(const std::vector<T>& v) {
      return v.rbegin();
    }
    template<typename T>
    static std::reverse_iterator<It> end(const std::vector<T>& v) {
      return v.rend();
    }
    template<typename T>
    static auto cbegin(const std::vector<T>& v) {
      return v.crbegin();
    }
    template<typename T>
    static auto cend(const std::vector<T>& v) {
      return v.crend();
    }
    static inline bool shouldSwapLR = true;
  };
  template<typename It>
  auto reverseIterator(It it) {
    return IRev<It>::get(it);
  }
}
