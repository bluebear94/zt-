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
  template<typename T>
  static void replaceSubrange(
      std::vector<T>& v1,
      size_t b1,
      size_t e1,
      typename std::vector<T>::iterator b2,
      typename std::vector<T>::iterator e2) {
    size_t size1 = (size_t) (e1 - b1);
    size_t size2 = (size_t) (e2 - b2);
    size_t oldSize = v1.size();
    // Shift left or right?
    if (size1 > size2) { // Left
      std::move(
        v1.begin() + e1, v1.end(),
        v1.begin() + b1 + size2);
      v1.resize(v1.size() + size2 - size1);
    } else if (size1 < size2) { // Right
      v1.resize(v1.size() + size2 - size1);
      std::move_backward(
        v1.begin() + e1, v1.begin() + oldSize,
        v1.end());
    }
    std::move(b2, e2, v1.begin() + b1);
  }
  template<typename T>
  static void replaceSubrange(
      std::vector<T>& v1,
      typename std::vector<T>::iterator b1,
      typename std::vector<T>::iterator e1,
      typename std::vector<T>::iterator b2,
      typename std::vector<T>::iterator e2) {
    replaceSubrange(v1, b1 - v1.begin(), e1 - v1.begin(), b2, e2);
  }
}
