#pragma once

#include <lua.hpp>

namespace sca {
  class SCA;
  struct PhonemeSpec;
  namespace lua {
    // Initialise the Lua bindings for SCA objects
    int init(lua_State* l);
    // Push a pointer to an SCA on the Lua stack
    int pushSCA(lua_State* l, SCA& sca);
    int pushPhonemeSpec(lua_State* l, PhonemeSpec& phoneme);
  }
}
