local TARGET_NAME = os.scriptdir():match(".+[/\\]([%w_]+)")
local LIB_DIR = "$(buildir)/".. TARGET_NAME .. "/"
local LIB_NAME = "lib" .. TARGET_NAME .. ".a "

target(TARGET_NAME)
    set_kind("static")
    set_targetdir(LIB_DIR)
    add_includedirs("./inc",{public = true})
    add_files("./src/*.c",{public = true})
    add_defines("LUAT_USE_TLS",{public = true})
    add_includedirs("$(projectdir)/thirdparty/LinkSDK/core",{public = true})
    add_includedirs("$(projectdir)/thirdparty/LinkSDK/core/utils",{public = true})
    add_includedirs("$(projectdir)/thirdparty/LinkSDK/core/sysdep",{public = true})
    add_includedirs("$(projectdir)/thirdparty/LinkSDK/components/data-model",{public = true})
    add_includedirs("$(projectdir)/thirdparty/LinkSDK/components/dynreg",{public = true})
    add_includedirs("$(projectdir)/thirdparty/LinkSDK/components/dynreg-mqtt",{public = true})
    add_includedirs("$(projectdir)/thirdparty/LinkSDK/components/ota",{public = true})
    add_files("$(projectdir)/thirdparty/LinkSDK/core/utils/*.c")
    add_files("$(projectdir)/thirdparty/LinkSDK/core/*.c")
    add_files("$(projectdir)/thirdparty/LinkSDK/core/sysdep/*.c")
    add_files("$(projectdir)/thirdparty/LinkSDK/external/*.c",{public = true})
    add_files("$(projectdir)/thirdparty/LinkSDK/components/data-model/*.c",{public = true})
    add_files("$(projectdir)/thirdparty/LinkSDK/components/dynreg/*.c",{public = true})
    add_files("$(projectdir)/thirdparty/LinkSDK/components/dynreg-mqtt/*.c",{public = true})
    add_files("$(projectdir)/thirdparty/LinkSDK/components/ota/*.c",{public = true})
    add_files("$(projectdir)/thirdparty/LinkSDK/portfiles/aiot_port/*.c",{public = true})
target_end()
