/*
* lua_alloc.h
* lua_Alloc definition for lua-less builds (based on Lua manual)
* See copyright notice in luabins.h
*/

typedef void * (*lua_Alloc) (void *ud,
                             void *ptr,
                             size_t osize,
                             size_t nsize)
                             ;
