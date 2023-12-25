@echo off
set PROJECT_NAME=example
rem you can set your gcc path
rem set GCC_PATH=E:\gcc_mcu
set ROOT_PATH = %cd:\=/%
if not "%1"=="" (
	set PROJECT_NAME=%1
) ELSE (
	@echo PROJECT not set
)

@echo PROJECT : %PROJECT_NAME%  



if not exist xmake.lua (
	echo xmake.lua not exist
	goto end
)
call xmake clean -a
call del /Q .xmake

:end
echo done


