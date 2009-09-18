/*
* save.c
* Luabins save code
* See copyright notice in luabins.h
*/

#include "luaheaders.h"

#include "luabins.h"
#include "saveload.h"

/* Arbitrary number of stack slots to be available on save of each element */
#define LUABINS_EXTRASTACK (10)

/* Arbitrary number of used stack slots to trigger preliminary concatenation */
/* TODO: Should be dependent on LUAI_MAXCSTACK? */
#define LUABINS_CONCATTHRESHOLD (1024)

/* NOTE: Overhead on string internalization.
   Use own growing buffer instead of Lua stack.
*/
#define push_bytes(L, p, n) \
  lua_pushlstring((L), (char *)(p), (n))

#define push_byte(L, byte) \
  { char c = (byte); push_bytes((L), &c, 1); }

static int save_value(lua_State * L, int index, int nesting);

/* If retain is 1, retains the top element on stack (slow) */
static void maybe_concat(lua_State * L, int base, int retain)
{
  int top = lua_gettop(L);
  if (top - base >= LUABINS_CONCATTHRESHOLD)
  {
    if (retain)
    {
      lua_insert(L, base);
    }

    lua_concat(L, top - base);

    if (retain)
    {
      /* swap result with retained element */
      lua_pushvalue(L, -2);
      lua_remove(L, -3);
    }
  }
}

/* Returns 0 on success, non-zero on failure */
static int save_table(lua_State * L, int index, int nesting)
{
  int result = LUABINS_ESUCCESS;
  int array_size = 0;
  int array_size_pos = 0;
  int hash_size_pos = 0;
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

  /*
    Note that array size may get corrected below
    as lua_objlen() may return extra for array holes.
  */
  array_size = (int)lua_objlen(L, index);

  lua_pushnil(L); /* placeholder for array size */
  array_size_pos = lua_gettop(L);

  lua_pushnil(L); /* placeholder for hash size */
  hash_size_pos = array_size_pos + 1;

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
      maybe_concat(L, value_pos, 0);
      result = save_value(L, value_pos, nesting);
    }

    if (result == LUABINS_ESUCCESS)
    {
      /* Remove value from stack. */
      lua_remove(L, value_pos);

      /* Move key to the top for the next iteration. */
      lua_pushvalue(L, key_pos);
      lua_remove(L, key_pos);

      /* Value could be a table as well.
         If stack is too cluttered, concat it
         while retaining our key on top.
      */
      maybe_concat(L, hash_size_pos + 1, 1);
    }

    ++total_size;
  }

  if (result == LUABINS_ESUCCESS)
  {
    /*
      Note that if array has holes, lua_objlen may report
      larger than actual array size. So we need to adjust.
    */
    int hash_size = 0;

    array_size = luabins_min(total_size, array_size);
    hash_size = luabins_max(0, total_size - array_size);

    push_bytes(L, (unsigned char *)&array_size, LUABINS_LINT);
    lua_replace(L, array_size_pos);

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
  unsigned char num_to_save = 0;
  int index = index_from;
  int base = lua_gettop(L);

  if (index_to - index_from > LUABINS_MAXTUPLE)
  {
    lua_pushliteral(L, "can't save that many items");
    return LUABINS_EFAILURE;
  }

  /* Allowing to call luabins_save(L, 1, lua_gettop(L))
     from C function, called from Lua with no arguments
     (when lua_gettop() would return 0)
  */
  if (index_to < index_from)
  {
    index_from = 0;
    index_to = 0;
    num_to_save = 0;
  }
  else
  {
    if (
        index_from < 0 || index_from > base ||
        index_to < 0 || index_to > base
      )
    {
      lua_pushliteral(L, "inexistant indices");
      return LUABINS_EFAILURE;
    }

    num_to_save = index_to - index_from + 1;
  }

  push_byte(L, num_to_save);
  for ( ; index <= index_to; ++index)
  {
    int result = 0;

    /* Check if stack has become too huge. */
    maybe_concat(L, base, 0);

    result = save_value(L, index, 0);
    if (result != LUABINS_ESUCCESS)
    {
      lua_settop(L, base); /* Discard intermediate results */
      switch (result)
      {
      case LUABINS_EBADTYPE:
        lua_pushliteral(L, "can't save: unsupported type detected");
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
  }

  lua_concat(L, lua_gettop(L) - base);

  return LUABINS_ESUCCESS;
}
