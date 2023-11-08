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
	
    add_linkgroups(TARGET_NAME, {group = true, whole = true})
    
target_end()