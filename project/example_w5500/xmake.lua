local TARGET_NAME = os.scriptdir():match(".+[/\\]([%w_]+)")
local LIB_DIR = "$(buildir)/".. TARGET_NAME .. "/"
local LIB_NAME = "lib" .. TARGET_NAME .. ".a "

target(TARGET_NAME)
    set_kind("static")
    set_targetdir(LIB_DIR)
    set_warnings("error")
    
    add_defines("LUAT_USE_DHCP",{public = true})
    add_defines("LUAT_USE_W5500",{public = true})
    add_defines("LUAT_USE_MOBILE",{public = true})
    -- LUAT_USE_W5500

    add_includedirs(LUATOS_ROOT.."/components/ethernet/common", {public = true})
    add_files(LUATOS_ROOT.."/components/ethernet/common/*.c")
    remove_files(LUATOS_ROOT.."/components/ethernet/common/luat_network_adapter.c")
    remove_files(LUATOS_ROOT.."/components/ethernet/common/dns_client.c")

    add_includedirs(LUATOS_ROOT.."/components/ethernet/w5500", {public = true})
    add_files(LUATOS_ROOT.."/components/ethernet/w5500/*.c")
    remove_files(LUATOS_ROOT.."/components/ethernet/w5500/luat_lib_w5500.c")


    

    --加入代码和头文件
    add_includedirs("./inc",{public = true})
    add_files("./src/*.c",{public = true})

    add_linkgroups(TARGET_NAME, {group = true, whole = true})
target_end()