local TARGET_NAME = os.scriptdir():match(".+[/\\]([%w_]+)")
local LIB_DIR = "$(buildir)/".. TARGET_NAME .. "/"


target(TARGET_NAME)
    set_kind("static")
    set_targetdir(LIB_DIR)
    set_warnings("error")
    --加入代码和头文件
    add_includedirs("./inc",{public = true})
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
    -- common
    add_includedirs(LUATOS_ROOT.."/components/common",{public = true})
    -- audio
    add_includedirs(LUATOS_ROOT.."/components/multimedia/",{public = true})
    add_files(LUATOS_ROOT.."/components/multimedia/luat_audio_es8311.c")
target_end()