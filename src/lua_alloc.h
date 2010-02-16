/*
* lua_alloc.h
* lua_Alloc definition for lua-less builds (based on Lua manual)
* See copyright notice in luabins.h
*/

#ifndef LUABINS_LUA_ALLOC_H_INCLUDED_
#define LUABINS_LUA_ALLOC_H_INCLUDED_

typedef void * (*lua_Alloc) (void *ud,
                             void *ptr,
                             size_t osize,
                             size_t nsize)
                             ;

#endif /* LUABINS_LUA_ALLOC_H_INCLUDED_ */
