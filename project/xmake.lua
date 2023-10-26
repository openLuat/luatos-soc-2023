local SDK_PATH = os.projectdir()
local CHIP_TARGET = CHIP_TARGET
local USER_PROJECT_NAME = USER_PROJECT_NAME
local CHIP = CHIP
-- 若启用is_lspd, 加上额外的宏
if is_lspd == true then
    add_defines("OPEN_CPU_MODE")
end

add_defines(
            "EC_ASSERT_FLAG",
            "PM_FEATURE_ENABLE",
            "UINILOG_FEATURE_ENABLE",
            "PSRAM_FEATURE_ENABLE",
            "FEATURE_OS_ENABLE",
            "FEATURE_FREERTOS_ENABLE",
            "configUSE_NEWLIB_REENTRANT=1",
            "FEATURE_YRCOMPRESS_ENABLE",
            "FEATURE_CCIO_ENABLE",
            "LWIP_CONFIG_FILE=\"lwip_config_cat.h\"",
            --"LWIP_CONFIG_FILE=\"lwip_config_ec7xx0h00.h\"",
            -- "FEATURE_MBEDTLS_ENABLE",-------------
            "LFS_NAME_MAX=63",
            "LFS_DEBUG_TRACE",
            "FEATURE_UART_HELP_DUMP_ENABLE",
            "HTTPS_WITH_CA",
            "FEATURE_HTTPC_ENABLE",
            -- "LITE_FEATURE_MODE",
            -- "RTE_RNDIS_EN=0", "RTE_ETHER_EN=0",
            "RTE_USB_EN=1",
            "RTE_PPP_EN=0",
            "RTE_ETHER_EN=0",
            "RTE_RNDIS_EN=0",
            "RTE_OPAQ_EN=0",-----
            "RTE_ONE_UART_AT=0",
            "RTE_TWO_UART_AT=0",
            "__USER_CODE__",
            "LUAT_USE_NETWORK",
            "LUAT_USE_LWIP",
            "__USE_SDK_LWIP__",
            "LUAT_USE_DNS",
            "__PRINT_ALIGNED_32BIT__",
            "_REENT_SMALL",
            "_REENT_GLOBAL_ATEXIT",
            "LWIP_INCLUDED_POLARSSL_MD5=1",
            "LUAT_EC7XX_CSDK",
            "LUAT_USE_STD_STRING",
            "LUAT_LOG_NO_NEWLINE",
            "DHCPD_ENABLE_DEFINE=1",
            "DEBUG_LOG_HEADER_FILE=\"debug_log_ap.h\"",
            {public = true})
add_defines("sprintf=sprintf_",
            "snprintf=snprintf_",
            "vsnprintf=vsnprintf_",
            {public = true})
add_cxflags("-fno-inline",
            "-mslow-flash-data",
            "-fstack-usage",
            -- "-Wstack-usage=4096",
            {force=true})

if CHIP_TARGET == "ec716s" or CHIP_TARGET == "ec718s" then
    -- 开启 lto
    add_defines("LTO_FEATURE_MODE")
    add_cxflags("-flto",
                "-fuse-linker-plugin",
                "-ffat-lto-objects",
                "-Wno-lto-type-mismatch",
                {force=true})
end 

add_ldflags("-lm",
                "-Wl,--wrap=_malloc_r",
                "-Wl,--wrap=_free_r",
                "-Wl,--wrap=_realloc_r",
                "-Wl,--wrap=clock",
                "-Wl,--wrap=localtime",
                "-Wl,--wrap=gmtime",
                "-Wl,--wrap=time",
                {force = true})

-- ==============================
-- === includes =====
-- SDK相关头文件引用
add_includedirs("$(projectdir)/PLAT/device/target/board/common/ARMCM3/inc",
                "$(projectdir)/PLAT/device/target/board/ec7xx_0h00/common/inc",
                "$(projectdir)/PLAT/device/target/board/ec7xx_0h00/common/pkginc",
                "$(projectdir)/PLAT/device/target/board/ec7xx_0h00/ap/gcc",
                "$(projectdir)/PLAT/device/target/board/ec7xx_0h00/ap/inc",
                "$(projectdir)/PLAT/driver/board/ec7xx_0h00/inc",
                "$(projectdir)/PLAT/driver/board/ec7xx_0h00/inc/eeprom",
                "$(projectdir)/PLAT/driver/board/ec7xx_0h00/inc/camera",
                "$(projectdir)/PLAT/driver/board/ec7xx_0h00/inc/camera/sp0A39",
                "$(projectdir)/PLAT/driver/board/ec7xx_0h00/inc/camera/sp0821",
                "$(projectdir)/PLAT/driver/board/ec7xx_0h00/inc/camera/gc6123",
                "$(projectdir)/PLAT/driver/board/ec7xx_0h00/inc/camera/gc6153",
                "$(projectdir)/PLAT/driver/board/ec7xx_0h00/inc/camera/gc032A",
                "$(projectdir)/PLAT/driver/board/ec7xx_0h00/inc/camera/bf30a2",
                "$(projectdir)/PLAT/driver/board/ec7xx_0h00/inc/audio",
                "$(projectdir)/PLAT/driver/board/ec7xx_0h00/inc/audio/codec",
                "$(projectdir)/PLAT/driver/board/ec7xx_0h00/inc/audio/codec/es8388",
                "$(projectdir)/PLAT/driver/board/ec7xx_0h00/inc/audio/codec/es8311",
                "$(projectdir)/PLAT/driver/board/ec7xx_0h00/inc/lcd/lcdDev",
                "$(projectdir)/PLAT/driver/board/ec7xx_0h00/inc/lcd",
                "$(projectdir)/PLAT/driver/board/ec7xx_0h00/inc/ntc",
                "$(projectdir)/PLAT/driver/board/ec7xx_0h00/inc/exstorage",
                "$(projectdir)/PLAT/driver/hal/common/inc",
                "$(projectdir)/PLAT/driver/hal/ec7xx/ap/inc",
                "$(projectdir)/PLAT/driver/hal/ec7xx/ap/inc/"..CHIP,
                "$(projectdir)/PLAT/driver/chip/ec7xx/ap/inc",
                "$(projectdir)/PLAT/driver/chip/ec7xx/ap/inc/"..CHIP,
                "$(projectdir)/PLAT/driver/chip/ec7xx/ap/inc_cmsis",
                "$(projectdir)/PLAT/os/freertos/inc",
                "$(projectdir)/PLAT/os/freertos/CMSIS/common/inc",
                "$(projectdir)/PLAT/os/freertos/CMSIS/ap/inc",
                "$(projectdir)/PLAT/os/freertos/portable/mem/tlsf",
                "$(projectdir)/PLAT/os/freertos/portable/gcc",
                "$(projectdir)/PLAT/middleware/developed/nvram/ec7xx/inc",
                "$(projectdir)/PLAT/middleware/developed/nvram/inc",
                "$(projectdir)/PLAT/middleware/developed/cms/psdial/inc",
                "$(projectdir)/PLAT/middleware/developed/cms/cms/inc",
                "$(projectdir)/PLAT/middleware/developed/cms/psil/inc",
                "$(projectdir)/PLAT/middleware/developed/cms/psstk/inc",
                "$(projectdir)/PLAT/middleware/developed/cms/sockmgr/inc",
                "$(projectdir)/PLAT/middleware/developed/cms/cmsnetlight/inc",
                "$(projectdir)/PLAT/middleware/developed/ecapi/aal/inc",
                "$(projectdir)/PLAT/middleware/developed/ecapi/appmwapi/inc",
                "$(projectdir)/PLAT/middleware/developed/ecapi/psapi/inc",
                "$(projectdir)/PLAT/middleware/developed/common/inc",
                "$(projectdir)/PLAT/middleware/developed/psnv/inc",
                "$(projectdir)/PLAT/os/freertos/portable/gcc",
                "$(projectdir)/PLAT/middleware/developed/tcpipmgr/app/inc",
                "$(projectdir)/PLAT/middleware/developed/tcpipmgr/common/inc",
                "$(projectdir)/PLAT/os/freertos/inc",
                "$(projectdir)/PLAT/middleware/developed/yrcompress",
                "$(projectdir)/PLAT/prebuild/PS/inc",
                "$(projectdir)/PLAT/middleware/thirdparty/lwip/src/include",
                "$(projectdir)/PLAT/middleware/thirdparty/lwip/src/include/lwip",
                "$(projectdir)/PLAT/middleware/developed/ccio/pub",
                "$(projectdir)/PLAT/middleware/developed/ccio/device/inc",
                "$(projectdir)/PLAT/middleware/developed/ccio/service/inc",
                "$(projectdir)/PLAT/middleware/developed/ccio/custom/inc",
                "$(projectdir)/PLAT/middleware/developed/fota/pub",
                "$(projectdir)/PLAT/middleware/developed/fota/custom/inc",
                "$(projectdir)/PLAT/middleware/thirdparty/lwip/src/include",
                "$(projectdir)/PLAT/middleware/thirdparty/lwip/src/include/posix",
                "$(projectdir)/PLAT/middleware/developed/at/atdecoder/inc",
                "$(projectdir)/PLAT/middleware/developed/at/atps/inc",
                "$(projectdir)/PLAT/middleware/developed/at/atps/inc/cnfind",
                "$(projectdir)/PLAT/middleware/developed/at/atcust/inc",
                "$(projectdir)/PLAT/middleware/developed/at/atcust/inc/cnfind",
                "$(projectdir)/PLAT/middleware/developed/at/atentity/inc",
                "$(projectdir)/PLAT/middleware/developed/at/atreply/inc",
                "$(projectdir)/PLAT/middleware/developed/at/atref/inc",
                "$(projectdir)/PLAT/middleware/developed/at/atref/inc/cnfind",
                "$(projectdir)/PLAT/tools/"..CHIP_TARGET,
                "$(projectdir)/PLAT/core/driver/include",
                "$(projectdir)/PLAT/core/common/include",
                "$(projectdir)/PLAT/core/multimedia/include",
                "$(projectdir)/PLAT/core/tts/include",
				"$(projectdir)/PLAT/core/include",
                "$(projectdir)/PLAT/prebuild/PS/inc",
                "$(projectdir)/PLAT/prebuild/PLAT/inc",
                "$(projectdir)/thirdparty/littlefs",
                "$(projectdir)/thirdparty/littlefs/port",
                {public = true})
            
-- CSDK 宏定义
add_defines("LUAT_USE_FS_VFS","MBEDTLS_CONFIG_FILE=\"mbedtls_ec7xx_config.h\"")
-- CSDK相关头文件引用
add_includedirs(LUATOS_ROOT .. "/luat/include",
                LUATOS_ROOT .. "/components/mobile",
                LUATOS_ROOT .. "components/printf",
                LUATOS_ROOT .. "components/ethernet/common",
                LUATOS_ROOT .. "components/mbedtls",
                LUATOS_ROOT .. "components/mbedtls/include",
                LUATOS_ROOT .. "components/mbedtls/include/mbedtls",
                LUATOS_ROOT .. "components/mbedtls/include/psa",
                LUATOS_ROOT .. "components/network/adapter",
				LUATOS_ROOT .. "/components/camera",
				"$(projectdir)/interface/include",
                {public = true})

local LIB_BASE = "$(projectdir)/PLAT/libs/"..CHIP_TARGET.."/libstartup.a "
LIB_BASE = LIB_BASE .. "$(projectdir)/PLAT/libs/"..CHIP_TARGET.."/libcore_airm2m.a "
LIB_BASE = LIB_BASE .. "$(projectdir)/PLAT/libs/"..CHIP_TARGET.."/libfreertos.a "
LIB_BASE = LIB_BASE .. "$(projectdir)/PLAT/libs/"..CHIP_TARGET.."/libpsnv.a "
LIB_BASE = LIB_BASE .. "$(projectdir)/PLAT/libs/"..CHIP_TARGET.."/libtcpipmgr.a "
LIB_BASE = LIB_BASE .. "$(projectdir)/PLAT/libs/"..CHIP_TARGET.."/libyrcompress.a "
LIB_BASE = LIB_BASE .. "$(projectdir)/PLAT/libs/"..CHIP_TARGET.."/libmiddleware_ec.a "
LIB_BASE = LIB_BASE .. "$(projectdir)/PLAT/libs/"..CHIP_TARGET.."/liblwip.a "

LIB_BASE = LIB_BASE .. "$(projectdir)/PLAT/libs/"..CHIP_TARGET.."/liblzma.a "

local LIB_PS_PLAT = "full"
local LIB_FW = "oc"
if is_lspd then
    LIB_PS_PLAT = "oc"
    LIB_FW = "wifi"
else 
    if CHIP_TARGET == "ec718p" then
        LIB_PS_PLAT = "full"
    else
        LIB_PS_PLAT = "mid"
    end
end

LIB_PS_PRE = "$(projectdir)/PLAT/prebuild/PS/lib/gcc/"..CHIP_TARGET.."/"..LIB_PS_PLAT
LIB_PLAT_PRE = "$(projectdir)/PLAT/prebuild/PLAT/lib/gcc/"..CHIP_TARGET.."/"..LIB_PS_PLAT

LIB_BASE = LIB_BASE .. LIB_PS_PRE .. "/libps.a "
LIB_BASE = LIB_BASE .. LIB_PS_PRE .. "/libpsl1.a "
LIB_BASE = LIB_BASE .. LIB_PS_PRE .. "/libpsif.a "

LIB_BASE = LIB_BASE .. LIB_PLAT_PRE .. "/libosa.a "
LIB_BASE = LIB_BASE .. LIB_PLAT_PRE .. "/libmiddleware_ec_private.a "
LIB_BASE = LIB_BASE .. LIB_PLAT_PRE .. "/libccio.a "
LIB_BASE = LIB_BASE .. LIB_PLAT_PRE .. "/libdeltapatch.a "
LIB_BASE = LIB_BASE .. LIB_PLAT_PRE .. "/libfota.a "
LIB_BASE = LIB_BASE .. LIB_PLAT_PRE .. "/libdriver_private.a "
LIB_BASE = LIB_BASE .. LIB_PLAT_PRE .. "/libusb_private.a "
LIB_USER = ""

after_load(function (target)
    for _, sourcebatch in pairs(target:sourcebatches()) do
        if sourcebatch.sourcekind == "as" then -- only asm files
            for idx, objectfile in ipairs(sourcebatch.objectfiles) do
                sourcebatch.objectfiles[idx] = objectfile:gsub("%.S%.o", ".o")
            end
        end
        if sourcebatch.sourcekind == "cc" then -- only c files
            for idx, objectfile in ipairs(sourcebatch.objectfiles) do
                sourcebatch.objectfiles[idx] = objectfile:gsub("%.c%.o", ".o")
            end
        end
    end
end)

includes(USER_PROJECT_NAME)

target("driver")
    set_kind("static")
    add_deps(USER_PROJECT_NAME)
    --driver
	add_files("$(projectdir)/PLAT/core/code/*.c",
            "$(projectdir)/PLAT/driver/board/ec7xx_0h00/src/*c",
            "$(projectdir)/PLAT/driver/hal/**.c",
            "$(projectdir)/PLAT/driver/chip/ec7xx/ap/src/"..CHIP.."/adc.c",
            "$(projectdir)/PLAT/driver/chip/ec7xx/ap/src/*.c",
            "$(projectdir)/PLAT/driver/chip/ec7xx/ap/src/usb/open/*.c",
            "$(projectdir)/PLAT/driver/chip/ec7xx/common/gcc/memcpy-armv7m.S",
            {public = true})

	remove_files(
                "$(projectdir)/PLAT/driver/chip/ec7xx/ap/src/cspi.c",
                "$(projectdir)/PLAT/driver/chip/ec7xx/ap/src/swdio.c"
                )

    set_targetdir("$(buildir)/libdriver_" .. USER_PROJECT_NAME)
    -- add_linkgroups("driver", {group = true})
target_end()

target(USER_PROJECT_NAME..".elf")
	set_kind("binary")
    set_targetdir("$(buildir)/"..USER_PROJECT_NAME)
    add_deps(USER_PROJECT_NAME)
    add_deps("driver")

    add_linkdirs("$(projectdir)/PLAT/device/target/board/ec7xx_0h00/ap/gcc/")
    add_linkdirs("$(projectdir)/PLAT/prebuild/PS/lib/gcc/"..CHIP_TARGET.."/"..LIB_PS_PLAT)
    add_linkdirs("$(projectdir)/PLAT/prebuild/PLAT/lib/gcc/"..CHIP_TARGET.."/"..LIB_PS_PLAT)
    add_linkdirs("$(projectdir)/PLAT/libs/"..CHIP_TARGET)

    -- add_linkgroups("startup","core_airm2m","freertos","psnv","tcpipmgr","yrcompress","middleware_ec","lwip","lzma", {whole = true,group = true})
    -- add_linkgroups("ps","psl1","psif", {whole = true,group = true})
    -- add_linkgroups("osa","middleware_ec_private","ccio","deltapatch","fota","driver_private","usb_private", {whole = true,group = true})

    -- interface
    add_files("$(projectdir)/interface/src/*.c")
    -- network
    add_files(LUATOS_ROOT .."/components/network/adapter/luat_network_adapter.c",
            LUATOS_ROOT .."/components/ethernet/common/dns_client.c"
            )

    -- mbedtls
    add_files(LUATOS_ROOT .."/components/mbedtls/library/*.c",{public = true})
    -- printf
    add_files(LUATOS_ROOT.."/components/printf/*.c")
    -- weak
    -- add_files(LUATOS_ROOT.."luat/weak/luat_log_weak.c")
    -- littlefs
    add_files("$(projectdir)/thirdparty/littlefs/**.c")
    -- vfs
    add_files(LUATOS_ROOT.."luat/vfs/luat_fs_lfs2.c",
            LUATOS_ROOT.."luat/vfs/luat_vfs.c")

    local toolchains = nil
    local out_path = nil
    local ld_parameter = nil 
    
    add_ldflags("-T$(projectdir)/PLAT/core/ld/ec7xx_0h00_flash.ld","-Wl,-Map,$(buildir)/"..USER_PROJECT_NAME.."/"..USER_PROJECT_NAME.."_$(mode).map",{force = true})
    --linkflags
    add_ldflags("-Wl,--whole-archive -Wl,--start-group " .. LIB_BASE .. LIB_USER .. " -Wl,--end-group -Wl,--no-whole-archive -ldriver ", {force=true})
	
    before_link(function(target)
        out_path = SDK_PATH .. "/out/" ..USER_PROJECT_NAME
		if not os.exists(out_path) then
			os.mkdir(SDK_PATH .. "/out/")
			os.mkdir(out_path)
		end
        toolchains = target:toolchains()[1]:bindir()
        for _, dep in ipairs(target:orderdeps()) do
            local linkdir = dep:targetdir()
            target:add("ldflags","-L./"..linkdir, {force=true})
        end  
        ld_parameter = {"-E","-P"}

        for _, dep in pairs(target:orderdeps()) do
            if dep:name() ~= "driver" then
                for _, dep_define_flasg in pairs(dep:get("defines")) do
                    if dep_define_flasg:startswith("AP_FLASH_LOAD_SIZE=") or dep_define_flasg:startswith("AP_PKGIMG_LIMIT_SIZE=") then
                        table.insert(ld_parameter,"-D" .. dep_define_flasg)
                    end
                end
            end
        end

        for _, define_flasg in pairs(target:get("defines")) do
            table.insert(ld_parameter,"-D" .. define_flasg)
        end
        os.execv(toolchains .. "/arm-none-eabi-gcc",table.join(ld_parameter, {"-I",SDK_PATH .. "/PLAT/device/target/board/ec7xx_0h00/common/pkginc"},{"-I",SDK_PATH .. "/PLAT/device/target/board/ec7xx_0h00/common/inc"}, {"-o",SDK_PATH .. "/PLAT/core/ld/ec7xx_0h00_flash.ld","-"}),{stdin = SDK_PATH .. "/PLAT/core/ld/ec7xx_0h00_flash.c"})
    end)

	after_build(function(target)
        local mem_parameter = {}
        for _, cx_flasg in pairs(target:get("cxflags")) do
            table.insert(mem_parameter,cx_flasg)
        end
        table.join2(mem_parameter,ld_parameter)
        for _, includedirs_flasg in pairs(target:get("includedirs")) do
            table.insert(mem_parameter,"-I" .. includedirs_flasg)
        end
        os.execv(toolchains .. "/arm-none-eabi-gcc",table.join(mem_parameter, {"-o",out_path .. "/mem_map.txt","-"}),{stdin = SDK_PATH .. "/PLAT/device/target/board/ec7xx_0h00/common/inc/mem_map.h"})
        os.cp(out_path .. "/mem_map.txt", "$(buildir)/"..USER_PROJECT_NAME.."/mem_map.txt")
		-- io.writefile("$(buildir)/"..USER_PROJECT_NAME.."/"..USER_PROJECT_NAME..".list", os.iorun(toolchains .. "/arm-none-eabi-objdump -h -S $(buildir)/"..USER_PROJECT_NAME.."/"..USER_PROJECT_NAME..".elf"))
        -- io.writefile("$(buildir)/"..USER_PROJECT_NAME.."/"..USER_PROJECT_NAME..".size", os.iorun(toolchains .. "/arm-none-eabi-size $(buildir)/"..USER_PROJECT_NAME.."/"..USER_PROJECT_NAME..".elf"))
		os.exec(toolchains .. "/arm-none-eabi-objcopy -O binary $(buildir)/"..USER_PROJECT_NAME.."/"..USER_PROJECT_NAME..".elf $(buildir)/"..USER_PROJECT_NAME.."/"..USER_PROJECT_NAME..".bin")
		os.exec(toolchains .."/arm-none-eabi-size $(buildir)/"..USER_PROJECT_NAME.."/"..USER_PROJECT_NAME..".elf")
        os.cp("$(buildir)/"..USER_PROJECT_NAME.."/"..USER_PROJECT_NAME..".bin", "$(buildir)/"..USER_PROJECT_NAME.."/"..USER_PROJECT_NAME.."_unZip.bin")
        os.exec((is_plat("windows") and "./PLAT/tools/fcelf.exe " or "./PLAT/tools/fcelf ").."-C -bin ".."$(buildir)/"..USER_PROJECT_NAME.."/"..USER_PROJECT_NAME.."_unZip.bin".. " -cfg ".. SDK_PATH .. "/PLAT/device/target/board/ec7xx_0h00/ap/gcc/sectionInfo_"..CHIP_TARGET..".json".. " -map ".."$(buildir)/"..USER_PROJECT_NAME.."/"..USER_PROJECT_NAME.. "_debug.map".." -out ".."$(buildir)/"..USER_PROJECT_NAME.."/" .. USER_PROJECT_NAME .. ".bin")

        os.cp("$(buildir)/"..USER_PROJECT_NAME.."/*.bin", out_path)
		os.cp("$(buildir)/"..USER_PROJECT_NAME.."/*.map", out_path)
		os.cp("$(buildir)/"..USER_PROJECT_NAME.."/*.elf", out_path)
		os.cp("./PLAT/tools/"..CHIP_TARGET.."/comdb.txt", out_path)
        
        ---------------------------------------------------------
        -------------- 这部分尚不能跨平台 -------------------------
        local binpkg = (is_plat("windows") and "./PLAT/tools/fcelf.exe " or "./PLAT/tools/fcelf ").."-M -input ./build/ap_bootloader/ap_bootloader.bin -addrname BL_PKGIMG_LNA -flashsize BOOTLOADER_PKGIMG_LIMIT_SIZE \
                        -input $(buildir)/"..USER_PROJECT_NAME.."/"..USER_PROJECT_NAME ..".bin -addrname AP_PKGIMG_LNA -flashsize AP_PKGIMG_LIMIT_SIZE \
                        -input ./PLAT/prebuild/FW/lib/gcc/"..CHIP_TARGET.."/"..LIB_FW.."/cp-demo-flash.bin -addrname CP_PKGIMG_LNA -flashsize CP_PKGIMG_LIMIT_SIZE \
                        -pkgmode 1 \
                        -banoldtool 1 \
                        -productname "..CHIP_TARGET:upper().."_PRD \
                        -def "..out_path .. "/mem_map.txt \
                        -outfile " .. out_path.."/"..USER_PROJECT_NAME..".binpkg"

        -- 如果所在平台没有fcelf, 可注释掉下面的行, 没有binpkg生成. 
        -- 仍可使用其他工具继续刷机
        -- print("fcelf CMD --> ", binpkg)
        os.exec(binpkg)
        ---------------------------------------------------------
        
        import("core.base.json")
        import("utils.archive")
        local options = {}
        options.compress = "best"

        if os.exists(out_path.."/pack") then
            os.rmdir(out_path.."/pack")
            os.mkdir(out_path.."/pack")
        end
        os.cp("tools/pack/", out_path)
        local info_table = json.loadfile(out_path.."/pack/info.json")
        info_table["rom"]["file"] = USER_PROJECT_NAME..".binpkg"

        if USER_PROJECT_NAME == 'luatos' then
            -- os.cp("$(projectdir)/project/luatos/pack", out_path)
            -- import("core.base.json")
            -- local info_table = json.loadfile(out_path.."/pack/info.json")
            -- if VM_64BIT then
            --     info_table["script"]["bitw"] = 64
            -- end
            -- if script_addr then
            --     info_table["download"]["script_addr"] = script_addr
            --     info_table["rom"]["fs"]["script"]["size"] = LUAT_SCRIPT_SIZE
            --     io.gsub(out_path.."/pack/config_ec7xx_usb.ini", "filepath = .\\script.bin\nburnaddr = 0x(%g+)", "filepath = .\\script.bin\nburnaddr = 0x"..script_addr)
            -- end
            -- if full_addr then
            --     info_table["fota"]["full_addr"] = full_addr
            -- end
            -- json.savefile(out_path.."/pack/info.json", info_table)
            -- os.cp(out_path.."/luatos.binpkg", out_path.."/pack")
            -- os.cp(out_path.."/luatos.elf", out_path.."/pack")
            -- os.cp("./PLAT/comdb.txt", out_path.."/pack")
            -- os.cp("./PLAT/device/target/board/ec7xx_0h00/common/inc/mem_map.h", out_path .. "/pack")
            -- os.cp("$(projectdir)/project/luatos/inc/luat_conf_bsp.h", out_path.."/pack")
            -- os.exec(path7z.." a -mx9 LuatOS-SoC_"..USER_PROJECT_NAME_VERSION.."_EC7XX.7z "..out_path.."/pack/* -r")
            -- local ver = "_FULL"
            -- if os.getenv("LUAT_EC7XX_LITE_MODE") == "1" then
            --     ver = ""
            -- end
            -- if os.getenv("LUAT_USE_TTS") == "1" then
            --     ver = "_TTS"
            --     if os.getenv("LUAT_USE_TTS_ONCHIP") == "1" then
            --         ver = "_TTS_ONCHIP"
            --     end
            -- end
            -- os.mv("LuatOS-SoC_"..USER_PROJECT_NAME_VERSION.."_EC7XX.7z", out_path.."/LuatOS-SoC_"..USER_PROJECT_NAME_VERSION.."_EC7XX"..ver..".soc")
            -- os.rm(out_path.."/pack")
        else 
            json.savefile(out_path.."/pack/info.json", info_table)
            os.cp(out_path.."/"..USER_PROJECT_NAME..".binpkg", out_path.."/pack")
            os.cp(out_path.."/"..USER_PROJECT_NAME..".elf", out_path.."/pack")
            os.cp("./PLAT/tools/"..CHIP_TARGET.."/comdb.txt", out_path.."/pack")
            os.cp(out_path.."/".."mem_map.txt", out_path.."/pack")
            local ret = archive.archive(out_path.."/"..USER_PROJECT_NAME..".7z", out_path.."/pack/*",options)
            if not ret then
                print("pls install p7zip-full in linux/mac , or 7zip in windows.")
            end
            os.mv(out_path.."/"..USER_PROJECT_NAME..".7z", out_path.."/"..USER_PROJECT_NAME..".soc")
            os.rm(out_path.."/pack")
        end

        -- 计算差分包大小, 需要把老的binpkg放在根目录,且命名为old.binpkg
        if os.exists("old.binpkg") then
            os.cp("./PLAT/tools/fcelf.exe", "tools/dtools/dep/fcelf.exe")
            os.cp(out_path.."/"..USER_PROJECT_NAME..".binpkg", "tools/dtools/new.binpkg")
            os.cp("old.binpkg", "tools/dtools/old.binpkg")
            os.exec("tools\\dtools\\run.bat BINPKG delta.par old.binpkg new.binpkg")
        end
	end)
target_end()
