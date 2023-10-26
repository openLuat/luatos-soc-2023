local TARGET_NAME = os.scriptdir():match(".+[/\\]([%w_]+)")
local LIB_DIR = "$(buildir)/".. TARGET_NAME .. "/"
local LIB_NAME = "lib" .. TARGET_NAME .. ".a "

target(TARGET_NAME)
    set_kind("static")
    set_targetdir(LIB_DIR)
    set_warnings("error")
    --加入代码和头文件
    add_includedirs("./inc",{public = true})
    add_files("./src/*.c",{public = true})

    --路径可以随便写,可以加任意路径的代码,下面代码等效上方代码
    -- add_includedirs(SDK_TOP .. "/project/" .. TARGET_NAME .. "/inc",{public = true})
    -- add_files(SDK_TOP .. "/project/" .. TARGET_NAME .. "/src/*.c",{public = true})

    add_includedirs(LUATOS_ROOT.."components/fskv", {public = true})
    add_files(LUATOS_ROOT.."components/fskv/*.c")
    remove_files(LUATOS_ROOT.."components/fskv/luat_lib_fskv.c")

    --可以继续增加add_includedirs和add_files
    --自动链接
    LIB_USER = LIB_USER .. SDK_TOP ..  "/".. LIB_DIR .. LIB_NAME .. " "
    --甚至可以加入自己的库
target_end()