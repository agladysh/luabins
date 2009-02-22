#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#ifdef __cplusplus
}
#endif /* __cplusplus */

#include "luabins.h"

#define STACKGUARD1 "-- stack ends here (1) --"
#define STACKGUARD2 "-- stack ends here (2) --"

/* Note this one does not dump values to protect from embedded zeroes. */
int dump_lua_stack(lua_State * L, int base)
{
  int top = lua_gettop(L);

  if (top == 0)
  {
    lua_pushliteral(L, "-- stack is empty --");
  }
  else
  {
    int pos = 0;
    luaL_Buffer b;

    luaL_buffinit(L, &b);

    for (pos = top; pos > 0; --pos)
    {
      luaL_addstring(&b, (pos != base) ? "[" : "{");
      lua_pushinteger(L, pos);
      luaL_addvalue(&b);
      luaL_addstring(&b, (pos != base) ? "] - " : "} -");
      luaL_addstring(&b, luaL_typename(L, pos));
      luaL_addstring(&b, "\n");
    }

    luaL_pushresult(&b);
  }

  if (lua_gettop(L) != top + 1)
  {
    return luaL_error(L, "dumpstack not balanced %d %d", top, lua_gettop(L));
  }

  return 1;
}

void fatal(lua_State * L, const char * msg)
{
  dump_lua_stack(L, 0);
  fprintf(stderr, "%s\nSTACK\n%s", msg, lua_tostring(L, -1));
  lua_pop(L, 1);
  fflush(stderr);
  exit(1);
}

void check(lua_State * L, int base, int extra)
{
  int top = lua_gettop(L);
  if (top != base + extra)
  {
    fatal(L, "stack unbalanced");
  }

  lua_pushliteral(L, STACKGUARD1);
  if (lua_rawequal(L, -1, base) == 0)
  {
    fatal(L, "stack guard corrupted");
  }
  lua_pop(L, 1);
}

void checkerr(lua_State * L, int base, const char * err)
{
  int top = lua_gettop(L);
  if (top != base + 1)
  {
    fatal(L, "stack unbalanced on error");
  }

  lua_pushliteral(L, STACKGUARD1);
  if (lua_rawequal(L, -1, base) == 0)
  {
    fatal(L, "stack guard corrupted");
  }
  lua_pop(L, 1);

  lua_pushstring(L, err);
  if (lua_rawequal(L, -1, -2) == 0)
  {
    fprintf(stderr, "actual:   '%s'\n", lua_tostring(L, -2));
    fprintf(stderr, "expected: '%s'\n", err);
    fatal(L, "error message mismatch");
  }
  lua_pop(L, 2); /* Pops error message as well */
}

int main()
{
  int base = 0;
  int count = 0;
  const unsigned char * str;
  size_t length = 0;

  lua_State * L = lua_open();
  luaL_openlibs(L);

  printf("STDC %d %d\n", __STDC__, __STRICT_ANSI__);

#ifdef __GNUG__
  printf("GNUG ON\n");
#else
  printf("GNUG OFF\n");
#endif /* __GNUG__ */

#ifdef __cplusplus
  printf("luabins C API test compiled as C++\n");
#else
  printf("luabins C API test compiled as plain C\n");
#endif /* __cplusplus */

  /* Push stack check value */
  lua_pushliteral(L, STACKGUARD1);
  base = lua_gettop(L);

  /* Sanity check */
  check(L, base, 0);

  /* Save error: inexistant index */
  if (luabins_save(L, lua_gettop(L) + 1, lua_gettop(L) + 1) == 0)
  {
    fatal(L, "save should fail");
  }

  checkerr(L, base, "inexistant indices");

  if (luabins_save(L, -1, -1) == 0)
  {
    fatal(L, "save should fail");
  }

  checkerr(L, base, "inexistant indices");

  /* Assuming other save errors to be tested in test.lua */

  /* Trigger load error */

  if (luabins_load(L, (const unsigned char *)"", 0, &count) == 0)
  {
    fatal(L, "load should fail");
  }

  checkerr(L, base, "corrupt data");

  /* Assuming other load errors to be tested in test.lua */

  /* Do empty save */
  if (luabins_save(L, base, base - 1) != 0)
  {
    fatal(L, "empty save failed");
  }
  check(L, base, 1);

  str = (const unsigned char *)lua_tolstring(L, -1, &length);
  if (str == NULL || length == 0)
  {
    fatal(L, "bad empty save string");
  }

  /* Load saved data */

  if (luabins_load(L, str, length, &count) != 0)
  {
    fatal(L, "empty load failed");
  }

  if (count != 0)
  {
    fatal(L, "bad empty load count");
  }

  /* Pop saved data string */
  check(L, base, 1);
  lua_pop(L, 1);
  check(L, base, 0);

  /* Save all good types */

  /* Load saved data */

  lua_close(L);

  /* TODO: Test saving at negative indexes */

  return 0;
}
