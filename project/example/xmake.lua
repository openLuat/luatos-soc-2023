local TARGET_NAME = os.scriptdir():match(".+[/\\]([%w_]+)")
local LIB_DIR = "$(buildir)/".. TARGET_NAME .. "/"
local LIB_NAME = "lib" .. TARGET_NAME .. ".a "
local CHIP_TARGET = CHIP_TARGET

target(TARGET_NAME)
    set_kind("static")
    set_targetdir(LIB_DIR)
    set_warnings("error")

    -- 程序区缩小到1b3000，剩余2b3000 - 1b3000 = 100000 1MB空间为用户区
    -- if CHIP_TARGET == "ec718p" then
    --     add_defines("AP_FLASH_LOAD_SIZE=0x1b3000",{public = true})
    --     add_defines("AP_PKGIMG_LIMIT_SIZE=0x1b3000",{public = true})
    -- end

    --加入代码和头文件
    add_includedirs("./inc",{public = true})
    add_files("./src/*.c",{public = true})

    --路径可以随便写,可以加任意路径的代码,下面代码等效上方代码
    -- add_includedirs("$(projectdir)/project/" .. TARGET_NAME .. "/inc",{public = true})
    -- add_files("$(projectdir)/project/" .. TARGET_NAME .. "/src/*.c",{public = true})

target_end()