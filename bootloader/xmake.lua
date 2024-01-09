
local SDK_PATH = os.projectdir()
local USER_PROJECT_NAME = USER_PROJECT_NAME
local LIB_PS_PLAT = nil
if has_config("chip_target") and has_config("lspd_mode") then 
    chip_target = get_config("chip_target") 
    LIB_PS_PLAT = "full"
    if get_config("lspd_mode")=="enable" then
        if chip_target == "ec718pv" then
        LIB_FW = "audio"
        LIB_PS_PLAT = "ims"
        else 
        LIB_PS_PLAT = "oc"
        end
    else 
        if chip_target == "ec718p" or chip_target == "ec718e" then
            LIB_PS_PLAT = "full"
        elseif chip_target == "ec718pv" then
            LIB_FW = "audio"
            LIB_PS_PLAT = "ims"
        else
            add_defines("MID_FEATURE_MODE")
            LIB_PS_PLAT = "mid"
        end
    end
    if chip_target == "ec718pv" then
        add_defines("FEATURE_AMR_CP_ENABLE","FEATURE_VEM_CP_ENABLE")
    end
end

add_defines("LTO_FEATURE_MODE",
            "DHCPD_ENABLE_DEFINE=0",
            "FEATURE_FOTA_ENABLE",
            "MIDDLEWARE_FOTA_ENABLE",
            "FEATURE_FOTA_CORE2_ENABLE",
            "USBC_USBMST_MGR_FEATURE_DISABLE=1",
            "USB_DRV_SMALL_IMAGE=1",
            "FEATURE_BOOTLOADER_PROJECT_ENABLE",
            "__BL_MODE__",
            "FEATURE_FOTA_HLS_ENABLE",
            "FEATURE_FOTA_USBURC_ENABLE",
            "DEBUG_LOG_HEADER_FILE=\"debug_log_dummy.h\""
            )
add_cxflags("-flto",
            "-fuse-linker-plugin",
            "-ffat-lto-objects",
            "-Wno-lto-type-mismatch",
            {force=true})

target("driver")
    set_kind("static")
    add_includedirs("$(projectdir)/PLAT/prebuild/PLAT/inc/usb_bl")

    add_includedirs("$(projectdir)/PLAT/middleware/developed/debug/inc",
                    "$(projectdir)/PLAT/project/ec7xx_0h00/ap/apps/bootloader/code/include",
                    "$(projectdir)/PLAT/project/ec7xx_0h00/ap/apps/bootloader/code/include/common",
                    "$(projectdir)/PLAT/project/ec7xx_0h00/ap/apps/bootloader/code/common/secure/hash/inc")
    -- wrapper
    add_includedirs("$(projectdir)/PLAT/middleware/thirdparty/lzma2201/C",
                    "$(projectdir)/PLAT/middleware/thirdparty/lzma2201/C/wrapper")

	add_files(
                "$(projectdir)/PLAT/core/code/boot_code.c",
                "$(projectdir)/PLAT/core/code/fota_code.c",
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
                "$(projectdir)/PLAT/driver/chip/ec7xx/common/gcc/memcpy-armv7m.S")

	remove_files(
                "$(projectdir)/PLAT/project/ec7xx_0h00/ap/apps/bootloader/code/main/system_ec7xx.c",
                "$(projectdir)/PLAT/project/ec7xx_0h00/ap/apps/bootloader/code/common/image/image.c",
                "$(projectdir)/PLAT/project/ec7xx_0h00/ap/apps/bootloader/code/common/secure/ecc/src/*.c"
                )
    set_targetdir("$(buildir)/bootloader_libdriver")
target_end()

target("ap_bootloader.elf")
    set_kind("binary")
    set_targetdir("$(buildir)/ap_bootloader")
    add_deps("driver")

    local chip_target = nil
    if has_config("chip_target") and has_config("lspd_mode") then chip_target = get_config("chip_target") end
    if chip_target then
        add_linkdirs("$(projectdir)/PLAT/prebuild/PLAT/lib/gcc/"..(chip_target=="ec718e"and"ec718p"or chip_target):sub(1,6).."/"..LIB_PS_PLAT)
        add_linkdirs("$(projectdir)/PLAT/libs/"..(chip_target=="ec718e"and"ec718p"or chip_target).."/bootloader")
        if (chip_target=="ec718e"and"ec718p"or chip_target):sub(1,6) == "ec718p" then
            add_linkgroups("ffota", {whole = true})
        end
    end

    add_linkdirs("$(projectdir)/lib/")
    add_linkgroups("driver","startup","core_airm2m","lzma","driver_private_bl","bootloader","usbbl_priv",
                    "osa","middleware_ec","middleware_ec_private","ccio","fota","deltapatch2", {whole = true})

    add_ldflags("-T$(projectdir)/PLAT/core/ld/ec7xx_0h00_flash_bl.ld","-Wl,-Map,$(buildir)/ap_bootloader/ap_bootloader_$(mode).map",{force = true})
    
    local toolchains = nil
    local ld_parameter = nil 
    before_link(function(target)
        toolchains = target:tool("cc"):match('.+\\bin') or target:tool("cc"):match('.+/bin')
        for _, dep in ipairs(target:orderdeps()) do
            local linkdir = dep:targetdir()
            target:add("ldflags","-L./"..linkdir, {force=true})
        end  
        ld_parameter = {"-E","-P"}

        local user_mem_map = {}
        for _, define_flasg in pairs(target:get("defines")) do
            table.insert(ld_parameter,"-D" .. define_flasg)
            if define_flasg == "__USER_MAP_CONF_FILE__=\"mem_map_7xx.h\"" then
                for _, filepath in ipairs(os.files("$(projectdir)/project/"..USER_PROJECT_NAME.."/**/mem_map_7xx.h")) do
                    if path.filename(filepath) == "mem_map_7xx.h" then
                        user_mem_map = {"-I",path.directory(filepath)}
                        break
                    end
                end
            end
        end

        os.execv(toolchains .. "/arm-none-eabi-gcc",table.join(ld_parameter,user_mem_map, {"-I",SDK_PATH .. "/PLAT/device/target/board/ec7xx_0h00/common/pkginc"},{"-I",SDK_PATH .. "/PLAT/device/target/board/ec7xx_0h00/common/inc"},{"-I",SDK_PATH .. "/PLAT/core/ld"},{"-o",SDK_PATH .. "/PLAT/core/ld/ec7xx_0h00_flash_bl.ld","-"}),{stdin = SDK_PATH .. "/PLAT/core/ld/ec7xx_0h00_flash_bl.c"})
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
        io.writefile("$(buildir)/ap_bootloader/ap_bootloader.size", os.iorun(toolchains .. "/arm-none-eabi-objdump -h $(buildir)/ap_bootloader/ap_bootloader.elf"))
        local size_file = io.open("$(buildir)/ap_bootloader/ap_bootloader.size", "a")
        size_file:write(os.iorun(toolchains .. "/arm-none-eabi-size -G $(buildir)/ap_bootloader/ap_bootloader.elf"))
        if (chip_target=="ec718e"and"ec718p"or chip_target):sub(1,6) == "ec718p" then size_file:write(os.iorun(toolchains .. "/arm-none-eabi-size -t -G $(projectdir)/lib/libffota.a")) end
        size_file:write(os.iorun(toolchains .. "/arm-none-eabi-size -t -G $(buildir)/bootloader_libdriver/libdriver.a"))
        for _, filepath in ipairs(os.files("$(projectdir)/PLAT/libs/"..(chip_target=="ec718e"and"ec718p"or chip_target).."/bootloader/*.a")) do
            size_file:write(os.iorun(toolchains .. "/arm-none-eabi-size -t -G " .. filepath))
        end
        size_file:close()
        os.exec((is_plat("windows") and "./PLAT/tools/fcelf.exe " or "./PLAT/tools/fcelf ").."-C -bin ".."$(buildir)/ap_bootloader/ap_bootloader_unZip.bin".. " -cfg ".. "$(projectdir)/PLAT/project/ec7xx_0h00/ap/apps/bootloader/GCC/sectionInfo_"..(chip_target=="ec718e"and"ec718p"or chip_target):sub(1,6)..".json".. " -map ".."$(buildir)/ap_bootloader/ap_bootloader_debug.map".." -out ".."$(buildir)/ap_bootloader/ap_bootloader.bin")
    end)
target_end()






