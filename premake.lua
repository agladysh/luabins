addoption("test", "Build test configuration")

project.configs = { "Release" }

local make_luabins_package = function(kind, name, language, standard, bindir)
  local package = newpackage()
  package.name = name
  package.kind = kind
  package.language = language
  package.files = { matchfiles("src/*.h", "src/*.c") }
  package.targetprefix = ""
  package.bindir = bindir

  if linux or macosx then
    table.insert(
        package.buildoptions,
        "-O3 -pedantic -Werror -Wall -std="..standard.." -x "..language
      )
  end

  if macosx then
    table.insert(package.linkoptions, "-bundle -undefined dynamic_lookup")
  end

  return package
end

if not options.test then
  make_luabins_package("dll", "luabins", "c", "c89", ".")
else
  local make_test_package = function(kind, language, standard, nopostbuild)
    local name = "luabins"
    local path = "./test/build/"..standard
    local package = make_luabins_package(
        kind,
        name.."-"..standard.."-"..kind,
        language,
        standard,
        path
      )

    package.target = name

    if not nopostbuild then
      table.insert(
          package.postbuildcommands,
          "@echo '***** Begin Lua tester ".. standard .." *****' 1>&2;"
          .. "cd "..path.." 1>&1;"
          .. " lua -l"..name.." ../../test.lua &&"
          .. " (cd -; "
          .. " echo '***** Done Lua tester ".. standard .." *****' 1>&2) ||"
          .. " (cd -; "
          .. " echo '***** ERROR Lua tester ".. standard .." *****' 1>&2;"
          .. " exit 1)"
        )

      -- TODO: Premake ignores this!
      --table.insert(package.files, "test/test.lua")
    end

    return package
  end

  local make_tester_package = function(language, standard)
    local name = "luabins-tester"
    local path = "./test/build/"..standard

    make_test_package("dll", language, standard, false)
    do
      local package = make_test_package("lib", language, standard, true)
      package.libdir = path
      package.targetprefix = "lib"
    end

    local package = newpackage()
    package.name = name .. "-" .. standard
    package.target = "test"
    package.kind = "exe"
    package.language = language -- Premake ignores this
    package.files = { "./test/test.c" }
    package.targetprefix = ""
    package.bindir = path

    if linux or macosx then
      table.insert(
          package.buildoptions,
          "-O3 -pedantic -Werror -Wall -std="..standard.." -x"..language
        )
    end

    table.insert(package.links, { "luabins", "lua" })

    table.insert(package.includepaths, "./src")

    table.insert(
        package.postbuildcommands,
        "@echo '***** Launching C tester ".. standard .." *****' 1>&2;"
        .. "cd "..path.." 1>&1;"
        .. " ./test &&"
        .. " (cd -; "
        .. " echo '***** Done C tester ".. standard .." *****' 1>&2) ||"
        .. " (cd -; "
        .. " echo '***** ERROR C tester ".. standard .." *****' 1>&2;"
        .. " exit 1)"
      )

    return package
  end

  make_luabins_package("dll", "luabins", "c", "c89", ".")

  make_tester_package("c", "c89")
  make_tester_package("c", "c99")
  make_tester_package("c++", "c++98")
end
