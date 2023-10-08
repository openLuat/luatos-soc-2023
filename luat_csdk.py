
import os
import shutil

CUR_DIR = os.path.dirname(__file__)
SDK_DIR = os.path.join(CUR_DIR, "PLAT")
CSDK_OUTPUT_DIR = os.path.join(CUR_DIR, "CSDK")
CSDK_PLAT_DIR = os.path.join(CSDK_OUTPUT_DIR, "PLAT")

if __name__ == '__main__':
    # 删除之前的csdk目录
    if os.path.exists(CSDK_OUTPUT_DIR):
        shutil.rmtree(CSDK_OUTPUT_DIR)
    # 创建新的csdk目录
    os.mkdir(CSDK_OUTPUT_DIR)
    os.mkdir(CSDK_PLAT_DIR)

    # -------- 开始拷贝 --------
    for dir in ["core","device","driver","libs","middleware","os","prebuild"]:
        shutil.copytree(os.path.join(SDK_DIR, dir), os.path.join(CSDK_PLAT_DIR, dir), 
                        ignore=shutil.ignore_patterns('*.c','*.mk','*.ld','*.inc','*.html','*.sct','*.bat','*.sh','*.txt',
                                                    'bzip2','cis_onenet','cjson','ctlwm2m','ctwing','dm','er-coap-13','httpclient',
                                                    'iperf','littlefs','lzma2201','mbedtls','mqtt','ping','sntp','wakaama_core'))
        if dir == "core":
            shutil.copyfile(os.path.join(SDK_DIR, "core", "speed","lte_speed.c"), os.path.join(CSDK_PLAT_DIR, "core", "speed","lte_speed.c"))
            # 拷贝生成ld的.c
            shutil.copyfile(os.path.join(SDK_DIR, "core", "ld","ec718p_0h00_flash.c"), os.path.join(CSDK_PLAT_DIR, "core", "ld","ec718p_0h00_flash.c"))

    # 拷贝 tools
    os.mkdir(os.path.join(CSDK_PLAT_DIR,"tools"))
    shutil.copyfile(os.path.join(SDK_DIR,"tools","ap_bootloader.bin"), os.path.join(CSDK_PLAT_DIR,"tools","ap_bootloader.bin"))
    shutil.copyfile(os.path.join(SDK_DIR,"tools","fcelf.exe"), os.path.join(CSDK_PLAT_DIR,"tools","fcelf.exe"))
    # 拷贝 comdb.txt
    shutil.copyfile(os.path.join(SDK_DIR,"comdb.txt"), os.path.join(CSDK_PLAT_DIR,"comdb.txt"))

    # 移除移芯的公司名
    for dirpath, dirnames, filenames in os.walk(CSDK_PLAT_DIR):
        for name in filenames :
            if name.endswith(".c") or name.endswith(".h") or name.endswith(".txt") :
                path = os.path.join(dirpath, name)
                with open(path, "rb") as f :
                    data = f.read()
                old_data = data
                if b'EigenComm' in data:
                    data = data.replace(b'EigenComm', b'AirM2M')
                if b'EIGENCOMM' in data:
                    data = data.replace(b'EIGENCOMM', b'AirM2M')
                if b'Eigencomm' in data:
                    data = data.replace(b'Eigencomm', b'AirM2M')
                if b'EigenCOMM' in data:
                    data = data.replace(b'EigenCOMM', b'AirM2M')
                if b'eigencomm' in data:
                    data = data.replace(b'eigencomm', b'AirM2M')
                
                if old_data != data :
                    with open(path, "wb") as f :
                        f.write(data)

    print(" ---------- done ---------- ")










