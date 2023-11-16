local TARGET_NAME = os.scriptdir():match(".+[/\\]([%w_]+)")
local LIB_DIR = "$(buildir)/".. TARGET_NAME .. "/"
local LIB_NAME = "lib" .. TARGET_NAME .. ".a "
local CHIP_TARGET = CHIP_TARGET

target(TARGET_NAME)
    set_kind("static")
    set_targetdir(LIB_DIR)
    -- set_warnings("error")

    -- 程序区缩小到1b3000，剩余2b3000 - 1b3000 = 100000 1MB空间为用户区
    -- if CHIP_TARGET == "ec718p" then
    --     add_defines("AP_FLASH_LOAD_SIZE=0x1b3000",{public = true})
    --     add_defines("AP_PKGIMG_LIMIT_SIZE=0x1b3000",{public = true})
    -- end

    add_defines("__LUATOS__",{public = true})

    add_includedirs(LUATOS_ROOT.."/lua/include",{public = true})
    add_files(LUATOS_ROOT .. "/lua/src/*.c")
    add_files(LUATOS_ROOT .. "/luat/modules/*.c")
    add_files(LUATOS_ROOT.."/luat/freertos/*.c")
    -- mbedtls
    add_defines("LUAT_USE_TLS",{public = true})
    -- common
    add_includedirs(LUATOS_ROOT.."/components/common",{public = true})
    -- add_files(LUATOS_ROOT.."/components/common/*.c")
    -- mobile
    add_files(LUATOS_ROOT.."/components/mobile/luat_lib_mobile.c")
    -- sms
    add_files(LUATOS_ROOT.."/components/sms/*.c")
    -- rsa
    add_files(LUATOS_ROOT.."/components/rsa/**.c")
    -- cjson
    add_includedirs(LUATOS_ROOT.."/components/cjson",{public = true})
    add_files(LUATOS_ROOT.."/components/cjson/*.c")
    -- lua-cjson
    add_includedirs(LUATOS_ROOT .. "/components/lua-cjson",{public = true})
    add_files(LUATOS_ROOT .. "/components/lua-cjson/*.c")
    -- miniz
    add_includedirs(LUATOS_ROOT .. "/components/miniz",{public = true})
    add_files(LUATOS_ROOT .. "/components/miniz/*.c")
    -- protobuf
    add_includedirs(LUATOS_ROOT.."/components/serialization/protobuf",{public = true})
    add_files(LUATOS_ROOT.."/components/serialization/protobuf/*.c")
    -- vfs
    add_files(LUATOS_ROOT.."/luat/vfs/luat_fs_inline.c",
            LUATOS_ROOT.."/luat/vfs/luat_fs_luadb.c",
            LUATOS_ROOT.."/luat/vfs/luat_inline_libs.c",
            LUATOS_ROOT.."/luat/vfs/luat_fs_mem.c")
    -- fatfs
    add_includedirs(LUATOS_ROOT.."/components/fatfs",{public = true})
    add_files(LUATOS_ROOT.."/components/fatfs/*.c")
    -- sfud
    add_defines("LUAT_USE_SFUD",{public = true})
    add_includedirs(LUATOS_ROOT.."/components/sfud",{public = true})
    add_files(LUATOS_ROOT.."/components/sfud/*.c")
    -- fonts
    add_includedirs(LUATOS_ROOT.."/components/luatfonts",{public = true})
    add_files(LUATOS_ROOT.."/components/luatfonts/*.c")
    -- eink
    add_includedirs(LUATOS_ROOT.."/components/eink",{public = true})
    add_includedirs(LUATOS_ROOT.."/components/epaper",{public = true})
    add_files(LUATOS_ROOT.."/components/eink/*.c")
    add_files(LUATOS_ROOT.."/components/epaper/*.c")
    remove_files(LUATOS_ROOT.."/components/epaper/GUI_Paint.c")
    -- u8g2
    add_includedirs(LUATOS_ROOT.."/components/u8g2", {public = true})
    add_files(LUATOS_ROOT.."/components/u8g2/*.c")
    -- lcd
    add_includedirs(LUATOS_ROOT.."/components/lcd", {public = true})
    add_files(LUATOS_ROOT.."/components/lcd/*.c")
    -- qrcode
    add_includedirs(LUATOS_ROOT.."/components/tjpgd", {public = true})
    add_files(LUATOS_ROOT.."/components/tjpgd/*.c")
    -- qrcode
    add_includedirs(LUATOS_ROOT.."/components/qrcode", {public = true})
    add_files(LUATOS_ROOT.."/components/qrcode/*.c")
    -- i2c-tools
    add_includedirs(LUATOS_ROOT.."/components/i2c-tools",{public = true})
    add_files(LUATOS_ROOT.."/components/i2c-tools/*.c")
    -- lora
    add_includedirs(LUATOS_ROOT.."/components/lora/sx126x",{public = true})
    add_files(LUATOS_ROOT.."/components/lora/**.c")
    -- lora2
    add_includedirs(LUATOS_ROOT.."/components/lora2/sx126x",{public = true})
    add_files(LUATOS_ROOT.."/components/lora2/**.c")
    -- fonts
    add_includedirs(LUATOS_ROOT.."/components/luatfonts",{public = true})
    add_files(LUATOS_ROOT.."/components/luatfonts/*.c")
    -- wlan
    add_includedirs(LUATOS_ROOT.."/components/wlan",{public = true})
    add_files(LUATOS_ROOT.."/components/wlan/*.c")
    -- network
    add_includedirs(LUATOS_ROOT .. "/components/ethernet/w5500", {public = true})
    add_files(LUATOS_ROOT .. "/components/ethernet/**.c")
	remove_files(LUATOS_ROOT .."/components/ethernet/common/dns_client.c")
    add_files(LUATOS_ROOT .."/components/network/adapter/luat_lib_socket.c")
    -- mqtt
    add_includedirs(LUATOS_ROOT.."/components/network/libemqtt", {public = true})
    add_files(LUATOS_ROOT.."/components/network/libemqtt/*.c")
    -- http
    add_includedirs(LUATOS_ROOT.."/components/network/libhttp", {public = true})
    add_files(LUATOS_ROOT.."/components/network/libhttp/*.c")
    -- http_parser
    add_includedirs(LUATOS_ROOT.."/components/network/http_parser", {public = true})
    add_files(LUATOS_ROOT.."/components/network/http_parser/*.c")
    -- websocket
    add_includedirs(LUATOS_ROOT.."/components/network/websocket", {public = true})
    add_files(LUATOS_ROOT.."/components/network/websocket/*.c")
    -- errdump
    add_includedirs(LUATOS_ROOT.."/components/network/errdump", {public = true})
    add_files(LUATOS_ROOT.."/components/network/errdump/*.c")
    -- -- httpsrv
    -- add_includedirs(LUATOS_ROOT.."/components/network/httpsrv/inc", {public = true})
    -- add_files(LUATOS_ROOT.."/components/network/httpsrv/src/*.c")
    -- iotauth
    add_files(LUATOS_ROOT.."/components/iotauth/luat_lib_iotauth.c")
    -- sntp
    add_includedirs(LUATOS_ROOT.."/components/network/libsntp", {public = true})
    add_files(LUATOS_ROOT.."/components/network/libsntp/*.c")
    -- libftp
    add_includedirs(LUATOS_ROOT.."/components/network/libftp", {public = true})
    add_files(LUATOS_ROOT.."/components/network/libftp/*.c")
    -- sfd
    add_includedirs(LUATOS_ROOT.."/components/sfd", {public = true})
    add_files(LUATOS_ROOT.."/components/sfd/*.c")
    -- fatfs
    add_includedirs(LUATOS_ROOT.."/components/fatfs", {public = true})
    add_files(LUATOS_ROOT.."/components/fatfs/*.c")
    -- iconv
    add_includedirs(LUATOS_ROOT.."/components/iconv", {public = true})
    add_files(LUATOS_ROOT.."/components/iconv/*.c")
    remove_files(LUATOS_ROOT.."/components/iconv/luat_iconv.c")
    -- max30102
    add_includedirs(LUATOS_ROOT.."/components/max30102", {public = true})
    add_files(LUATOS_ROOT.."/components/max30102/*.c")
    -- ymodem
    add_includedirs(LUATOS_ROOT.."/components/ymodem", {public = true})
    add_files(LUATOS_ROOT.."/components/ymodem/*.c")
    -- shell
    add_includedirs(LUATOS_ROOT .. "/components/shell", {public = true})
    add_files(LUATOS_ROOT.."/components/shell/*.c")
    -- cmux
    add_includedirs(LUATOS_ROOT .. "/components/cmux", {public = true})
    add_files(LUATOS_ROOT .. "/components/cmux/*.c")
    -- repl
    add_includedirs(LUATOS_ROOT.."/components/repl", {public = true})
    add_files(LUATOS_ROOT.."/components/repl/*.c")
    -- statem
    add_includedirs(LUATOS_ROOT.."/components/statem", {public = true})
    add_files(LUATOS_ROOT.."/components/statem/*.c")
    -- ercoap
    add_includedirs(LUATOS_ROOT.."/components/network/ercoap/include",{public = true})
    add_files(LUATOS_ROOT.."/components/network/ercoap/src/*.c")
    add_files(LUATOS_ROOT.."/components/network/ercoap/binding/*.c")
    
    --加入代码和头文件
    add_includedirs("./inc",{public = true})
    add_files("./src/*.c",{public = true})

target_end()