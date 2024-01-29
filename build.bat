@echo off
set PROJECT_NAME=example
set LSPD_MODE=enable
rem force to use audio module to support noise reduction ,disable is auto detect
set DENOISE_FORCE=disable
rem you can set your gcc path
rem set GCC_PATH=E:\gcc_mcu
@REM set CHIP_TARGET=ec718e
set CHIP_TARGET=ec718p
@REM set CHIP_TARGET=ec718pv
@REM set CHIP_TARGET=ec718s
@REM set CHIP_TARGET=ec716s
rem you can set your outside project
rem set PROJECT_DIR=F:\example_test
if not "%1"=="" (
	set PROJECT_NAME=%1
) ELSE (
	@echo PROJECT not set
)


if not "%2"=="" (
	set LSPD_MODE=%2
) ELSE (
	if "%1" == "luatos" (
		set LSPD_MODE=enable
	) ELSE (
		@echo LSPD_MODE not set
	)
)

@echo "=============================="
@echo "AirM2M https://openluat.com"
@echo "=============================="
@echo PROJECT   : %PROJECT_NAME%  
@echo CHIP_TARGET : %CHIP_TARGET%  
@echo LSPD_MODE : %LSPD_MODE%  
@echo "=============================="

if not exist xmake.lua (
	echo xmake.lua not exist
	goto end
)
call xmake f --chip_target=%CHIP_TARGET% --lspd_mode=%LSPD_MODE% --denoise_force=%DENOISE_FORCE%
if "%3"=="-v" (
	call xmake -v
) ELSE (
	call xmake -w
)

:end
echo end


