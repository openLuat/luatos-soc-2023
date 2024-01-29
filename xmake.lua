set_project("EC7XX")
set_xmakever("2.8.6")
set_version("0.0.2", {build = "%Y%m%d%H%M"})
add_rules("mode.debug", "mode.release")
set_defaultmode("debug")

LUATOS_ROOT = "$(projectdir)/../LuatOS"
LUAT_BSP_VERSION = "V0002"
USER_PROJECT_NAME = "example"
chip_target = nil
USER_PROJECT_DIR = nil

package("gnu_rm")
	set_kind("toolchain")
	set_homepage("https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm")
	set_description("GNU Arm Embedded Toolchain")

	if is_host("windows") then
		set_urls("http://cdndownload.openluat.com/xmake/toolchains/gcc-arm/gcc-arm-none-eabi-10-2020-q4-major-win32.zip")
		add_versions("ec7xx", "90057b8737b888c53ca5aee332f1f73c401d6d3873124d2c2906df4347ebef9e")
	elseif is_host("linux") then
		set_urls("http://cdndownload.openluat.com/xmake/toolchains/gcc-arm/gcc-arm-none-eabi-10-2020-q4-major-x86_64-linux.tar.bz2")
		add_versions("ec7xx", "21134caa478bbf5352e239fbc6e2da3038f8d2207e089efc96c3b55f1edcd618")
	elseif is_host("macosx") then
		set_urls("http://cdndownload.openluat.com/xmake/toolchains/gcc-arm/gcc-arm-none-eabi-10-2020-q4-major-mac.tar.bz2")
		add_versions("ec7xx", "bed12de3565d4eb02e7b58be945376eaca79a8ae3ebb785ec7344e7e2db0bdc0")
	end
	on_install("@windows", "@linux", "@macosx", function (package)
		os.vcp("*", package:installdir())
	end)
package_end()

if os.getenv("GCC_PATH") then
	toolchain("arm_toolchain")
	    set_kind("standalone")
	    set_sdkdir(os.getenv("GCC_PATH"))
	toolchain_end()
	set_toolchains("arm_toolchain")
else
	add_requires("gnu_rm ec7xx")
	set_toolchains("gnu-rm@gnu_rm")
end

set_plat("cross")
set_arch("arm")
set_languages("gnu11", "cxx11")
set_warnings("all")
set_optimize("smallest")

if os.getenv("PROJECT_DIR") then
    USER_PROJECT_DIR = os.getenv("PROJECT_DIR")
    USER_PROJECT_NAME = USER_PROJECT_DIR:match(".+[/\\]([%w_]+)")
end

option("chip_target")
    set_default("ec718p")
    set_showmenu(true)
    set_values("ec716s","ec718s","ec718e","ec718p","ec718pv")
    set_description("chip target")
option_end()
add_options("chip_target")

if has_config("chip_target") then chip_target = get_config("chip_target") end

-- 获取项目名称
if os.getenv("PROJECT_NAME") then
	USER_PROJECT_NAME = os.getenv("PROJECT_NAME")
end

-- option("project_name")
--     set_default(true)
--     set_showmenu(true)
--     set_description("project name")
-- option_end()
-- add_options("project_name")

-- if has_config("project_name") then project_name = get_config("project_name") end

option("lspd_mode")
    set_default("enable")
    set_showmenu(true)
    set_description("Enable or disable low speed mode , more memory")
option_end()
add_options("lspd_mode")

option("denoise_force")
    set_default("disable")
    set_showmenu(true)
    set_description("Enable or disable denoise_force, force to use audio module to support noise reduction ,disable is auto detect ")
option_end()
add_options("denoise_force")

if chip_target then
    CHIP = "ec718"
    if chip_target == "ec718p" or chip_target == "ec718pv" or chip_target == "ec718e" then
        add_defines("CHIP_EC718","TYPE_EC718P")
    elseif chip_target == "ec718s" then
        add_defines("CHIP_EC718","TYPE_EC718S")
    elseif chip_target == "ec716s" then
        CHIP = "ec716"
        add_defines("CHIP_EC716","TYPE_EC716S")
    end

    -- 若启用is_lspd, 加上额外的宏
    if (chip_target == "ec718pv" or chip_target == "ec718p" or chip_target == "ec718e") and get_config("lspd_mode")  or chip_target == "ec716s" or chip_target == "ec718s" then
        add_defines("OPEN_CPU_MODE")
    end
    add_includedirs("$(projectdir)/PLAT/driver/hal/ec7xx/ap/inc/"..CHIP,
                "$(projectdir)/PLAT/driver/chip/ec7xx/ap/inc/"..CHIP)
end

add_defines("__USER_CODE__",
            "CORE_IS_AP",
            "SDK_REL_BUILD",
            "RAMCODE_COMPRESS_EN",
            "REL_COMPRESS_EN",
            "ARM_MATH_CM3",
            "FEATURE_LZMA_ENABLE",
            "WDT_FEATURE_ENABLE=1",
            "TRACE_LEVEL=5",
            "SOFTPACK_VERSION=\"\"",
            "HAVE_STRUCT_TIMESPEC",
            "FEATURE_FOTAPAR_ENABLE",
            "__ASSEMBLY__",
            "__CURRENT_FILE_NAME__=__FILE__"
            )

add_cxflags("-g3",
            "-mcpu=cortex-m3",
            "-mthumb",
            "-nostartfiles",
            "-mapcs-frame",
            "-ffunction-sections",
            "-fdata-sections",
            "-fno-isolate-erroneous-paths-dereference",
            "-freorder-blocks-algorithm=stc",
            "-Wno-format",
            {force=true})

add_cxflags("-Werror=maybe-uninitialized", {force=true})

add_asflags("-mcpu=cortex-m3 -mthumb",{force = true})

add_ldflags("-mcpu=cortex-m3",
            "-mthumb",
            "--specs=nano.specs",
            "-lm",
            "-Wl,--cref",
            "-Wl,--check-sections",
            "-Wl,--gc-sections",
            "-Wl,--no-undefined",
            "-Wl,--no-print-map-discarded",
            "-Wl,--print-memory-usage",
            {force = true})
-- SDK通用头文件引用
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
                -- "$(projectdir)/PLAT/driver/board/ec7xx_0h00/inc/audio/codec/es8388",
                "$(projectdir)/PLAT/driver/board/ec7xx_0h00/inc/audio/codec/es8311",
                "$(projectdir)/PLAT/driver/board/ec7xx_0h00/inc/lcd/lcdDev",
                "$(projectdir)/PLAT/driver/board/ec7xx_0h00/inc/lcd",
                "$(projectdir)/PLAT/driver/board/ec7xx_0h00/inc/ntc",
                "$(projectdir)/PLAT/driver/board/ec7xx_0h00/inc/exstorage",
                "$(projectdir)/PLAT/driver/hal/common/inc",
                "$(projectdir)/PLAT/driver/hal/ec7xx/ap/inc",
                "$(projectdir)/PLAT/driver/chip/ec7xx/ap/inc",
                "$(projectdir)/PLAT/driver/chip/ec7xx/ap/inc_cmsis",
                "$(projectdir)/PLAT/os/freertos/inc",
                "$(projectdir)/PLAT/os/freertos/CMSIS/common/inc",
                "$(projectdir)/PLAT/os/freertos/CMSIS/ap/inc",
                "$(projectdir)/PLAT/os/freertos/portable/mem/tlsf",
                "$(projectdir)/PLAT/os/freertos/portable/gcc",
                "$(projectdir)/PLAT/middleware/developed/nvram/inc",
                "$(projectdir)/PLAT/middleware/developed/nvram/ec7xx/inc",
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
                "$(projectdir)/PLAT/middleware/developed/tcpipmgr/app/inc",
                "$(projectdir)/PLAT/middleware/developed/tcpipmgr/common/inc",
                "$(projectdir)/PLAT/middleware/developed/yrcompress",
                "$(projectdir)/PLAT/middleware/thirdparty/lwip/src/include",
                "$(projectdir)/PLAT/middleware/thirdparty/lwip/src/include/lwip",
                "$(projectdir)/PLAT/middleware/thirdparty/lwip/src/include/posix",
                "$(projectdir)/PLAT/middleware/developed/ccio/pub",
                "$(projectdir)/PLAT/middleware/developed/ccio/device/inc",
                "$(projectdir)/PLAT/middleware/developed/ccio/service/inc",
                "$(projectdir)/PLAT/middleware/developed/ccio/custom/inc",
                "$(projectdir)/PLAT/middleware/developed/fota/pub",
                "$(projectdir)/PLAT/middleware/developed/fota/custom/inc",
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
                "$(projectdir)/PLAT/prebuild/PS/inc",
                "$(projectdir)/PLAT/prebuild/PLAT/inc")

local USER_PROJECT_NAME = USER_PROJECT_NAME
on_load(function (target)
    local chip_target = get_config("chip_target")
    assert (chip_target == "ec718e" or chip_target == "ec718p" or chip_target == "ec718pv" or chip_target == "ec718s" or chip_target == "ec716s" ,"target only support ec718e/ec718p/ec718pv/ec718s/ec716s")
    for _, filepath in ipairs(os.files("$(projectdir)/project/"..USER_PROJECT_NAME.."/**/mem_map_7xx.h")) do
        if path.filename(filepath) == "mem_map_7xx.h" then
            target:add("defines", "__USER_MAP_CONF_FILE__=\"mem_map_7xx.h\"")
            target:add("includedirs", path.directory(filepath))
            break
        end
    end
end)

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

includes("bootloader")
includes("project")

