local TARGET_NAME = "download_play_mp3"
local LIB_DIR = "$(buildir)/".. TARGET_NAME .. "/"
local LIB_NAME = "lib" .. TARGET_NAME .. ".a "
target(TARGET_NAME)
    set_kind("static")
    set_targetdir(LIB_DIR)
    -- set_warnings("error")
    --加入代码和头文件
    add_includedirs("./include",{public = true})
    add_files("./src/*.c",{public = true})
	

    LIB_USER = LIB_USER .. SDK_TOP .. LIB_DIR .. LIB_NAME .. " "
target_end()