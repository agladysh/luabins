/*
* load.c
* Luabins load code
* See copyright notice in luabins.h
*/

#include <string.h>

#include <lua.h>
#include <lauxlib.h>

#include "luabins.h"
#include "saveload.h"

typedef struct lbs_LoadState
{
  unsigned char * pos;
  size_t unread;
} lbs_LoadState;

void lbsLS_init(lbs_LoadState * ls, unsigned char * data, size_t len)
{
  ls->pos = data;
  ls->unread = len;
}

#define lbsLS_good(ls) \
  ((ls)->unread > 0)

static unsigned char lbsLS_readbyte(lbs_LoadState * ls)
{
  if (lbsLS_good(ls))
  {
    unsigned char b = *ls->pos;
    ++ls->pos;
    --ls->unread;
    return b;
  }
  return 0;
}

static unsigned char * lbsLS_eat(lbs_LoadState * ls, size_t len)
{
  unsigned char * result = NULL;
  if (lbsLS_good(ls))
  {
    if (ls->unread >= len)
    {
      result = ls->pos;
      ls->pos += len;
      ls->unread -= len;
    }
    else
    {
      ls->unread = 0;
      ls->pos = NULL;
    }
  }
  return result;
}

static int lbsLS_readbytes(lbs_LoadState * ls, unsigned char * buf, size_t len)
{
  unsigned char * pos = lbsLS_eat(ls, len);
  if (pos != NULL)
  {
    memcpy(buf, pos, len);
    return LUABINS_ESUCCESS;
  }
  return LUABINS_EBADDATA;
}

static int load_value(lua_State * L, lbs_LoadState * ls);

static int load_table(lua_State * L, lbs_LoadState * ls)
{
  int array_size = 0;
  int hash_size = 0;
  int total_size = 0;

  int result = lbsLS_readbytes(ls, (unsigned char *)&array_size, LUABINS_LINT);
  if (result == LUABINS_ESUCCESS)
  {
    result = lbsLS_readbytes(ls, (unsigned char *)&hash_size, LUABINS_LINT);
  }

  if (result == LUABINS_ESUCCESS)
  {
    int i = 0;
    total_size = array_size + hash_size;

    lua_createtable(L, array_size, hash_size);
    for (i = 0; i < total_size; ++i)
    {
      result = load_value(L, ls); /* Load key. */
      if (result != LUABINS_ESUCCESS)
      {
        break;
      }

      result = load_value(L, ls); /* Load value. */
      if (result != LUABINS_ESUCCESS)
      {
        break;
      }

      lua_rawset(L, -3);
    }
  }


  return result;
}

static int load_value(lua_State * L, lbs_LoadState * ls)
{
  int result = LUABINS_ESUCCESS;
  unsigned char type = lbsLS_readbyte(ls);

  switch (type)
  {
  case LUABINS_CNIL:
    lua_pushnil(L);
    break;

  case LUABINS_CFALSE:
    lua_pushboolean(L, 0);
    break;

  case LUABINS_CTRUE:
    lua_pushboolean(L, 1);
    break;

  case LUABINS_CNUMBER:
    {
      lua_Number value;
      result = lbsLS_readbytes(ls, (unsigned char *)&value, LUABINS_LNUMBER);
      if (result == LUABINS_ESUCCESS)
      {
        lua_pushnumber(L, value);
      }
    }
    break;

  case LUABINS_CSTRING:
    {
      size_t len = 0;
      result = lbsLS_readbytes(ls, (unsigned char *)&len, LUABINS_LSIZET);
      if (result == LUABINS_ESUCCESS)
      {
        unsigned char * pos = lbsLS_eat(ls, len);
        if (pos)
        {
          lua_pushlstring(L, (char *)pos, len);
        }
        else
        {
          result = LUABINS_EBADDATA;
        }
      }
    }
    break;

  case LUABINS_CTABLE:
    result = load_table(L, ls);
    break;

  default:
    result = LUABINS_EBADDATA;
    break;
  }

  return result;
}

int luabins_load(lua_State * L, unsigned char * data, size_t len, int * count)
{
  lbs_LoadState ls;
  int result = LUABINS_ESUCCESS;
  unsigned char num_items = 0;
  int base = lua_gettop(L);
  int i = 0;

  lbsLS_init(&ls, data, len);
  num_items = lbsLS_readbyte(&ls);
  for (
      i = 0;
      i < num_items && result == LUABINS_ESUCCESS;
      ++i
    )
  {
    result = load_value(L, &ls);
  }

  if (!lbsLS_good(&ls))
  {
    result = LUABINS_EBADDATA;
  }

  if (result == LUABINS_ESUCCESS)
  {
    *count = num_items;
  }
  else
  {
    lua_settop(L, base); /* Discard intermediate results */
    lua_pushnil(L);
    switch (result)
    {
    case LUABINS_EBADDATA:
      lua_pushliteral(L, "corrupt data");
      break;

    default: /* Should not happen */
      lua_pushliteral(L, "load failed");
      break;
    }
  }

  return result;
}

int luabins_load_inplace(lua_State * L, int index, int * count)
{
  size_t len = 0;
  unsigned char * data = NULL;

  if (lua_type(L, index) != LUA_TSTRING)
  {
    /* Note we can't use luaL_typerror() here,
       since we may be called from C and luaL_where() may be meaningless.
    */
    lua_pushnil(L); lua_pushliteral(L, "wrong argument type");
    return LUABINS_EFAILURE;
  }

  data = (unsigned char *)lua_tolstring(L, index, &len);

  return luabins_load(L, data, len, count);
}
