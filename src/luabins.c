/*
* luabins.c
* Luabins implementation
* See copyright notice in luabins.h
*/

#include <string.h>

#include <lua.h>
#include <lauxlib.h>

#include "luabins.h"

#define LUABINS_ESUCCESS (0)
#define LUABINS_EFAILURE (1)
#define LUABINS_EBADTYPE (2)
#define LUABINS_ETOODEEP (3)
#define LUABINS_ENOSTACK (4)
#define LUABINS_EBADDATA (5)

#define LUABINS_CNIL    '-'
#define LUABINS_CFALSE  '0'
#define LUABINS_CTRUE   '1'
#define LUABINS_CNUMBER 'N'
#define LUABINS_CSTRING 'S'
#define LUABINS_CTABLE  'T'

/* Arbitrary number of stack slots to be available on save of each element */
#define LUABINS_EXTRASTACK (10)

/* Arbitrary number of used stack slots to trigger preliminary concatenation */
#define LUABINS_CONCATTHRESHOLD (LUAI_MAXCSTACK / 2)

/*
* PORTABILITY WARNING!
* You have to ensure manually that length constants below are the same
* for both code that does the save and code that does the load.
* Also these constants must be actual or code below would break.
* Beware of endianness and lua_Number actual type as well.
*/
#define LUABINS_LINT    (sizeof(int))
#define LUABINS_LSIZET  (sizeof(size_t))
#define LUABINS_LNUMBER (sizeof(lua_Number))

/* NOTE: Overhead on string internalization.
   Use own growing buffer instead of Lua stack.
*/
#define push_bytes(L, p, n) \
  lua_pushlstring((L), (char *)(p), (n))

#define push_byte(L, byte) \
  { char c = (byte); push_bytes((L), &c, 1); }

static int save_value(lua_State * L, int index, int nesting);

#define maybe_concat(l, b) \
  { \
    lua_State * L = (l); int base = (b); \
    int top = lua_gettop(L); \
    if (top - base >= LUABINS_CONCATTHRESHOLD) \
    { \
      lua_concat(L, top - base); \
    } \
  }

/* Returns 0 on success, non-zero on failure */
static int save_table(lua_State * L, int index, int nesting)
{
  int result = LUABINS_ESUCCESS;
  int hash_size_pos = 0;
  int array_size = 0;
  int total_size = 0;

  if (nesting > LUABINS_MAXTABLENESTING)
  {
    return LUABINS_ETOODEEP;
  }

  /* TODO: Hauling stack for key and value removal
     may get too heavy for larger tables. Think out a better way.
  */

  /* If __len metamethod for tables would ever work, this would be broken.
     Note also inelegant downsize from size_t to int.
     TODO: Handle integer overflow here.
  */
  array_size = (int)lua_objlen(L, index);
  push_bytes(L, (unsigned char *)&array_size, LUABINS_LINT);

  lua_pushnil(L); /* placeholder for hash size */
  hash_size_pos = lua_gettop(L);

  lua_pushnil(L); /* key for lua_next() */
  while (result == LUABINS_ESUCCESS && lua_next(L, index) != 0)
  {
    int value_pos = lua_gettop(L);
    int key_pos = value_pos - 1;

    /* Save key. */
    result = save_value(L, key_pos, nesting);

    /* Save value. */
    if (result == LUABINS_ESUCCESS)
    {
      /* Key may be a table with a lot of elements
         and generate a lot of cruft on stack.
      */
      maybe_concat(L, value_pos);
      result = save_value(L, value_pos, nesting);
    }

    if (result == LUABINS_ESUCCESS)
    {
      /* Value may be a table as well. */
      maybe_concat(L, value_pos);

      /* Remove value from stack. */
      lua_remove(L, value_pos);

      /* Move key to the top for the next iteration. */
      lua_pushvalue(L, key_pos);
      lua_remove(L, key_pos);
    }

    ++total_size;
  }

  if (result == LUABINS_ESUCCESS)
  {
    int hash_size = total_size - array_size;
    push_bytes(L, (unsigned char *)&hash_size, LUABINS_LINT);
    lua_replace(L, hash_size_pos);
  }

  return result;
}

/* Returns 0 on success, non-zero on failure */
static int save_value(lua_State * L, int index, int nesting)
{
  /* Ensure that we have have some extra space on stack */
  if (lua_checkstack(L, LUABINS_EXTRASTACK) == 0)
  {
    return LUABINS_ENOSTACK;
  }

  switch (lua_type(L, index))
  {
  case LUA_TNIL:
    push_byte(L, LUABINS_CNIL);
    break;

  case LUA_TBOOLEAN:
    push_byte(
        L,
        (lua_toboolean(L, index) == 0) ? LUABINS_CFALSE : LUABINS_CTRUE
      );
    break;

  case LUA_TNUMBER:
    {
      lua_Number num = lua_tonumber(L, index);
      push_byte(L, LUABINS_CNUMBER);
      push_bytes(L, (unsigned char *)&num, LUABINS_LNUMBER);
    }
    break;

  case LUA_TSTRING:
    {
      size_t len = lua_objlen(L, index);
      push_byte(L, LUABINS_CSTRING);
      push_bytes(L, (unsigned char *)&len, LUABINS_LSIZET);
      lua_pushvalue(L, index);
    }
    break;

  case LUA_TTABLE:
    {
      int result = LUABINS_ESUCCESS;
      push_byte(L, LUABINS_CTABLE);
      result = save_table(L, index, nesting + 1);
      if (result != LUABINS_ESUCCESS)
      {
        return result;
      }
    }
    break;

  case LUA_TNONE:
  case LUA_TFUNCTION:
  case LUA_TTHREAD:
  case LUA_TUSERDATA:
  default:
    return LUABINS_EBADTYPE;
  }

  return LUABINS_ESUCCESS;
}

int luabins_save(lua_State * L, int index_from, int index_to)
{
  unsigned char num_to_save = index_to - index_from;
  int index = index_from;
  int base = lua_gettop(L);

  if (index_to - index_from > LUABINS_MAXTUPLE)
  {
    lua_pushnil(L); lua_pushliteral(L, "can't save that many items");
    return LUABINS_EFAILURE;
  }

  if (index_to < index_from)
  {
    /* Allowing to call luabins_save(L, 1, lua_gettop(L))
       from C function, called from Lua with no arguments
       (when lua_gettop() would return 0)
    */
    num_to_save = 0;
  }

  push_byte(L, num_to_save);
  for ( ; index <= index_to; ++index)
  {
    int result = save_value(L, index, 0);
    if (result != LUABINS_ESUCCESS)
    {
      lua_settop(L, base); /* Discard intermediate results */
      lua_pushnil(L);
      switch (result)
      {
      case LUABINS_EBADTYPE:
        lua_pushfstring(
            L,
            "can't save unsupported type" LUA_QL("%s"),
            luaL_typename(L, index)
          );
        break;

      case LUABINS_ETOODEEP:
        lua_pushliteral(L, "can't save: nesting is too deep");
        break;

      case LUABINS_ENOSTACK:
        lua_pushliteral(L, "can't save: can't grow stack");
        break;

      default: /* Should not happen */
        lua_pushliteral(L, "save failed");
        break;
      }

      return result;
    }

    /* Check if stack has become too huge. */
    /* TODO: Should it be here? Perhaps checks in save_table() are enough. */
    maybe_concat(L, base);
  }

  lua_concat(L, lua_gettop(L) - base);

  return LUABINS_ESUCCESS;
}

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
static const struct luaL_reg luabins_funcs[] =
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
  luaL_register(L, "luabins", luabins_funcs);
  lua_pushliteral(L, LUABINS_VERSION);
  lua_setfield(L, -1, "VERSION");

  return 1;
}

#ifdef __cplusplus
}
#endif
