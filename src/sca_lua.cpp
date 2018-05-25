#include "sca_lua.h"

#include "Rule.h"
#include "SCA.h"
#include "sca_lua_macro_magic.h"

namespace sca::lua {
  // ======================== Boilerplate ===========================
  static constexpr const char*
    SCA_METATABLE_NAME = "ztš.SCA";
  static constexpr const char*
    SCA_PHONEME_METATABLE_NAME = "ztš.SCA.Phoneme";
  static constexpr const char*
    SCA_FEATURE_METATABLE_NAME = "ztš.SCA.Feature";
  static constexpr const char*
    SCA_CHARCLASS_METATABLE_NAME = "ztš.SCA.CharClass";
  static constexpr const char* // Not sure if I'll use this.
    SCA_RULE_METATABLE_NAME = "ztš.SCA.Rule";
  // Let the user stomp over SCA's data as they please... within limits.
  DEF_FUNCS(SCA, SCA_METATABLE_NAME, );
  DEF_FUNCS(PhonemeSpec, SCA_PHONEME_METATABLE_NAME, );
  DEF_FUNCS(Feature, SCA_FEATURE_METATABLE_NAME, );
  DEF_FUNCS(CharClass, SCA_CHARCLASS_METATABLE_NAME, );
  // ======================== Utilities =============================
  static std::string_view getStdString(lua_State* l, int arg) {
    size_t len;
    const char* s = luaL_checklstring(l, arg, &len);
    return std::string_view(s, len);
  }
  // ======================== Functions for Lua =====================
  // ======================== SCA ===================================
  // phoneme = sca:getPhoneme(name)
  int scaGetPhoneme(lua_State* l) {
    SCA* sca = checkForSCA(l, 1);
    std::string name = std::string(getStdString(l, 2));
    PhonemeSpec* ps;
    Error res = sca->getPhonemeByName(name, ps);
    if (!res.ok()) {
      return luaL_error(l, "%s", errorAsString(res).c_str());
    }
    pushPhonemeSpec(l, *ps);
    return 1;
  }
  // index, feature = sca:getFeature(name)
  int scaGetFeature(lua_State* l) {
    SCA* sca = checkForSCA(l, 1);
    std::string name = std::string(getStdString(l, 2));
    size_t id; Feature* ps;
    Error res = sca->getFeatureByName(name, id, ps);
    if (!res.ok()) {
      return luaL_error(l, "%s", errorAsString(res).c_str());
    }
    lua_pushinteger(l, id);
    pushFeature(l, *ps);
    return 2;
  }
  // index, class = sca:getClass(name)
  int scaGetClass(lua_State* l) {
    SCA* sca = checkForSCA(l, 1);
    std::string name = std::string(getStdString(l, 2));
    size_t id; CharClass* cc;
    Error res = sca->getClassByName(name, id, cc);
    if (!res.ok()) {
      return luaL_error(l, "%s", errorAsString(res).c_str());
    }
    lua_pushinteger(l, id);
    pushCharClass(l, *cc);
    return 2;
  }
  // feature = sca:getFeatureByIndex(idx)
  int scaGetFeatureByIndex(lua_State* l) {
    SCA* sca = checkForSCA(l, 1);
    lua_Integer i = luaL_checkinteger(l, 2);
    luaL_argcheck(l, i >= 0, 1, "Index must be positive");
    Feature* ps = sca->getFeatureByIDOrNull((size_t) i);
    if (ps == nullptr) {
      lua_pushnil(l);
      return 1;
    }
    pushFeature(l, *ps);
    return 1;
  }
  // class = sca:getClassByIndex(idx)
  int scaGetClassByIndex(lua_State* l) {
    SCA* sca = checkForSCA(l, 1);
    lua_Integer i = luaL_checkinteger(l, 2);
    luaL_argcheck(l, i >= 0, 1, "Index must be positive");
    CharClass* cc = sca->getClassByIDOrNull((size_t) i);
    if (cc == nullptr) {
      lua_pushnil(l);
      return 1;
    }
    pushCharClass(l, *cc);
    return 1;
  }
  // ======================== PhonemeSpec ===========================
  // name = ps:getName()
  int psGetName(lua_State* l) {
    PhonemeSpec* ps = checkForPhonemeSpec(l, 1);
    lua_pushlstring(l, ps->name.c_str(), ps->name.length());
    return 1;
  }
  // charClass = ps:getCharClass(sca)
  int psGetCharClass(lua_State* l) {
    PhonemeSpec* ps = checkForPhonemeSpec(l, 1);
    SCA* sca = checkForSCA(l, 2);
    CharClass* cc = sca->getClassByIDOrNull(ps->charClass);
    if (cc == nullptr) {
      lua_pushnil(l);
      return 1;
    }
    pushCharClass(l, *cc);
    return 1;
  }
  // charClassIndex = ps:getCharClassIndex()
  int psGetCharClassIndex(lua_State* l) {
    PhonemeSpec* ps = checkForPhonemeSpec(l, 1);
    lua_pushinteger(l, ps->charClass);
    return 1;
  }
  // fv = ps:getFeatureValue(fid, sca)
  int psGetFeatureValue(lua_State* l) {
    PhonemeSpec* ps = checkForPhonemeSpec(l, 1);
    lua_Integer fid = luaL_checkinteger(l, 2);
    SCA* sca = checkForSCA(l, 3);
    Feature* f = sca->getFeatureByIDOrNull(fid);
    if (f == nullptr) {
      lua_pushnil(l);
      return 1;
    }
    size_t fv = ps->getFeatureValue(fid, *sca);
    lua_pushinteger(l, fv);
    return 1;
  }
  // fn = ps:getFeatureName(fid, sca)
  int psGetFeatureName(lua_State* l) {
    PhonemeSpec* ps = checkForPhonemeSpec(l, 1);
    lua_Integer fid = luaL_checkinteger(l, 2);
    SCA* sca = checkForSCA(l, 3);
    Feature* f = sca->getFeatureByIDOrNull(fid);
    if (f == nullptr) {
      lua_pushnil(l);
      return 1;
    }
    size_t fv = ps->getFeatureValue(fid, *sca);
    const std::string& name = f->instanceNames[fv];
    lua_pushlstring(l, name.c_str(), name.size());
    return 1;
  }
  // ======================== Feature ===============================
  // name = feature:getName()
  int featureGetName(lua_State* l) {
    Feature* f = checkForFeature(l, 1);
    lua_pushlstring(l, f->featureName.c_str(), f->featureName.size());
    return 1;
  }
  // instanceNames = feature:getInstanceNames()
  int featureGetInstanceNames(lua_State* l) {
    Feature* f = checkForFeature(l, 1);
    lua_newtable(l);
    for (size_t i = 0; i < f->instanceNames.size(); ++i) {
      lua_pushlstring(l,
        f->instanceNames[i].c_str(), f->instanceNames[i].size());
      lua_seti(l, -2, i + 1);
    }
    return 1;
  }
  // default = feature:getDefault()
  int featureGetDefault(lua_State* l) {
    Feature* f = checkForFeature(l, 1);
    lua_pushinteger(l, f->def + 1);
    return 1;
  }
  // isCore = feature:isCore()
  int featureIsCore(lua_State* l) {
    Feature* f = checkForFeature(l, 1);
    lua_pushboolean(l, f->isCore);
    return 1;
  }
  // isCore = feature:isOrdered()
  int featureIsOrdered(lua_State* l) {
    Feature* f = checkForFeature(l, 1);
    lua_pushboolean(l, f->ordered);
    return 1;
  }
  // ======================== CharClass =============================
  // name = cc:getName()
  int ccGetName(lua_State* l) {
    CharClass* cc = checkForCharClass(l, 1);
    lua_pushlstring(l, cc->name.c_str(), cc->name.size());
    return 1;
  }
  // ======================== Library info ==========================
  static const luaL_Reg scaMethods[] = {
    {"getPhoneme", scaGetPhoneme},
    {"getFeature", scaGetFeature},
    {"getClass", scaGetClass},
    {"getFeatureByIndex", scaGetFeatureByIndex},
    {"getClassByIndex", scaGetClassByIndex},
    {nullptr, nullptr}
  };
  static const luaL_Reg psMethods[] = {
    {"getName", psGetName},
    {"getCharClass", psGetCharClass},
    {"getCharClassIndex", psGetCharClassIndex},
    {"getFeatureValue", psGetFeatureValue},
    {"getFeatureName", psGetFeatureName},
    {nullptr, nullptr}
  };
  static const luaL_Reg featureMethods[] = {
    {"getName", featureGetName},
    {"getInstanceNames", featureGetInstanceNames},
    {"getDefault", featureGetDefault},
    {"isCore", featureIsCore},
    {"isOrdered", featureIsOrdered},
    {nullptr, nullptr}
  };
  static const luaL_Reg charClassMethods[] = {
    {"getName", ccGetName},
    {nullptr, nullptr}
  };
  // ======================== Call these! ===========================
  // Initialise SCA library
  int init(lua_State* l) {
    // SCA
    luaL_newmetatable(l, SCA_METATABLE_NAME);
    lua_pushstring(l, "__index");
    lua_pushvalue(l, -2);
    lua_settable(l, -3);
    luaL_setfuncs(l, scaMethods, 0);
    // PhonemeSpec
    luaL_newmetatable(l, SCA_PHONEME_METATABLE_NAME);
    lua_pushstring(l, "__index");
    lua_pushvalue(l, -2);
    lua_settable(l, -3);
    luaL_setfuncs(l, psMethods, 0);
    // Feature
    luaL_newmetatable(l, SCA_FEATURE_METATABLE_NAME);
    lua_pushstring(l, "__index");
    lua_pushvalue(l, -2);
    lua_settable(l, -3);
    luaL_setfuncs(l, featureMethods, 0);
    // CharClass
    luaL_newmetatable(l, SCA_CHARCLASS_METATABLE_NAME);
    lua_pushstring(l, "__index");
    lua_pushvalue(l, -2);
    lua_settable(l, -3);
    luaL_setfuncs(l, charClassMethods, 0);
    luaL_newmetatable(l, SCA_RULE_METATABLE_NAME);
    // register functions here...
    return 1;
  }
}
