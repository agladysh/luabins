/*
* save.c
* Luabins save code
* See copyright notice in luabins.h
*/

#include "luaheaders.h"

#include "luabins.h"
#include "saveload.h"
#include "savebuffer.h"

/* TODO: Test this with custom allocator! */

#if 1
  #define SPAM(a) printf a
#else
  #define SPAM(a) (void)0
#endif

#define push_bytes lbsSB_write

#define overwrite_bytes lbsSB_overwrite

int push_byte(luabins_SaveBuffer * sb, unsigned char b)
{
  return push_bytes(sb, &b, 1);
}

static int save_value(
    lua_State * L,
    luabins_SaveBuffer * sb,
    int index,
    int nesting
  );

/* Returns 0 on success, non-zero on failure */
static int save_table(
    lua_State * L,
    luabins_SaveBuffer * sb,
    int index,
    int nesting
  )
{
  int result = LUABINS_ESUCCESS;
  int array_size_pos = 0;
  int hash_size_pos = 0;
  int total_size = 0;
  int base = lua_gettop(L);

  if (nesting > LUABINS_MAXTABLENESTING)
  {
    return LUABINS_ETOODEEP;
  }

  /* TODO: Hauling stack for key and value removal
     may get too heavy for larger tables. Think out a better way.
  */

  result = lbsSB_grow(sb, LUABINS_LINT + LUABINS_LINT);
  if (result == LUABINS_ESUCCESS)
  {
    int zero_size_dummy = 0;

    array_size_pos = lbsSB_length(sb);
    push_bytes(sb, (const unsigned char *)&zero_size_dummy, LUABINS_LINT);

    hash_size_pos = lbsSB_length(sb);
    push_bytes(sb, (const unsigned char *)&zero_size_dummy, LUABINS_LINT);

    lua_pushnil(L); /* key for lua_next() */
  }

  lua_checkstack(L, 2); /* Key and value */
  while (result == LUABINS_ESUCCESS && lua_next(L, index) != 0)
  {
    int value_pos = lua_gettop(L); /* We need absolute values */
    int key_pos = value_pos - 1;

    SPAM(("base %d, value_pos %d\n", base, value_pos));

    /* Save key. */
    result = save_value(L, sb, key_pos, nesting);

    /* Save value. */
    if (result == LUABINS_ESUCCESS)
    {
      result = save_value(L, sb, value_pos, nesting);
    }

    if (result == LUABINS_ESUCCESS)
    {
      /* Remove value from stack, leave key for the next iteration. */
      lua_pop(L, 1);
      ++total_size;
    }
  }

  if (result == LUABINS_ESUCCESS)
  {
    /*
      Note that if array has holes, lua_objlen() may report
      larger than actual array size. So we need to adjust.

      TODO: Note inelegant downsize from size_t to int.
            Handle integer overflow here.
    */
    int array_size = luabins_min(total_size, (int)lua_objlen(L, index));
    int hash_size = luabins_max(0, total_size - array_size);

    /* Overwrites should not fail, as we reserved size above */
    overwrite_bytes(
        sb,
        array_size_pos,
        (const unsigned char *)&array_size,
        LUABINS_LINT
      );

    overwrite_bytes(
        sb,
        hash_size_pos,
        (const unsigned char *)&hash_size,
        LUABINS_LINT
      );
  }

  SPAM(("base %d, cur %d\n", base, lua_gettop(L)));

  return result;
}

/* Returns 0 on success, non-zero on failure */
static int save_value(
    lua_State * L,
    luabins_SaveBuffer * sb,
    int index,
    int nesting
  )
{
  int result = LUABINS_ESUCCESS;

  switch (lua_type(L, index))
  {
  case LUA_TNIL:
    result = push_byte(sb, LUABINS_CNIL);
    break;

  case LUA_TBOOLEAN:
    result = push_byte(
        sb,
        (lua_toboolean(L, index) == 0)
          ? LUABINS_CFALSE
          : LUABINS_CTRUE
      );
    break;

  case LUA_TNUMBER:
    {
      lua_Number num = lua_tonumber(L, index);
      result = lbsSB_grow(sb, 1 + LUABINS_LNUMBER);
      if (result == LUABINS_ESUCCESS)
      {
        push_byte(sb, LUABINS_CNUMBER);
        push_bytes(sb, (const unsigned char *)&num, LUABINS_LNUMBER);
      }
    }
    break;

  case LUA_TSTRING:
    {
      size_t len = 0;
      const char * buf = lua_tolstring(L, index, &len);

      result = lbsSB_grow(sb, 1 + LUABINS_LSIZET + len);
      if (result == LUABINS_ESUCCESS)
      {
        push_byte(sb, LUABINS_CSTRING);
        push_bytes(sb, (const unsigned char *)&len, LUABINS_LSIZET);
        push_bytes(sb, (const unsigned char *)buf, len);
      }
    }
    break;

  case LUA_TTABLE:
    {
      result = push_byte(sb, LUABINS_CTABLE);
      if (result == LUABINS_ESUCCESS)
      {
        result = save_table(L, sb, index, nesting + 1);
      }
    }
    break;

  case LUA_TNONE:
  case LUA_TFUNCTION:
  case LUA_TTHREAD:
  case LUA_TUSERDATA:
  default:
    result = LUABINS_EBADTYPE;
  }

  return result;
}

int luabins_save(lua_State * L, int index_from, int index_to)
{
  unsigned char num_to_save = 0;
  int index = index_from;
  int base = lua_gettop(L);
  luabins_SaveBuffer sb;

  /*
  * TODO: If lua_error() would happen below, would leak the buffer.
  */

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
      lua_pushliteral(L, "can't save: inexistant indices");
      return LUABINS_EFAILURE;
    }

    num_to_save = index_to - index_from + 1;
  }

  {
    void * alloc_ud = NULL;
    lua_Alloc alloc_fn = lua_getallocf(L, &alloc_ud);
    lbsSB_init(&sb, alloc_fn, alloc_ud, LUABINS_SAVEBLOCKSIZE);
  }

  push_byte(&sb, num_to_save);
  for ( ; index <= index_to; ++index)
  {
    int result = 0;

    result = save_value(L, &sb, index, 0);
    if (result != LUABINS_ESUCCESS)
    {
      switch (result)
      {
      case LUABINS_EBADTYPE:
        lua_pushliteral(L, "can't save: unsupported type detected");
        break;

      case LUABINS_ETOODEEP:
        lua_pushliteral(L, "can't save: nesting is too deep");
        break;

      case LUABINS_ETOOLONG:
        lua_pushliteral(L, "can't save: not enough memory");
        break;

      default: /* Should not happen */
        lua_pushliteral(L, "save failed");
        break;
      }

      lbsSB_destroy(&sb);

      return result;
    }
  }

  {
    size_t len = 0UL;
    const unsigned char * buf = lbsSB_buffer(&sb, &len);
    lua_pushlstring(L, (const char *)buf, len);
  }

  return LUABINS_ESUCCESS;
}
