local TARGET_NAME = os.scriptdir():match(".+[/\\]([%w_]+)")
local LIB_DIR = "$(buildir)/".. TARGET_NAME .. "/"
local LIB_NAME = "lib" .. TARGET_NAME .. ".a "
local CHIP_TARGET = CHIP_TARGET
target(TARGET_NAME)
    set_kind("static")
    set_targetdir(LIB_DIR)
    set_warnings("error")

    -- 程序区缩小到1b7000，剩余2ca000 - 1b7000 = 113000
    if CHIP_TARGET == "ec718p" then
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
    --可以继续增加add_includedirs和add_files
    --自动链接
    LIB_USER = LIB_USER .. "$(projectdir)/" .. LIB_DIR .. LIB_NAME .. " "
    --甚至可以加入自己的库
target_end()