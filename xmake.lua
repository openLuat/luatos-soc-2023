set_project("EC7XX")
set_xmakever("2.8.2")
set_version("0.0.2", {build = "%Y%m%d%H%M"})
add_rules("mode.debug", "mode.release")
set_defaultmode("debug")

SDK_TOP = "$(projectdir)"
LUATOS_ROOT = SDK_TOP .. "/../LuatOS/"
local SDK_PATH
local USER_PROJECT_NAME = "example"
USER_PROJECT_DIR  = ""
local CHIP_TARGET = "ec718p"

-- local USER_PROJECT_NAME_VERSION
-- local VM_64BIT = nil
-- local LUAT_SCRIPT_SIZE
-- local LUAT_SCRIPT_OTA_SIZE

USER_AP_SIZE = nil
local script_addr = nil
local full_addr = nil

package("gnu_rm")
	set_kind("toolchain")
	set_homepage("https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm")
	set_description("GNU Arm Embedded Toolchain")

	if is_host("windows") then
		set_urls("http://cdndownload.openluat.com/xmake/toolchains/gcc-arm/gcc-arm-none-eabi-10-2020-q4-major-win32.zip")
		add_versions("ec7xx", "90057b8737b888c53ca5aee332f1f73c401d6d3873124d2c2906df4347ebef9e")
	end
	on_install("@windows", "@linux", function (package)
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

-- 获取构建目标
if os.getenv("CHIP_TARGET") then
	CHIP_TARGET = os.getenv("CHIP_TARGET")
end

-- 获取项目名称
if os.getenv("PROJECT_NAME") then
	USER_PROJECT_NAME = os.getenv("PROJECT_NAME")
end

-- 是否启用低速模式, 内存更大, 但与rndis不兼容
if os.getenv("LSPD_MODE") == "enable" then
    is_lspd = true
else
    is_lspd = false
end

if os.getenv("ROOT_PATH") then
	SDK_TOP = os.getenv("ROOT_PATH")
else
	SDK_TOP = os.curdir()
end
SDK_TOP = SDK_TOP .. "/"
SDK_PATH = SDK_TOP

if os.getenv("PROJECT_DIR") then
    USER_PROJECT_DIR = os.getenv("PROJECT_DIR")
else
    USER_PROJECT_DIR = SDK_TOP .. "/project/" .. USER_PROJECT_NAME
end

set_plat("cross")
set_arch("arm")
set_languages("gnu99", "cxx11")
set_warnings("everything")

-- ==============================
-- === defines =====
local CHIP = "ec718"
if CHIP_TARGET == "ec718p" then
    add_defines("CHIP_EC718","TYPE_EC718P")
	if USER_AP_SIZE then
		add_defines("AP_FLASH_LOAD_SIZE="..USER_AP_SIZE)
		add_defines("AP_PKGIMG_LIMIT_SIZE="..USER_AP_SIZE)
	end
elseif CHIP_TARGET == "ec718s" then
    is_lspd = true
    add_defines("CHIP_EC718","TYPE_EC718S")
elseif CHIP_TARGET == "ec716s" then
    is_lspd = true
    CHIP = "ec716"
    add_defines("CHIP_EC716","TYPE_EC716S")
end

-- 若启用is_lspd, 加上额外的宏
if is_lspd == true then
    add_defines("OPEN_CPU_MODE")
end

add_defines("__USER_CODE__",
            "LTO_FEATURE_MODE",
            "CORE_IS_AP",
            "SDK_REL_BUILD",
            -- "__CURRENT_FILE_NAME__=system_ec7xx",
            "EC_ASSERT_FLAG",
            "DHCPD_ENABLE_DEFINE=1",
            "PM_FEATURE_ENABLE",
            "UINILOG_FEATURE_ENABLE",
            "PSRAM_FEATURE_ENABLE",
            "FEATURE_OS_ENABLE",
            "FEATURE_FREERTOS_ENABLE",
            "FEATURE_FOTAPAR_ENABLE",
            "configUSE_NEWLIB_REENTRANT=1",
            "ARM_MATH_CM3",
            "FEATURE_YRCOMPRESS_ENABLE",
            "FEATURE_CCIO_ENABLE",
            "DHCPD_ENABLE_DEFINE=1",
            "LWIP_CONFIG_FILE=\"lwip_config_cat.h\"",
            --"LWIP_CONFIG_FILE=\"lwip_config_ec7xx0h00.h\"",
            -- "FEATURE_MBEDTLS_ENABLE",-------------
            "LFS_NAME_MAX=63",
            "LFS_DEBUG_TRACE",
            "FEATURE_LZMA_ENABLE",
            "WDT_FEATURE_ENABLE=1",
            "FEATURE_UART_HELP_DUMP_ENABLE",
            "DEBUG_LOG_HEADER_FILE=\"debug_log_ap.h\"",
            "TRACE_LEVEL=5",
            "SOFTPACK_VERSION=\"\"",
            "HAVE_STRUCT_TIMESPEC",

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
            "LUAT_USE_TLS",
            "__USE_SDK_LWIP__",
            "LUAT_USE_DNS",
            "__PRINT_ALIGNED_32BIT__",
            "_REENT_SMALL",
            "_REENT_GLOBAL_ATEXIT",

            "LWIP_INCLUDED_POLARSSL_MD5=1",
            "RAMCODE_COMPRESS_EN",
            "REL_COMPRESS_EN",
            "__ASSEMBLY__",
            "LUAT_EC7XX_CSDK",
            "LUAT_USE_STD_STRING",
			"LUAT_LOG_NO_NEWLINE"
)

set_optimize("smallest")
add_cxflags("-g3",
            "-mcpu=cortex-m3",
            "-mthumb",
            "-std=gnu99",
            "-nostartfiles",
            "-mapcs-frame",

            "-ffunction-sections",
            "-fdata-sections",
            "-fno-isolate-erroneous-paths-dereference",
            "-freorder-blocks-algorithm=stc",
            "-Wall",
            "-Wno-format",
            "-gdwarf-2",
            "-fno-inline",
            "-mslow-flash-data",
            "-fstack-usage",
            "-Wstack-usage=4096",

            "-flto",
            "-fuse-linker-plugin",
            "-ffat-lto-objects",
            "-Wno-lto-type-mismatch",
{force=true})

add_ldflags(" -Wl,--wrap=clock ",{force = true})
add_ldflags(" -Wl,--wrap=localtime ",{force = true})
add_ldflags(" -Wl,--wrap=gmtime ",{force = true})
add_ldflags(" -Wl,--wrap=time ",{force = true})

add_ldflags("--specs=nano.specs", {force=true})
add_asflags("-Wl,--cref -Wl,--check-sections -Wl,--gc-sections -lm -Wl,--print-memory-usage -Wl,--wrap=_malloc_r -Wl,--wrap=_free_r -Wl,--wrap=_realloc_r  -mcpu=cortex-m3 -mthumb -DTRACE_LEVEL=5 -DSOFTPACK_VERSION=\"\" -DHAVE_STRUCT_TIMESPEC")

add_defines("sprintf=sprintf_")
add_defines("snprintf=snprintf_")
add_defines("vsnprintf=vsnprintf_")

-- ==============================
-- === includes =====
-- SDK相关头文件引用
add_includedirs(
                SDK_TOP .. "/PLAT/device/target/board/common/ARMCM3/inc",
                SDK_TOP .. "/PLAT/device/target/board/ec7xx_0h00/common/inc",
                SDK_TOP .. "/PLAT/device/target/board/ec7xx_0h00/common/pkginc",
                SDK_TOP .. "/PLAT/device/target/board/ec7xx_0h00/ap/gcc",
                SDK_TOP .. "/PLAT/device/target/board/ec7xx_0h00/ap/inc",
                SDK_TOP .. "/PLAT/driver/board/ec7xx_0h00/inc",
                SDK_TOP .. "/PLAT/driver/board/ec7xx_0h00/inc/eeprom",
                SDK_TOP .. "/PLAT/driver/board/ec7xx_0h00/inc/camera",
                SDK_TOP .. "/PLAT/driver/board/ec7xx_0h00/inc/camera/sp0A39",
                SDK_TOP .. "/PLAT/driver/board/ec7xx_0h00/inc/camera/sp0821",
                SDK_TOP .. "/PLAT/driver/board/ec7xx_0h00/inc/camera/gc6123",
                SDK_TOP .. "/PLAT/driver/board/ec7xx_0h00/inc/camera/gc6153",
                SDK_TOP .. "/PLAT/driver/board/ec7xx_0h00/inc/camera/gc032A",
                SDK_TOP .. "/PLAT/driver/board/ec7xx_0h00/inc/camera/bf30a2",
                SDK_TOP .. "/PLAT/driver/board/ec7xx_0h00/inc/audio",
                SDK_TOP .. "/PLAT/driver/board/ec7xx_0h00/inc/audio/codec",
                SDK_TOP .. "/PLAT/driver/board/ec7xx_0h00/inc/audio/codec/es8388",
                SDK_TOP .. "/PLAT/driver/board/ec7xx_0h00/inc/audio/codec/es8311",
                SDK_TOP .. "/PLAT/driver/board/ec7xx_0h00/inc/lcd/lcdDev",
                SDK_TOP .. "/PLAT/driver/board/ec7xx_0h00/inc/lcd",
                SDK_TOP .. "/PLAT/driver/board/ec7xx_0h00/inc/ntc",
                SDK_TOP .. "/PLAT/driver/board/ec7xx_0h00/inc/exstorage",
                SDK_TOP .. "/PLAT/driver/hal/common/inc",
                SDK_TOP .. "/PLAT/driver/hal/ec7xx/ap/inc",
                SDK_TOP .. "/PLAT/driver/hal/ec7xx/ap/inc/"..CHIP,
                SDK_TOP .. "/PLAT/driver/chip/ec7xx/ap/inc",
                SDK_TOP .. "/PLAT/driver/chip/ec7xx/ap/inc/"..CHIP,
                SDK_TOP .. "/PLAT/driver/chip/ec7xx/ap/inc_cmsis",
                SDK_TOP .. "/PLAT/os/freertos/inc",
                SDK_TOP .. "/PLAT/os/freertos/CMSIS/common/inc",
                SDK_TOP .. "/PLAT/os/freertos/CMSIS/ap/inc",
                SDK_TOP .. "/PLAT/os/freertos/portable/mem/tlsf",
                SDK_TOP .. "/PLAT/os/freertos/portable/gcc",
                SDK_TOP .. "/PLAT/middleware/developed/nvram/ec7xx/inc",
                SDK_TOP .. "/PLAT/middleware/developed/nvram/inc",
                SDK_TOP .. "/PLAT/middleware/developed/cms/psdial/inc",
                SDK_TOP .. "/PLAT/middleware/developed/cms/cms/inc",
                SDK_TOP .. "/PLAT/middleware/developed/cms/psil/inc",
                SDK_TOP .. "/PLAT/middleware/developed/cms/psstk/inc",
                SDK_TOP .. "/PLAT/middleware/developed/cms/sockmgr/inc",
                SDK_TOP .. "/PLAT/middleware/developed/cms/cmsnetlight/inc",
                SDK_TOP .. "/PLAT/middleware/developed/ecapi/aal/inc",
                SDK_TOP .. "/PLAT/middleware/developed/ecapi/appmwapi/inc",
                SDK_TOP .. "/PLAT/middleware/developed/ecapi/psapi/inc",
                SDK_TOP .. "/PLAT/middleware/developed/common/inc",
                SDK_TOP .. "/PLAT/middleware/developed/psnv/inc",
                SDK_TOP .. "/PLAT/os/freertos/portable/gcc",
                SDK_TOP .. "/PLAT/middleware/developed/tcpipmgr/app/inc",
                SDK_TOP .. "/PLAT/middleware/developed/tcpipmgr/common/inc",
                SDK_TOP .. "/PLAT/os/freertos/inc",
                SDK_TOP .. "/PLAT/middleware/developed/yrcompress",
                SDK_TOP .. "/PLAT/prebuild/PS/inc",
                SDK_TOP .. "/PLAT/middleware/thirdparty/lwip/src/include",
                SDK_TOP .. "/PLAT/middleware/thirdparty/lwip/src/include/lwip",
                SDK_TOP .. "/PLAT/middleware/developed/ccio/pub",
                SDK_TOP .. "/PLAT/middleware/developed/ccio/device/inc",
                SDK_TOP .. "/PLAT/middleware/developed/ccio/service/inc",
                SDK_TOP .. "/PLAT/middleware/developed/ccio/custom/inc",
                SDK_TOP .. "/PLAT/middleware/developed/fota/pub",
                SDK_TOP .. "/PLAT/middleware/developed/fota/custom/inc",
                SDK_TOP .. "/PLAT/middleware/thirdparty/lwip/src/include",
                SDK_TOP .. "/PLAT/middleware/thirdparty/lwip/src/include/posix",
                SDK_TOP .. "/PLAT/middleware/developed/at/atdecoder/inc",
                SDK_TOP .. "/PLAT/middleware/developed/at/atps/inc",
                SDK_TOP .. "/PLAT/middleware/developed/at/atps/inc/cnfind",
                SDK_TOP .. "/PLAT/middleware/developed/at/atcust/inc",
                SDK_TOP .. "/PLAT/middleware/developed/at/atcust/inc/cnfind",
                SDK_TOP .. "/PLAT/middleware/developed/at/atentity/inc",
                SDK_TOP .. "/PLAT/middleware/developed/at/atreply/inc",
                SDK_TOP .. "/PLAT/middleware/developed/at/atref/inc",
                SDK_TOP .. "/PLAT/middleware/developed/at/atref/inc/cnfind",
                SDK_TOP .. "/PLAT/tools/"..CHIP_TARGET,
                SDK_TOP .. "/PLAT/core/driver/include",
                SDK_TOP .. "/PLAT/core/common/include",
                SDK_TOP .. "/PLAT/core/multimedia/include",
                SDK_TOP .. "/PLAT/core/tts/include",
				SDK_TOP .. "/PLAT/core/include",
                SDK_TOP .. "/PLAT/prebuild/PS/inc",
                SDK_TOP .. "/PLAT/prebuild/PLAT/inc",
                SDK_TOP .. "/thirdparty/littlefs",
                SDK_TOP .. "/thirdparty/littlefs/port",

{public = true})

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
				SDK_TOP .. "/interface/include",
                {public = true})

add_defines("MBEDTLS_CONFIG_FILE=\"mbedtls_ec7xx_config.h\"","LUAT_USE_FS_VFS")

-- if USER_PROJECT_NAME == 'luatos' then
--     if os.getenv("LUAT_EC7XX_LITE_MODE") == "1" then
--         add_defines("LUAT_EC7XX_LITE_MODE", "LUAT_SCRIPT_SIZE=448", "LUAT_SCRIPT_OTA_SIZE=284")
--     end
--     if os.getenv("LUAT_USE_TTS") == "1" then
--         add_defines("LUAT_USE_TTS")
--     end
--     if os.getenv("LUAT_USE_TTS_ONCHIP") == "1" then
--         add_defines("LUAT_USE_TTS_ONCHIP")
--     end
--     add_defines("__LUATOS__","LWIP_NUM_SOCKETS=8")
-- 	add_defines("OPEN_CPU_MODE")
-- 	is_lspd = true
-- end

--linkflags
local LD_BASE_FLAGS = "-Wl,--cref -Wl,--check-sections -Wl,--gc-sections -lm -Wl,--print-memory-usage"
LD_BASE_FLAGS = LD_BASE_FLAGS .. " -L" .. SDK_TOP .. "/PLAT/device/target/board/ec7xx_0h00/ap/gcc/"
LD_BASE_FLAGS = LD_BASE_FLAGS .. " -T" .. SDK_TOP .. "/PLAT/core/ld/ec7xx_0h00_flash.ld -Wl,-Map,$(buildir)/"..USER_PROJECT_NAME.."/"..USER_PROJECT_NAME.."_$(mode).map "
LD_BASE_FLAGS = LD_BASE_FLAGS .. " -Wl,--wrap=_malloc_r -Wl,--wrap=_free_r -Wl,--wrap=_realloc_r  -mcpu=cortex-m3 -mthumb -DTRACE_LEVEL=5 -DSOFTPACK_VERSION=\"\" -DHAVE_STRUCT_TIMESPEC"

-- add_linkdirs("$(projectdir)/PLAT/libs")
-- add_links("core_airm2m")

local LIB_BASE = SDK_TOP .. "/PLAT/libs/"..CHIP_TARGET.."/libstartup.a "
LIB_BASE = LIB_BASE .. SDK_TOP .. "/PLAT/libs/"..CHIP_TARGET.."/libcore_airm2m.a "
LIB_BASE = LIB_BASE .. SDK_TOP .. "/PLAT/libs/"..CHIP_TARGET.."/libfreertos.a "
LIB_BASE = LIB_BASE .. SDK_TOP .. "/PLAT/libs/"..CHIP_TARGET.."/libpsnv.a "
LIB_BASE = LIB_BASE .. SDK_TOP .. "/PLAT/libs/"..CHIP_TARGET.."/libtcpipmgr.a "
LIB_BASE = LIB_BASE .. SDK_TOP .. "/PLAT/libs/"..CHIP_TARGET.."/libyrcompress.a "
LIB_BASE = LIB_BASE .. SDK_TOP .. "/PLAT/libs/"..CHIP_TARGET.."/libmiddleware_ec.a "
LIB_BASE = LIB_BASE .. SDK_TOP .. "/PLAT/libs/"..CHIP_TARGET.."/liblwip.a "

LIB_BASE = LIB_BASE .. SDK_TOP .. "/PLAT/libs/"..CHIP_TARGET.."/liblzma.a "
if os.getenv("LUAT_FAST_ADD_USER_LIB") == "1" then
    LIB_BASE = LIB_BASE .. SDK_TOP .. os.getenv("USER_LIB") .. " "
end

if is_lspd then
    LIB_PS_PRE = SDK_TOP .. "/PLAT/prebuild/PS/lib/gcc/"..CHIP_TARGET.."/oc"
    LIB_PLAT_PRE = SDK_TOP .. "/PLAT/prebuild/PLAT/lib/gcc/"..CHIP_TARGET.."/oc"
else
    LIB_PS_PRE = SDK_TOP .. "/PLAT/prebuild/PS/lib/gcc/"..CHIP_TARGET
    LIB_PLAT_PRE = SDK_TOP .. "/PLAT/prebuild/PLAT/lib/gcc/"..CHIP_TARGET
end

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

includes(USER_PROJECT_DIR)
add_includedirs(USER_PROJECT_DIR .. "/inc", 
                {public = true})
add_includedirs(USER_PROJECT_DIR .. "/include", 
                {public = true})

target("driver")
    set_kind("static")
    add_deps(USER_PROJECT_NAME)
    --driver
	add_files(SDK_TOP .. "/PLAT/core/speed/*.c",
            SDK_TOP .. "/PLAT/driver/board/ec7xx_0h00/src/*c",
            SDK_TOP .. "/PLAT/driver/hal/**.c",
            SDK_TOP .. "/PLAT/driver/chip/ec7xx/ap/src/"..CHIP.."/adc.c",
            SDK_TOP .. "/PLAT/driver/chip/ec7xx/ap/src/*.c",
            SDK_TOP .. "/PLAT/driver/chip/ec7xx/ap/src/usb/open/*.c",
            SDK_TOP .. "/PLAT/driver/chip/ec7xx/common/gcc/memcpy-armv7m.S",
            {public = true})

	remove_files(
                SDK_TOP .. "/PLAT/driver/chip/ec7xx/ap/src/cspi.c",
                SDK_TOP .. "/PLAT/driver/chip/ec7xx/ap/src/swdio.c"
                )

    set_targetdir("$(buildir)/libdriver_" .. USER_PROJECT_NAME)
target_end()

target(USER_PROJECT_NAME..".elf")
	set_kind("binary")
    set_targetdir("$(buildir)/"..USER_PROJECT_NAME)

    add_deps("driver")

    -- mbedtls
    add_files(LUATOS_ROOT .."components/mbedtls/library/*.c")
    -- network
    add_files(SDK_TOP .. "/interface/src/*.c",
            LUATOS_ROOT .."components/network/adapter/luat_network_adapter.c",
            LUATOS_ROOT .."components/ethernet/common/dns_client.c"
            )

    -- printf
    add_files(LUATOS_ROOT.."components/printf/*.c")
    -- weak
    -- add_files(LUATOS_ROOT.."luat/weak/luat_log_weak.c")
    -- littlefs
    add_files(SDK_TOP.."thirdparty/littlefs/**.c")
    -- vfs
    add_files(LUATOS_ROOT.."luat/vfs/luat_fs_lfs2.c",
            LUATOS_ROOT.."luat/vfs/luat_vfs.c")
    
    -- if USER_PROJECT_NAME ~= 'luatos' then
    --     -- remove_files(
    --     -- )
    --     -- add_files(SDK_TOP.."/thirdparty/flashdb/src/*.c",{public = true})
    -- else
    --     -- remove_files(SDK_TOP .. "/interface/src/luat_kv_ec7xx.c")
    -- end

    local toolchains = nil
    local out_path = nil
    local ld_parameter = nil 
	add_ldflags(LD_BASE_FLAGS .. " -Wl,--whole-archive -Wl,--start-group " .. LIB_BASE .. LIB_USER .. " -Wl,--end-group -Wl,--no-whole-archive -Wl,--no-undefined -Wl,--no-print-map-discarded -ldriver ", {force=true})
	
    on_load(function (target)
		out_path = SDK_PATH .. "/out/" ..USER_PROJECT_NAME
		if not os.exists(out_path) then
			os.mkdir(out_path)
		end
        -- if USER_PROJECT_NAME == 'luatos' then
        --     local conf_data = io.readfile("$(projectdir)/project/luatos/inc/luat_conf_bsp.h")
        --     USER_PROJECT_NAME_VERSION = conf_data:match("#define LUAT_BSP_VERSION \"(%w+)\"")
        --     VM_64BIT = conf_data:find("\r#define LUAT_CONF_VM_64bit") or conf_data:find("\n#define LUAT_CONF_VM_64bit")
        --     local TTS_ONCHIP = conf_data:find("\r#define LUAT_USE_TTS_ONCHIP") or conf_data:find("\n#define LUAT_USE_TTS_ONCHIP")

        --     local mem_map_data = io.readfile("$(projectdir)/PLAT/device/target/board/ec7xx_0h00/common/inc/mem_map.h")
        --     FLASH_FOTA_REGION_START = tonumber(mem_map_data:match("#define FLASH_FOTA_REGION_START%s+%((%g+)%)"))
        --     if TTS_ONCHIP or os.getenv("LUAT_USE_TTS_ONCHIP") == "1" then
        --         LUAT_SCRIPT_SIZE = 64
        --         LUAT_SCRIPT_OTA_SIZE = 48
        --     elseif os.getenv("LUAT_EC718_LITE_MODE") == "1" then
        --         LUAT_SCRIPT_SIZE = 448
        --         LUAT_SCRIPT_OTA_SIZE = 284
        --     else
        --         LUAT_SCRIPT_SIZE = tonumber(conf_data:match("\r#define LUAT_SCRIPT_SIZE (%d+)") or conf_data:match("\n#define LUAT_SCRIPT_SIZE (%d+)"))
        --         LUAT_SCRIPT_OTA_SIZE = tonumber(conf_data:match("\r#define LUAT_SCRIPT_OTA_SIZE (%d+)") or conf_data:match("\n#define LUAT_SCRIPT_OTA_SIZE (%d+)"))
        --     end
        --     print(string.format("script zone %d ota %d", LUAT_SCRIPT_SIZE, LUAT_SCRIPT_OTA_SIZE))
        --     LUA_SCRIPT_ADDR = FLASH_FOTA_REGION_START - (LUAT_SCRIPT_SIZE + LUAT_SCRIPT_OTA_SIZE) * 1024
        --     LUA_SCRIPT_OTA_ADDR = FLASH_FOTA_REGION_START - LUAT_SCRIPT_OTA_SIZE * 1024
        --     script_addr = string.format("%X", LUA_SCRIPT_ADDR)
        --     full_addr = string.format("%X", LUA_SCRIPT_OTA_ADDR)
        --     -- print(FLASH_FOTA_REGION_START,LUAT_SCRIPT_SIZE,LUAT_SCRIPT_OTA_SIZE)
        --     -- print(script_addr,full_addr)
        -- end
    end)
    before_link(function(target)
        toolchains = target:toolchains()[1]:bindir()
        for _, dep in ipairs(target:orderdeps()) do
            local linkdir = dep:targetdir()
            target:add("ldflags","-L./"..linkdir, {force=true})
            -- print("linkdir",linkdir)
        end  

        ld_parameter = {"-E","-P"}
        for _, define_flasg in pairs(target:get("defines")) do
            table.insert(ld_parameter,"-D" .. define_flasg)
        end

        os.execv(toolchains .. "/arm-none-eabi-gcc",table.join(ld_parameter, {"-I",SDK_PATH .. "/PLAT/device/target/board/ec7xx_0h00/common/pkginc"},{"-I",SDK_PATH .. "/PLAT/device/target/board/ec7xx_0h00/common/inc"}, {"-o",SDK_PATH .. "/PLAT/core/ld/ec7xx_0h00_flash.ld","-"}),{stdin = SDK_PATH .. "/PLAT/core/ld/ec7xx_0h00_flash.c"})
        
        mem_parameter = {}
        for _, cx_flasg in pairs(target:get("cxflags")) do
            table.insert(mem_parameter,cx_flasg)
        end
        table.join2(mem_parameter,ld_parameter)
        for _, includedirs_flasg in pairs(target:get("includedirs")) do
            table.insert(mem_parameter,"-I" .. includedirs_flasg)
        end
        os.execv(toolchains .. "/arm-none-eabi-gcc",table.join(mem_parameter, {"-o",out_path .. "/mem_map.txt","-"}),{stdin = SDK_PATH .. "/PLAT/device/target/board/ec7xx_0h00/common/inc/mem_map.h"})
        os.cp(out_path .. "/mem_map.txt", "$(buildir)/"..USER_PROJECT_NAME.."/mem_map.txt")
    end)
	after_build(function(target)
		os.exec(toolchains .. "/arm-none-eabi-objcopy -O binary $(buildir)/"..USER_PROJECT_NAME.."/"..USER_PROJECT_NAME..".elf $(buildir)/"..USER_PROJECT_NAME.."/"..USER_PROJECT_NAME..".bin")
        os.iorun(toolchains .. "/arm-none-eabi-size $(buildir)/"..USER_PROJECT_NAME.."/"..USER_PROJECT_NAME..".elf")
		-- io.writefile("$(buildir)/"..USER_PROJECT_NAME.."/"..USER_PROJECT_NAME..".list", os.iorun(toolchains .. "/arm-none-eabi-objdump -h -S $(buildir)/"..USER_PROJECT_NAME.."/"..USER_PROJECT_NAME..".elf"))
        -- io.writefile("$(buildir)/"..USER_PROJECT_NAME.."/"..USER_PROJECT_NAME..".size", os.iorun(toolchains .. "/arm-none-eabi-size $(buildir)/"..USER_PROJECT_NAME.."/"..USER_PROJECT_NAME..".elf"))
		os.exec(toolchains .. "/arm-none-eabi-objcopy -O binary $(buildir)/"..USER_PROJECT_NAME.."/"..USER_PROJECT_NAME..".elf $(buildir)/"..USER_PROJECT_NAME.."/"..USER_PROJECT_NAME..".bin")
		os.exec(toolchains .."/arm-none-eabi-size $(buildir)/"..USER_PROJECT_NAME.."/"..USER_PROJECT_NAME..".elf")
        os.cp("$(buildir)/"..USER_PROJECT_NAME.."/"..USER_PROJECT_NAME..".bin", "$(buildir)/"..USER_PROJECT_NAME.."/"..USER_PROJECT_NAME.."_unZip.bin")
        os.exec("./PLAT/tools/fcelf.exe -C -bin ".."$(buildir)/"..USER_PROJECT_NAME.."/"..USER_PROJECT_NAME.."_unZip.bin".. " -cfg ".. SDK_PATH .. "/PLAT/device/target/board/ec7xx_0h00/ap/gcc/sectionInfo_"..CHIP_TARGET..".json".. " -map ".."$(buildir)/"..USER_PROJECT_NAME.."/"..USER_PROJECT_NAME.. "_debug.map".." -out ".."$(buildir)/"..USER_PROJECT_NAME.."/" .. USER_PROJECT_NAME .. ".bin")

        os.cp("$(buildir)/"..USER_PROJECT_NAME.."/*.bin", out_path)
		os.cp("$(buildir)/"..USER_PROJECT_NAME.."/*.map", out_path)
		os.cp("$(buildir)/"..USER_PROJECT_NAME.."/*.elf", out_path)
		os.cp("./PLAT/tools/"..CHIP_TARGET.."/comdb.txt", out_path)
        
        ---------------------------------------------------------
        -------------- 这部分尚不能跨平台 -------------------------
        local binpkg = (is_plat("windows") and "./PLAT/tools/fcelf.exe " or "./fcelf ").."-M -input ./PLAT/tools/"..CHIP_TARGET.."/bootloader/ap_bootloader.bin -addrname BL_PKGIMG_LNA -flashsize BOOTLOADER_PKGIMG_LIMIT_SIZE \
                        -input $(buildir)/"..USER_PROJECT_NAME.."/"..USER_PROJECT_NAME ..".bin -addrname AP_PKGIMG_LNA -flashsize AP_PKGIMG_LIMIT_SIZE \
                        -input ./PLAT/prebuild/FW/lib/gcc/"..CHIP_TARGET.."/cp-demo-flash.bin -addrname CP_PKGIMG_LNA -flashsize CP_PKGIMG_LIMIT_SIZE \
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
            archive.archive(out_path.."/"..USER_PROJECT_NAME..".7z", out_path.."/pack/*",options)
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
