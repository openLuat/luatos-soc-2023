local TARGET_NAME = "example_flash"
local LIB_DIR = "$(buildir)/".. TARGET_NAME .. "/"
local LIB_NAME = "lib" .. TARGET_NAME .. ".a "
local CHIP_TARGET = CHIP_TARGET
target(TARGET_NAME)
    set_kind("static")
    set_targetdir(LIB_DIR)
    set_warnings("error")

    -- 程序区缩小到1b7000，剩余2ca000 - 1b7000 = 113000 1100KB空间为用户区
    if CHIP_TARGET == "ec718p" then
       add_defines("AP_FLASH_LOAD_SIZE=0x1b7000",{public = true})
       add_defines("AP_PKGIMG_LIMIT_SIZE=0x1b7000",{public = true})
    end
    --加入代码和头文件
    add_includedirs("./inc",{public = true})
    add_files("./src/*.c",{public = true})

    --add_files("../../thirdparty/fal/src/*.c",{public = true})
    --add_files("../../thirdparty/flashdb/src/*.c",{public = true})

    --可以继续增加add_includedirs和add_files
    --自动链接
    LIB_USER = LIB_USER .. SDK_TOP .. LIB_DIR .. LIB_NAME .. " "
    --甚至可以加入自己的库
target_end()