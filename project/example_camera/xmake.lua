local TARGET_NAME = os.scriptdir():match(".+[/\\]([%w_]+)")
local LIB_DIR = "$(buildir)/".. TARGET_NAME .. "/"

target(TARGET_NAME)
    set_kind("static")
    set_targetdir(LIB_DIR)
    
    --加入代码和头文件
    add_includedirs("./inc",{public = true})
    add_files("./src/*.c",{public = true})
    --路径可以随便写,可以加任意路径的代码,下面代码等效上方代码
    -- add_includedirs("$(projectdir)/project/" .. TARGET_NAME .. "/inc",{public = true})
    -- add_files("$(projectdir)/project/" .. TARGET_NAME .. "/src/*.c",{public = true})
    add_linkdirs("$(projectdir)/lib","$(projectdir)/PLAT/core/lib",{public = true})
	add_linkgroups("image_decoder_0", {whole = true,public = true})
	--lcd
	add_includedirs(LUATOS_ROOT.."/components/lcd", {public = true})
    add_files(LUATOS_ROOT.."/components/lcd/*.c")
    remove_files(LUATOS_ROOT.."/components/lcd/luat_lib_lcd.c")
target_end()