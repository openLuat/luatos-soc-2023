local TARGET_NAME = os.scriptdir():match(".+[/\\]([%w_]+)")
local LIB_DIR = "$(buildir)/".. TARGET_NAME .. "/"
local LIB_NAME = "lib" .. TARGET_NAME .. ".a "

--local LUATOS_ROOT = SDK_TOP .. "/../LuatOS/"
--includes(SDK_TOP.."/luatos_lwip_socket")
target(TARGET_NAME)
    set_kind("static")
    set_targetdir(LIB_DIR)
    set_warnings("error")

    -- mbedtls
    add_defines("LUAT_USE_TLS","MBEDTLS_CONFIG_FILE=\"mbedtls_ec7xx_config.h\"",{public = true})

    --加入代码和头文件
    add_includedirs("./inc",{public = true})
    -- add_includedirs(SDK_TOP .. "/interface/private_include", 
    --                 {public = true})
    add_files("./src/*.c",{public = true})
    

    --自动链接
    LIB_USER = LIB_USER .. SDK_TOP .."/".. LIB_DIR .. LIB_NAME .. " "
    --甚至可以加入自己的库
target_end()