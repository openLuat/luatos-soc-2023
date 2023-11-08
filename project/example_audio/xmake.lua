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

	add_linkgroups(TARGET_NAME,"mp3", {group = true, whole = true})
    add_linkdirs("$(projectdir)/PLAT/core/lib")
    add_linkgroups("aisound50_16K", {group = true, whole = true})
    --8K版本用下面的库，注释掉16K的库
    -- add_linkgroups("aisound50_8K", {group = true, whole = true})
    --8K英文版本用下面的库，注释掉16K的库
    -- add_linkgroups("aisound50_8K_eng", {group = true, whole = true})
    --16K英文版本用下面的库，注释掉16K的库
    -- add_linkgroups("aisound50_16K_eng", {group = true, whole = true})

target_end()