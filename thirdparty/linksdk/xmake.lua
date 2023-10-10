
target("linksdk")
    local LIB_DIR = "$(buildir)/linksdk/"
    set_kind("static")
    set_targetdir(LIB_DIR)

    -- cjson
    add_includedirs(LUATOS_ROOT.."components/cjson",{public = true})
    add_files(LUATOS_ROOT.."components/cjson/*.c")

    --加入代码和头文件
    add_includedirs("./components/bootstrap",
                    "./components/data-model",
                    "./components/devinfo",
                    "./components/diag",
                    "./components/dynreg",
                    "./components/dynreg-mqtt",
                    "./components/logpost",
                    "./components/ntp",
                    "./components/ota",
                    "./components/shadow",
                    "./components/subdev",
                    "./components/task",
                    "./core",
                    "./core/sysdep",
                    "./core/utils",
                    {public = true})

    add_files("./components/**.c",
                "./core/**.c",
                "./components/**.c",
                "./core/utils/*.c",
                "./external/*.c",
                "./portfiles/aiot_port/*.c",
                {public = true})

    --可以继续增加add_includedirs和add_files
    --自动链接
    LIB_USER = LIB_USER .. SDK_TOP .. LIB_DIR .. "liblinksdk.a "
target_end()