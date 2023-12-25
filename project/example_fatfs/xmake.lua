local TARGET_NAME = os.scriptdir():match(".+[/\\]([%w_]+)")
local LIB_DIR = "$(buildir)/".. TARGET_NAME .. "/"
local LIB_NAME = "lib" .. TARGET_NAME .. ".a "

target(TARGET_NAME)
    set_kind("static")
    set_targetdir(LIB_DIR)
    set_warnings("error")
    --加入代码和头文件
    add_defines("LUAT_USE_FATFS",{public = true})
    add_files(LUATOS_ROOT.."/luat/vfs/luat_fs_fatfs.c")

    add_includedirs(LUATOS_ROOT.."/components/fatfs",{public = true})
    add_files(LUATOS_ROOT.."/components/fatfs/*.c")
    remove_files(LUATOS_ROOT.."/components/fatfs/luat_lib_fatfs.c")
    add_includedirs("./inc",{public = true})
    add_files("./src/**.c")

target_end()