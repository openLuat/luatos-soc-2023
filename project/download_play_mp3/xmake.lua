local TARGET_NAME = os.scriptdir():match(".+[/\\]([%w_]+)")
local LIB_DIR = "$(buildir)/".. TARGET_NAME .. "/"
local LIB_NAME = "lib" .. TARGET_NAME .. ".a "
target(TARGET_NAME)
    set_kind("static")
    set_targetdir(LIB_DIR)
    -- set_warnings("error")
    --加入代码和头文件
    add_defines("LUAT_USE_TLS",{public = true})
     add_includedirs(LUATOS_ROOT.."/components/network/libhttp", {public = true})
    add_includedirs(LUATOS_ROOT.."/components/network/http_parser", {public = true})
    add_files(LUATOS_ROOT.."/components/network/libhttp/*.c")
    remove_files(LUATOS_ROOT.."/components/network/libhttp/luat_lib_http.c")
    add_files(LUATOS_ROOT.."/components/network/http_parser/*.c")



    add_includedirs("./include",{public = true})
    add_files("./src/*.c",{public = true})

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

    add_files("$(projectdir)/PLAT/core/lib/libaisound50_16K.a")
target_end()