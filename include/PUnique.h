#pragma once

namespace sca {
  template<typename T>
  class PUnique {
  public:
    static PUnique<T> owner(T* p) {
      return PUnique(p, true);
    }
    static PUnique<T> observer(T* p) {
      return PUnique(p, false);
    }
    static const PUnique<T> owner(const T* p) {
      return PUnique((T*) p, true);
    }
    static const PUnique<T> observer(const T* p) {
      return PUnique((T*) p, false);
    }
    PUnique(PUnique&& o) : p(o.p), owned(o.owned) {
      o.owned = false;
      o.p = nullptr;
    }
    PUnique<T>& operator=(PUnique&& o) {
      p = o.p;
      owned = o.owned;
      o.owned = false;
      o.p = nullptr;
      return *this;
    }
    ~PUnique() {
      if (owned) delete p;
    }
    T* get() { return p; }
    const T* get() const { return p; }
    T& operator*() { return *p; }
    const T& operator*() const { return *p; }
    T* operator->() { return p; }
    const T* operator->() const { return p; }
  private:
    PUnique(T* p, bool owned) : p(p), owned(owned) {}
    T* p;
    bool owned;
  };
  template<typename T, typename... Args>
  PUnique<T> makePOwner(Args&&... args) {
    return PUnique<T>::owner(new T(std::forward<Args...>(args...)));
  }
  template<typename T>
  PUnique<T> makePObserver(T& t) {
    return PUnique<T>::observer(&t);
  }
  template<typename T>
  const PUnique<T> makePObserver(const T& t) {
    return PUnique<T>::observer(&t);
  }
}
