local TARGET_NAME = os.scriptdir():match(".+[/\\]([%w_]+)")
local LIB_DIR = "$(buildir)/".. TARGET_NAME .. "/"

target(TARGET_NAME)
    set_kind("static")
    set_targetdir(LIB_DIR)
    set_warnings("error")
    local chip_target = nil
    if has_config("chip_target") then chip_target = get_config("chip_target") end
    -- 程序区缩小到1b7000，剩余2ca000 - 1b7000 = 113000
    if chip_target == "ec718p" then
        add_defines("AP_FLASH_LOAD_SIZE=0x1b7000",{public = true})
        add_defines("AP_PKGIMG_LIMIT_SIZE=0x1b7000",{public = true})
        add_defines("FULL_OTA_SAVE_ADDR=0x235000",{public = true})
    end
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