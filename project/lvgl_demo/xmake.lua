local TARGET_NAME = os.scriptdir():match(".+[/\\]([%w_]+)")
local LIB_DIR = "$(buildir)/".. TARGET_NAME .. "/"
local LIB_NAME = "lib" .. TARGET_NAME .. ".a "

target(TARGET_NAME)
    set_kind("static")
    set_targetdir(LIB_DIR)

    -- set_basename(TARGET_NAME)
    -- 程序区缩小到1b3000，剩余2b3000 - 1b3000 = 100000 1MB空间为用户区
    -- if CHIP_TARGET == "ec718p" then
    --     add_defines("AP_FLASH_LOAD_SIZE=0x1b3000",{public = true})
    --     add_defines("AP_PKGIMG_LIMIT_SIZE=0x1b3000",{public = true})
    -- end

    --加入代码和头文件
    add_includedirs("./inc",{public = true})

    -- 此demo默认添加了benchmark和widgets demo，且在lv_conf.h中已打开对应功能宏
    -- 默认演示benchmark demo
    -- 若需使用其他demo，请将下面一行代码注释，自行添加源文件，并在lv_conf.h内修打开对应功能宏
    add_files("./src/show.c",{public = true})

    -- 将下面注释打开并将上一行代码注释，即可演示widgets demo
    -- add_files("./src/demo_widgets.c",{public = true})

    add_includedirs(LUATOS_ROOT.."/components/u8g2", {public = true})
    add_includedirs(LUATOS_ROOT.."/components/lcd", {public = true})
    add_files(LUATOS_ROOT.."/components/lcd/*.c")
    remove_files(LUATOS_ROOT.."/components/lcd/luat_lib_lcd.c")
	add_includedirs(LUATOS_ROOT.."/components/lvgl8",{public = true})
	add_includedirs(LUATOS_ROOT.."/components/lvgl8/src",{public = true})
	add_includedirs(LUATOS_ROOT.."/components/lvgl8/demos",{public = true})
	add_files(LUATOS_ROOT.."/components/lvgl8/src/**.c",{public = true})
	add_files(LUATOS_ROOT.."/components/lvgl8/demos/benchmark/**.c",{public = true})
	add_files(LUATOS_ROOT.."/components/lvgl8/demos/widgets/**.c",{public = true})
    --可以继续增加add_includedirs和add_files

target_end()