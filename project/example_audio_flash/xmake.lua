local TARGET_NAME = os.scriptdir():match(".+[/\\]([%w_]+)")
local LIB_DIR = "$(buildir)/".. TARGET_NAME .. "/"

target(TARGET_NAME)
    set_kind("static")
    set_targetdir(LIB_DIR)
    set_warnings("error")

    --加入代码和头文件
    add_includedirs("./inc",{public = true})
    add_files("./src/*.c",{public = true})
    add_defines("LUAT_USE_SFUD",{public = true})

    --sfud
    add_includedirs(LUATOS_ROOT.."/components/sfud",{public = true})
    add_files(LUATOS_ROOT.."/components/sfud/*.c")
    remove_files(LUATOS_ROOT.."/components/sfud/luat_lib_sfud.c")

    add_linkdirs("$(projectdir)/lib","$(projectdir)/PLAT/core/lib",{public = true})
    -- 此处使用 libaisound50_16K.a, 还可选择 8K版本:libaisound50_8K.a,8K英文版本:libaisound50_8K_eng.a,16K英文版本:libaisound50_16K_eng.a
    add_linkgroups("tts_res", "aisound50_16K", {whole = true,public = true})
-- 
target_end()