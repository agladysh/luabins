/*
* saveload.h
* Luabins internal constants
* See copyright notice in luabins.h
*/

#ifndef LUABINS_SAVELOAD_H_
#define LUABINS_SAVELOAD_H_

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

#endif /* LUABINS_SAVELOAD_H_ */
