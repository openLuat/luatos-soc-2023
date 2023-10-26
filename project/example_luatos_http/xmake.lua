local TARGET_NAME = os.scriptdir():match(".+[/\\]([%w_]+)")
local LIB_DIR = "$(buildir)/".. TARGET_NAME .. "/"
local LIB_NAME = "lib" .. TARGET_NAME .. ".a "

target(TARGET_NAME)
    set_kind("static")
    set_targetdir(LIB_DIR)
    set_warnings("error")

    -- mbedtls
    add_defines("LUAT_USE_TLS",{public = true})

    --加入代码和头文件
    add_includedirs("./inc",{public = true})
    add_files("./src/*.c",{public = true})

    add_includedirs(LUATOS_ROOT.."components/network/libhttp", {public = true})
    add_includedirs(LUATOS_ROOT.."components/network/http_parser", {public = true})
    add_files(LUATOS_ROOT.."components/network/libhttp/*.c")
    remove_files(LUATOS_ROOT.."components/network/libhttp/luat_lib_http.c")
    add_files(LUATOS_ROOT.."components/network/http_parser/*.c")
    

    --自动链接
    LIB_USER = LIB_USER .. SDK_TOP .."/".. LIB_DIR .. LIB_NAME .. " "
    --甚至可以加入自己的库
target_end()