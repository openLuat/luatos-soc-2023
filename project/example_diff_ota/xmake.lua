local TARGET_NAME = os.scriptdir():match(".+[/\\]([%w_]+)")
local LIB_DIR = "$(buildir)/".. TARGET_NAME .. "/"
local LIB_NAME = "lib" .. TARGET_NAME .. ".a "
local CHIP_TARGET = CHIP_TARGET
target(TARGET_NAME)
    set_kind("static")
    set_targetdir(LIB_DIR)
    set_warnings("error")

    --加入代码和头文件
    add_includedirs("./inc",{public = true})
    add_files("./src/*.c",{public = true})

	--ymodem
    add_includedirs(LUATOS_ROOT .. "/components/ymodem")
    add_files(LUATOS_ROOT .. "/components/ymodem/luat_ymodem.c")

    --http
    add_includedirs(LUATOS_ROOT.."/components/network/libhttp", {public = true})
    add_includedirs(LUATOS_ROOT.."/components/network/http_parser", {public = true})
    add_files(LUATOS_ROOT.."/components/network/libhttp/*.c")
    remove_files(LUATOS_ROOT.."/components/network/libhttp/luat_lib_http.c")
    add_files(LUATOS_ROOT.."/components/network/http_parser/*.c")

target_end()