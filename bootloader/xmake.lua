
local SDK_PATH = os.projectdir()
local CHIP_TARGET = CHIP_TARGET

local BL_LIB_BASE = "$(projectdir)/PLAT/libs/"..CHIP_TARGET.."/bootloader/libcore_airm2m.a "
BL_LIB_BASE = BL_LIB_BASE .. "$(projectdir)/PLAT/libs/"..CHIP_TARGET.."/bootloader/libstartup.a "
BL_LIB_BASE = BL_LIB_BASE .. "$(projectdir)/PLAT/libs/"..CHIP_TARGET.."/bootloader/libmiddleware_ec.a "
BL_LIB_BASE = BL_LIB_BASE .. "$(projectdir)/PLAT/libs/"..CHIP_TARGET.."/bootloader/liblzma.a "

if CHIP_TARGET == "ec718p" then
--BL_LIB_BASE = BL_LIB_BASE .. "$(projectdir)/lib/libffota.a "
end

local LIB_PS_PLAT = "full"
if is_lspd then
    LIB_PS_PLAT = "oc"
else 
    if CHIP_TARGET == "ec718p" then
        LIB_PS_PLAT = "full"
    else
        add_defines("MID_FEATURE_MODE")
        LIB_PS_PLAT = "mid"
    end
end

local LIB_PLAT_PRE = "$(projectdir)/PLAT/prebuild/PLAT/lib/gcc/"..CHIP_TARGET.."/"..LIB_PS_PLAT

BL_LIB_BASE = BL_LIB_BASE .. LIB_PLAT_PRE .. "/libosa.a "
BL_LIB_BASE = BL_LIB_BASE .. LIB_PLAT_PRE .. "/libmiddleware_ec_private.a "
BL_LIB_BASE = BL_LIB_BASE .. LIB_PLAT_PRE .. "/libccio.a "
BL_LIB_BASE = BL_LIB_BASE .. LIB_PLAT_PRE .. "/libfota.a "
BL_LIB_BASE = BL_LIB_BASE .. LIB_PLAT_PRE .. "/libdeltapatch2.a "
BL_LIB_BASE = BL_LIB_BASE .. LIB_PLAT_PRE .. "/libdriver_private_bl.a "
BL_LIB_BASE = BL_LIB_BASE .. LIB_PLAT_PRE .. "/libbootloader.a "
BL_LIB_BASE = BL_LIB_BASE .. LIB_PLAT_PRE .. "/libusbbl_priv.a "

target("ap_bootloader.elf")
    set_kind("binary")
    set_targetdir("$(buildir)/ap_bootloader")

    add_defines("LTO_FEATURE_MODE",
                "DHCPD_ENABLE_DEFINE=0",
                "FEATURE_FOTA_ENABLE",
                "MIDDLEWARE_FOTA_ENABLE",
                "FEATURE_FOTA_CORE2_ENABLE",
                "USBC_USBMST_MGR_FEATURE_DISABLE=1",
                "USB_DRV_SMALL_IMAGE=1",
                "FEATURE_BOOTLOADER_PROJECT_ENABLE",
                "__BL_MODE__",
                "DEBUG_LOG_HEADER_FILE=\"debug_log_dummy.h\""
                )
    add_cxflags("-flto",
                "-fuse-linker-plugin",
                "-ffat-lto-objects",
                "-Wno-lto-type-mismatch",
                {force=true})
                
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
                    "$(projectdir)/PLAT/driver/hal/ec7xx/ap/inc/ec718",
                    "$(projectdir)/PLAT/driver/chip/ec7xx/ap/inc",
                    "$(projectdir)/PLAT/driver/chip/ec7xx/ap/inc/ec718",
                    "$(projectdir)/PLAT/driver/chip/ec7xx/ap/inc_cmsis",
                    "$(projectdir)/PLAT/os/freertos/inc",
                    "$(projectdir)/PLAT/os/freertos/CMSIS/common/inc",
                    "$(projectdir)/PLAT/os/freertos/CMSIS/ap/inc",
                    "$(projectdir)/PLAT/os/freertos/portable/mem/tlsf",
                    "$(projectdir)/PLAT/os/freertos/portable/gcc",
                    "$(projectdir)/PLAT/middleware/developed/nvram/ec7xx/inc",
                    "$(projectdir)/PLAT/middleware/developed/debug/inc",
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
                    "$(projectdir)/PLAT/core/driver/include",
                    "$(projectdir)/PLAT/core/common/include",
                    "$(projectdir)/PLAT/core/multimedia/include",
                    "$(projectdir)/PLAT/core/tts/include",
                    "$(projectdir)/PLAT/prebuild/PLAT/inc",
                    "$(projectdir)/PLAT/project/ec7xx_0h00/ap/apps/bootloader/code/include",
                    "$(projectdir)/PLAT/project/ec7xx_0h00/ap/apps/bootloader/code/include/common",
                    "$(projectdir)/PLAT/project/ec7xx_0h00/ap/apps/bootloader/code/common/secure/hash/inc",
                    "$(projectdir)/PLAT/core/include",
                    {public = true})

	add_files(
                "$(projectdir)/PLAT/core/code/boot_code.c",
                "$(projectdir)/PLAT/core/code/fota_code.c",
                -- lzma
                -- "$(projectdir)/PLAT/middleware/thirdparty/lzma2201/C/*.c",
                -- "$(projectdir)/PLAT/middleware/thirdparty/lzma2201/C/wrapper/*.c",
                -- driver
                "$(projectdir)/PLAT/driver/board/ec7xx_0h00/src/plat_config.c",
                "$(projectdir)/PLAT/driver/hal/ec7xx/ap/src/hal_pwrkey.c",
                "$(projectdir)/PLAT/driver/hal/ec7xx/ap/src/hal_misc.c",
                "$(projectdir)/PLAT/driver/hal/common/src/ec_string.c",
                "$(projectdir)/PLAT/driver/chip/ec7xx/ap/src/clock.c",
                "$(projectdir)/PLAT/driver/chip/ec7xx/ap/src/gpio.c",
                "$(projectdir)/PLAT/driver/chip/ec7xx/ap/src/oneWire.c",
                "$(projectdir)/PLAT/driver/chip/ec7xx/ap/src/pad.c",
                "$(projectdir)/PLAT/driver/chip/ec7xx/ap/src/wdt.c",
                "$(projectdir)/PLAT/driver/chip/ec7xx/ap/src/usb_bl/open/*.c",
                "$(projectdir)/PLAT/driver/chip/ec7xx/ap/src/usb_bl/usb_device/*.c",
                "$(projectdir)/PLAT/driver/chip/ec7xx/common/gcc/memcpy-armv7m.S",
            {public = true})

	remove_files(
                "$(projectdir)/PLAT/project/ec7xx_0h00/ap/apps/bootloader/code/main/system_ec7xx.c",
                "$(projectdir)/PLAT/project/ec7xx_0h00/ap/apps/bootloader/code/common/image/image.c",
                "$(projectdir)/PLAT/project/ec7xx_0h00/ap/apps/bootloader/code/common/secure/ecc/src/*.c"
                )

    add_linkdirs("$(projectdir)/PLAT/prebuild/PLAT/lib/gcc/"..CHIP_TARGET.."/"..LIB_PS_PLAT)
    add_linkdirs("$(projectdir)/PLAT/libs/"..CHIP_TARGET.."/bootloader")

    -- add_linkgroups("startup","core_airm2m","middleware_ec","lzma", {whole = true,group = true})
    -- add_linkgroups("osa","middleware_ec_private","ccio","fota","deltapatch2","driver_private_bl","bootloader","usbbl_priv", {whole = true,group = true})

    add_ldflags("-T$(projectdir)/PLAT/core/ld/ec7xx_0h00_flash_bl.ld","-Wl,-Map,$(buildir)/ap_bootloader/ap_bootloader_$(mode).map",{force = true})
    add_ldflags("-Wl,--whole-archive -Wl,--start-group " .. BL_LIB_BASE .. " -Wl,--end-group -Wl,--no-whole-archive ", {force=true})
    local toolchains = nil
    local ld_parameter = nil 
    before_link(function(target)
        toolchains = target:toolchains()[1]:bindir()
        for _, dep in ipairs(target:orderdeps()) do
            local linkdir = dep:targetdir()
            target:add("ldflags","-L./"..linkdir, {force=true})
        end  
        ld_parameter = {"-E","-P"}
        for _, define_flasg in pairs(target:get("defines")) do
            table.insert(ld_parameter,"-D" .. define_flasg)
        end
        os.execv(toolchains .. "/arm-none-eabi-gcc",table.join(ld_parameter, {"-I",SDK_PATH .. "/PLAT/device/target/board/ec7xx_0h00/common/pkginc"},{"-I",SDK_PATH .. "/PLAT/device/target/board/ec7xx_0h00/common/inc"},{"-I",SDK_PATH .. "/PLAT/core/ld"},{"-o",SDK_PATH .. "/PLAT/core/ld/ec7xx_0h00_flash_bl.ld","-"}),{stdin = SDK_PATH .. "/PLAT/core/ld/ec7xx_0h00_flash_bl.c"})
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
        os.execv(toolchains .. "/arm-none-eabi-gcc",table.join(mem_parameter, {"-o",SDK_PATH .. "/build/ap_bootloader/mem_map.txt","-"}),{stdin = SDK_PATH .. "/PLAT/device/target/board/ec7xx_0h00/common/inc/mem_map.h"})
        os.exec(toolchains .. "/arm-none-eabi-objcopy -O binary $(buildir)/ap_bootloader/ap_bootloader.elf $(buildir)/ap_bootloader/ap_bootloader.bin")
        os.iorun(toolchains .. "/arm-none-eabi-size $(buildir)/ap_bootloader/ap_bootloader.elf")
        os.cp("$(buildir)/ap_bootloader/ap_bootloader.bin", "$(buildir)/ap_bootloader/ap_bootloader_unZip.bin")
        os.exec((is_plat("windows") and "./PLAT/tools/fcelf.exe " or "./PLAT/tools/fcelf ").."-C -bin ".."$(buildir)/ap_bootloader/ap_bootloader_unZip.bin".. " -cfg ".. "$(projectdir)/PLAT/project/ec7xx_0h00/ap/apps/bootloader/GCC/sectionInfo_"..CHIP_TARGET..".json".. " -map ".."$(buildir)/ap_bootloader/ap_bootloader_debug.map".." -out ".."$(buildir)/ap_bootloader/ap_bootloader.bin")
    end)
target_end()






