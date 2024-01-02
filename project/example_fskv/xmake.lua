local TARGET_NAME = os.scriptdir():match(".+[/\\]([%w_]+)")
local LIB_DIR = "$(buildir)/".. TARGET_NAME .. "/"

target(TARGET_NAME)
    set_kind("static")
    set_targetdir(LIB_DIR)
    set_warnings("error")
    --加入代码和头文件
    add_includedirs("./inc",{public = true})
    add_files("./src/*.c",{public = true})

    add_includedirs(LUATOS_ROOT.."/components/fskv", {public = true})
    add_files(LUATOS_ROOT.."/components/fskv/*.c")
    remove_files(LUATOS_ROOT.."/components/fskv/luat_lib_fskv.c")

target_end()