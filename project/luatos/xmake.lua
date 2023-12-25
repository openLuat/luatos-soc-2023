local TARGET_NAME = os.scriptdir():match(".+[/\\]([%w_]+)")
local LIB_DIR = "$(buildir)/".. TARGET_NAME .. "/"
local CHIP_TARGET = CHIP_TARGET

target(TARGET_NAME)
    set_kind("static")
    set_targetdir(LIB_DIR)
    -- set_warnings("error")
    add_linkdirs("$(projectdir)/lib","$(projectdir)/PLAT/core/lib",{public = true})
    add_linkgroups("mp3", {whole = true,public = true})
    
    on_load(function(target)
        assert (CHIP_TARGET == "ec718p" or CHIP_TARGET == "ec718pv" or CHIP_TARGET == "ec718e" ,"luatos only support ec718p/ec718pv/ec718e")
        local conf_data = io.readfile("$(projectdir)/project/luatos/inc/luat_conf_bsp.h")
        local FLASH_FOTA_REGION_START = 0x340000 -- ec718p FLASH_FOTA_REGION_START
        -- print("FLASH_FOTA_REGION_START",FLASH_FOTA_REGION_START)
        local LUAT_SCRIPT_SIZE = tonumber(conf_data:match("\r#define LUAT_SCRIPT_SIZE (%d+)") or conf_data:match("\n#define LUAT_SCRIPT_SIZE (%d+)"))
        local LUAT_SCRIPT_OTA_SIZE = tonumber(conf_data:match("\r#define LUAT_SCRIPT_OTA_SIZE (%d+)") or conf_data:match("\n#define LUAT_SCRIPT_OTA_SIZE (%d+)"))
        -- print(string.format("script zone %d ota %d", LUAT_SCRIPT_SIZE, LUAT_SCRIPT_OTA_SIZE))
        local LUA_SCRIPT_ADDR = FLASH_FOTA_REGION_START - (LUAT_SCRIPT_SIZE + LUAT_SCRIPT_OTA_SIZE) * 1024
        local LUA_SCRIPT_OTA_ADDR = FLASH_FOTA_REGION_START - LUAT_SCRIPT_OTA_SIZE * 1024
        local script_addr = string.format("%X", LUA_SCRIPT_ADDR)
        local full_addr = string.format("%X", LUA_SCRIPT_OTA_ADDR)
        -- print("LUA_SCRIPT_ADDR",LUA_SCRIPT_ADDR)
        -- print("LUA_SCRIPT_OTA_ADDR",LUA_SCRIPT_OTA_ADDR)
        -- print("script_addr",script_addr)
        target:add("defines","AP_FLASH_LOAD_SIZE=0x"..script_addr.."-0x0007e000",{public = true})
        target:add("defines","AP_PKGIMG_LIMIT_SIZE=0x"..script_addr.."-0x0007e000",{public = true})

        local LUAT_USE_TTS_8K = conf_data:find("\r#define LUAT_USE_TTS_8K") or conf_data:find("\n#define LUAT_USE_TTS_8K")
        if LUAT_USE_TTS_8K then
            target:add("files","$(projectdir)/PLAT/core/lib/libaisound50_8K.a")
            -- target:add("linkgroups","aisound50_8K", {whole = true,public = true})
        else 
            target:add("files","$(projectdir)/PLAT/core/lib/libaisound50_16K.a")
            -- target:add("linkgroups","aisound50_16K", {whole = true,public = true})
        end

        local LUAT_USE_TLS_DISABLE = conf_data:find("\r#define LUAT_USE_TLS_DISABLE") or conf_data:find("\n#define LUAT_USE_TLS_DISABLE")
        if not LUAT_USE_TLS_DISABLE then
            -- mbedtls
            target:add("defines", "LUAT_USE_TLS",{public = true})
        end
    end)

    add_defines("__LUATOS__",{public = true})

    add_includedirs(LUATOS_ROOT.."/lua/include",{public = true})
    add_files(LUATOS_ROOT .. "/lua/src/*.c")
    add_files(LUATOS_ROOT .. "/luat/modules/*.c")

    add_files(LUATOS_ROOT.."/luat/freertos/luat_timer_freertos.c")
    add_files(LUATOS_ROOT.."/luat/freertos/luat_msgbus_freertos.c")
    -- weak
    add_files(LUATOS_ROOT.."/luat/weak/luat_rtos_lua.c")
    -- common
    add_includedirs(LUATOS_ROOT.."/components/common",{public = true})
    -- add_files(LUATOS_ROOT.."/components/common/*.c")
    -- mobile
    add_files(LUATOS_ROOT.."/components/mobile/luat_lib_mobile.c")
    -- sms
    add_files(LUATOS_ROOT.."/components/sms/*.c")
    -- hmeta
    add_includedirs(LUATOS_ROOT.."/components/hmeta",{public = true})
    add_files(LUATOS_ROOT.."/components/hmeta/*.c")
    -- profiler
    add_includedirs(LUATOS_ROOT.."/components/mempool/profiler/include",{public = true})
    add_files(LUATOS_ROOT.."/components/mempool/profiler/**.c")
    -- rsa
    add_files(LUATOS_ROOT.."/components/rsa/**.c")
    -- lua-cjson
    add_includedirs(LUATOS_ROOT .. "/components/lua-cjson",{public = true})
    add_files(LUATOS_ROOT .. "/components/lua-cjson/*.c")
    -- fastlz
    add_includedirs(LUATOS_ROOT .. "/components/fastlz",{public = true})
    add_files(LUATOS_ROOT .. "/components/fastlz/*.c")
    -- miniz
    add_includedirs(LUATOS_ROOT .. "/components/miniz",{public = true})
    add_files(LUATOS_ROOT .. "/components/miniz/*.c")
    -- coremark
    add_includedirs(LUATOS_ROOT .. "/components/coremark",{public = true})
    add_files(LUATOS_ROOT .. "/components/coremark/*.c")
    -- gmssl
    add_includedirs(LUATOS_ROOT .. "/components/gmssl/include",{public = true})
    add_files(LUATOS_ROOT .. "/components/gmssl/**.c")
    -- protobuf
    add_includedirs(LUATOS_ROOT.."/components/serialization/protobuf",{public = true})
    add_files(LUATOS_ROOT.."/components/serialization/protobuf/*.c")
    -- vfs
    add_files(LUATOS_ROOT.."/luat/vfs/*.c")
    remove_files(LUATOS_ROOT.."/luat/vfs/luat_fs_lfs2.c",
            LUATOS_ROOT.."/luat/vfs/luat_vfs.c")
    -- fatfs
    add_includedirs(LUATOS_ROOT.."/components/fatfs",{public = true})
    add_files(LUATOS_ROOT.."/components/fatfs/*.c")
    -- sfud
    add_defines("LUAT_USE_SFUD",{public = true})
    add_includedirs(LUATOS_ROOT.."/components/sfud",{public = true})
    add_files(LUATOS_ROOT.."/components/sfud/*.c")
    -- fskv
    add_includedirs(LUATOS_ROOT.."/components/fskv",{public = true})
    add_files(LUATOS_ROOT.."/components/fskv/*.c")
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
    -- lvgl
    add_includedirs(LUATOS_ROOT.."/components/lvgl", {public = true})
    add_includedirs(LUATOS_ROOT.."/components/lvgl/binding", {public = true})
    add_includedirs(LUATOS_ROOT.."/components/lvgl/gen", {public = true})
    add_includedirs(LUATOS_ROOT.."/components/lvgl/src", {public = true})
    add_includedirs(LUATOS_ROOT.."/components/lvgl/font", {public = true})
    add_includedirs(LUATOS_ROOT.."/components/lvgl/src/lv_font", {public = true})
    add_files(LUATOS_ROOT.."/components/lvgl/**.c")
    -- 默认不编译lv的demos, 节省大量的编译时间
    remove_files(LUATOS_ROOT.."/components/lvgl/lv_demos/**.c")
    -- i2c-tools
    add_includedirs(LUATOS_ROOT.."/components/i2c-tools",{public = true})
    add_files(LUATOS_ROOT.."/components/i2c-tools/*.c")
    -- lora
    add_includedirs(LUATOS_ROOT.."/components/lora/sx126x",{public = true})
    add_files(LUATOS_ROOT.."/components/lora/**.c")
    -- lora2
    add_includedirs(LUATOS_ROOT.."/components/lora2/sx126x",{public = true})
    add_files(LUATOS_ROOT.."/components/lora2/**.c")
    -- libgnss
    add_files(LUATOS_ROOT.."/components/minmea/*.c|minmea.c")
    -- mlx90640
    add_includedirs(LUATOS_ROOT.."/components/mlx90640-library",{public = true})
    add_files(LUATOS_ROOT.."/components/mlx90640-library/*.c")
    -- wlan
    add_files(LUATOS_ROOT.."/components/wlan/*.c")
    -- audio
    add_includedirs(LUATOS_ROOT.."/components/multimedia/",{public = true})
    add_includedirs(LUATOS_ROOT.."/components/multimedia/mp3_decode",{public = true})
    add_includedirs(LUATOS_ROOT.."/components/multimedia/amr_decode/amr_common/dec/include",{public = true})
    add_includedirs(LUATOS_ROOT.."/components/multimedia/amr_decode/amr_nb/common/include",{public = true})
    add_includedirs(LUATOS_ROOT.."/components/multimedia/amr_decode/amr_nb/dec/include",{public = true})
    add_includedirs(LUATOS_ROOT.."/components/multimedia/amr_decode/amr_wb/dec/include",{public = true})
    add_includedirs(LUATOS_ROOT.."/components/multimedia/amr_decode/opencore-amrnb",{public = true})
    add_includedirs(LUATOS_ROOT.."/components/multimedia/amr_decode/opencore-amrwb",{public = true})
    add_includedirs(LUATOS_ROOT.."/components/multimedia/amr_decode/oscl",{public = true})
    add_includedirs(LUATOS_ROOT.."/components/multimedia/amr_decode/amr_nb/enc/src",{public = true})
    add_files(LUATOS_ROOT.."/components/multimedia/**.c")
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
    add_includedirs(LUATOS_ROOT.."/components/network/httpsrv/inc", {public = true})
    add_files(LUATOS_ROOT.."/components/network/httpsrv/src/*.c")
    -- iotauth
    add_includedirs(LUATOS_ROOT.."/components/iotauth", {public = true})
    add_files(LUATOS_ROOT.."/components/iotauth/*.c")
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
    -- sqlite3
    add_includedirs(LUATOS_ROOT.."/components/sqlite3/include",{public = true})
    add_files(LUATOS_ROOT.."/components/sqlite3/src/*.c")
    add_files(LUATOS_ROOT.."/components/sqlite3/binding/*.c")
    -- ws2812 单独的库
    add_includedirs(LUATOS_ROOT.."/components/ws2812/include",{public = true})
    add_files(LUATOS_ROOT.."/components/ws2812/src/*.c")
    add_files(LUATOS_ROOT.."/components/ws2812/binding/*.c")
    -- xxtea
    add_includedirs(LUATOS_ROOT.."/components/xxtea/include",{public = true})
    add_files(LUATOS_ROOT.."/components/xxtea/src/*.c")
    add_files(LUATOS_ROOT.."/components/xxtea/binding/*.c")
    --加入代码和头文件
    add_includedirs("./inc",{public = true})
    add_files("./src/*.c",{public = true})

target_end()