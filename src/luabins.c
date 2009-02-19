/*
* luabins.c
* Luabins Lua module code
* See copyright notice in luabins.h
*/

#include <lua.h>
#include <lauxlib.h>

#include "luabins.h"

static int l_save(lua_State * L)
{
  return luabins_save(L, 1, lua_gettop(L)) == 0 ? 1 : 2;
}

static int l_load(lua_State * L)
{
  int count = 0;

  luaL_checktype(L, 1, LUA_TSTRING);

  return luabins_load_inplace(L, 1, &count) == 0 ? count : 2;
}

/* luabins Lua module API */
static const struct luaL_reg R[] =
{
	{ "save", l_save },
	{ "load", l_load },
	{ NULL, NULL }
};

#ifdef __cplusplus
extern "C" {
#endif

LUALIB_API int luaopen_luabins(lua_State * L)
{
  luaL_register(L, "luabins", R);
  lua_pushliteral(L, LUABINS_VERSION);
  lua_setfield(L, -2, "VERSION");

  return 1;
}

#ifdef __cplusplus
}
#endif
