local TARGET_NAME = os.scriptdir():match(".+[/\\]([%w_]+)")
local LIB_DIR = "$(buildir)/".. TARGET_NAME .. "/"

target(TARGET_NAME)
    set_kind("static")
    set_targetdir(LIB_DIR)
    set_warnings("error")
    --加入代码和头文件
    add_includedirs("./inc",{public = true})
    add_files("./src/**.c")
    
    local chip_target = get_config("chip_target")
    -- cjson
	if chip_target ~= "ec718pv" then
		add_files(LUATOS_ROOT.."/components/cjson/*.c")
	end

target_end()