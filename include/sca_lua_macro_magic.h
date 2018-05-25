#pragma once

// TODO: remove [[maybe_unused]] when we're done
#define DEF_CHECK_FUNC(Type, metatableName, C) \
  [[maybe_unused]] \
  static C Type* checkFor##Type##C(lua_State* l, int index = 1) { \
    void* ud = luaL_checkudata(l, index, metatableName); \
    luaL_argcheck(l, ud != nullptr, index, #Type " expected"); \
    return *(C Type**) ud; \
  }
#define DEF_PUSH_FUNC(Type, metatableName, C) \
  [[maybe_unused]] \
  int push##Type##C(lua_State* l, C Type& t) { \
    C Type** ud = (C Type**) lua_newuserdata(l, sizeof(C Type*)); \
    luaL_getmetatable(l, metatableName); \
    lua_setmetatable(l, -2); \
    *ud = &t; \
    return 1; \
  }
#define DEF_FUNCS(Type, metatableName, C) \
  DEF_CHECK_FUNC(Type, metatableName, C); \
  DEF_PUSH_FUNC(Type, metatableName, C)
