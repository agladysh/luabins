package.name = "luabins"
package.kind = "dll"
package.language = "c"
package.files = { "src/luabins.c", "src/luabins.h" }
package.targetprefix = ""
package.bindir = "./"

if linux or macosx then
  table.insert(package.buildoptions, "-pedantic -Werror -Wall -std=c89")
end

if macosx then
  table.insert(package.linkoptions, "-bundle -undefined dynamic_lookup")
  -- Ports default Lua installation
  table.insert(package.libpaths, "/opt/local/lib")
end
