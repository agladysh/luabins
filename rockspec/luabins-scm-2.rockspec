package = "luabins"
version = "scm-2"
source = {
   url = "git://github.com/agladysh/luabins.git"
}
description = {
   summary = "Trivial Lua Binary Serialization Library",
   detailed = [[
      Luabins allows to save tuples of primitive Lua types into binary chunks and to load saved data back.
   ]],
   homepage = "http://github.com/agladysh/luabins",
   license = "MIT/X11"
}
dependencies = {
   "lua >= 5.1"
}
build = {
   type = "builtin",
   modules = {
      luabins = {
         sources = {
            "src/load.c",
            "src/luabins.c",
            "src/luainternals.c",
            "src/lualess.c",
            "src/save.c",
            "src/savebuffer.c",
            "src/write.c"
         },
         incdirs = {
            "src/"
         }
      }
   }
}
