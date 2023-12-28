local TARGET_NAME = os.scriptdir():match(".+[/\\]([%w_]+)")
local LIB_DIR = "$(buildir)/".. TARGET_NAME .. "/"
local LIB_NAME = "lib" .. TARGET_NAME .. ".a "

target(TARGET_NAME)
    set_kind("static")
    set_targetdir(LIB_DIR)
    set_warnings("error")
    --加入代码和头文件
    add_includedirs("./include",{public = true})
    add_files("./src/*.c",{public = true})

    --加入amr编解码库
    add_includedirs(LUATOS_ROOT .. "/components/multimedia/amr_decode/amr_common/dec/include",{public = true})
    add_includedirs(LUATOS_ROOT .. "/components/multimedia/amr_decode/amr_nb/common/include",{public = true})
    add_includedirs(LUATOS_ROOT .. "/components/multimedia/amr_decode/amr_nb/dec/include",{public = true})
    add_includedirs(LUATOS_ROOT .. "/components/multimedia/amr_decode/amr_wb/dec/include",{public = true})
    add_includedirs(LUATOS_ROOT .. "/components/multimedia/amr_decode/opencore-amrnb",{public = true})
    add_includedirs(LUATOS_ROOT .. "/components/multimedia/amr_decode/opencore-amrwb",{public = true})
    add_includedirs(LUATOS_ROOT .. "/components/multimedia/amr_decode/oscl",{public = true})
    add_includedirs(LUATOS_ROOT .. "/components/multimedia/amr_decode/amr_nb/enc/src",{public = true})
    -- **.c会递归所有子文件夹下的文件
    add_files(LUATOS_ROOT .. "/components/multimedia/amr_decode/**.c",{public = true})

    add_files("$(projectdir)/lib/libmp3.a")
	add_files("$(projectdir)/PLAT/core/lib/libtts_res.a")
    add_files("$(projectdir)/PLAT/core/lib/libaisound50_16K.a")
    --8K版本用下面的库，注释掉16K的库
    -- add_files("$(projectdir)/PLAT/core/lib/libaisound50_8K.a")
    --8K英文版本用下面的库，注释掉16K的库
    -- add_files("$(projectdir)/PLAT/core/lib/libaisound50_8K_eng.a")
    --16K英文版本用下面的库，注释掉16K的库
    -- add_files("$(projectdir)/PLAT/core/lib/libaisound50_16K_eng.a")

    -- add_linkdirs("$(projectdir)/lib","$(projectdir)/PLAT/core/lib",{public = true})
    -- -- 此处使用 libaisound50_16K.a, 还可选择 8K版本:libaisound50_8K.a,8K英文版本:libaisound50_8K_eng.a,16K英文版本:libaisound50_16K_eng.a
    -- add_linkgroups("mp3","aisound50_16K", {whole = true,public = true})

target_end()