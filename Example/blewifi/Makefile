##### ARM GCC Toolchain Development #####
FOLDER  = ./Output-GCC
LD_SCRIPT = ../../SDK/APS/project/opl1000/link.ld
PATCH_LIB = ../../SDK/APS/targets/opl1000/Output-GCC/opl1000_app_m3.a
# Output file names
OUTPUT = opl1000_app_m3
OUTPUT_TRANSPARNET = opl1000_app_m3_transparent

# Compiler & Linker
CROSS_COMPILE = arm-none-eabi-
CC      = $(CROSS_COMPILE)gcc
OBJDUMP = $(CROSS_COMPILE)objdump
OBJCOPY = $(CROSS_COMPILE)objcopy
AR      = $(CROSS_COMPILE)ar

#############  CFLAGS   ##############

# Options for specific architecture
DEFS  = -DGCC
DEFS += -DARMCM3
DEFS += -DOPL1000_APS
DEFS += -D__noSIMULATOR__
DEFS += -D__BLE__
DEFS += -D__LE_HOST_USE_CMD__
DEFS += -D__LWIP_TASK__
DEFS += -D__WPA_SUPPLICANT__
DEFS += -D__noHW_CRYPTO_ENGINE__
DEFS += -D__WIFI_MAC_TASK__
DEFS += -D__NL1000_An__
DEFS += -D__PMP_ENABLE__
DEFS += -D__PMP_REGION__
DEFS += -D__HEAP_REGION__
DEFS += -DnoLWIP_DYNAMIC_DEBUG_ENABLE
DEFS += -D__AT_CMD_TASK__
DEFS += -D__noRTL_SIMULATION__
DEFS += -D__WIFI_AUTO_CONNECT__
DEFS += -D_ARMABI_PURE= 

DEFS_TRANSPARENT = -D__BLEWIFI_TRANSPARENT__

CFLAGS  = $(DEFS)
CFLAGS += -mthumb
CFLAGS += -mcpu=cortex-m3
CFLAGS += -mlittle-endian
CFLAGS += -std=c99
CFLAGS += -Os
CFLAGS += -x none
CFLAGS += -include sys_common.h
CFLAGS += -nostartfiles
CFLAGS += -no-pie
CFLAGS += -fdata-sections
CFLAGS += -ffunction-sections
CFLAGS += -specs=nosys.specs
CFLAGS += -specs=nano.specs
CFLAGS += -Xlinker "--just-symbols=../../SDK/APS/targets/opl1000/object/symdefs_gcc.o"
CFLAGS += -Wno-implicit-function-declaration
CFLAGS += -Wno-int-conversion

LFLAGS  = -T $(LD_SCRIPT)
LFLAGS += -Wl,--gc-sections
LFLAGS += -Wl,--entry=__Patch_EntryPoint
LFLAGS += -Wl,-Map=$(FOLDER)/$(OUTPUT).map

LFLAGS_TRANSPARNET  = -T $(LD_SCRIPT)
LFLAGS_TRANSPARNET += -Wl,--gc-sections
LFLAGS_TRANSPARNET += -Wl,--entry=__Patch_EntryPoint
LFLAGS_TRANSPARNET += -Wl,-Map=$(FOLDER)/$(OUTPUT_TRANSPARNET).map

#############   include    ##############
INCDIR  = -I ../../SDK/APS/apps/le_app/cmd_app
INCDIR += -I ../../SDK/APS/apps/le_app/mtc_app
INCDIR += -I ../../SDK/APS/apps/le_app/pts_app
INCDIR += -I ../../SDK/APS/driver/chip/opl1000/hal_auxadc
INCDIR += -I ../../SDK/APS/driver/chip/opl1000/hal_dbg_uart
INCDIR += -I ../../SDK/APS/driver/chip/opl1000/hal_dma
INCDIR += -I ../../SDK/APS/driver/chip/opl1000/hal_i2c
INCDIR += -I ../../SDK/APS/driver/chip/opl1000/hal_patch
INCDIR += -I ../../SDK/APS/driver/chip/opl1000/hal_pwm
INCDIR += -I ../../SDK/APS/driver/chip/opl1000/hal_spi
INCDIR += -I ../../SDK/APS/driver/chip/opl1000/hal_system
INCDIR += -I ../../SDK/APS/driver/chip/opl1000/hal_tick
INCDIR += -I ../../SDK/APS/driver/chip/opl1000/hal_tmr
INCDIR += -I ../../SDK/APS/driver/chip/opl1000/hal_uart
INCDIR += -I ../../SDK/APS/driver/chip/opl1000/hal_vic
INCDIR += -I ../../SDK/APS/driver/chip/opl1000/hal_wdt
INCDIR += -I ../../SDK/APS/driver/chip/opl1000/securityipdriver
INCDIR += -I ../../SDK/APS/driver/CMSIS/Include
INCDIR += -I ../../SDK/APS/driver/CMSIS/Device/opl1000/Include
INCDIR += -I ../../SDK/APS/FreeRtos/Source/include
INCDIR += -I ../../SDK/APS/FreeRtos/Source/portable/GCC/ARM_CM3
INCDIR += -I ../../SDK/APS/middleware/netlink/at
INCDIR += -I ../../SDK/APS/middleware/netlink/ble_controller_layer/inc
INCDIR += -I ../../SDK/APS/middleware/netlink/cli
INCDIR += -I ../../SDK/APS/middleware/netlink/common/sys_api
INCDIR += -I ../../SDK/APS/middleware/netlink/common/sys_api/rom_if
INCDIR += -I ../../SDK/APS/middleware/netlink/common/sys_ctrl
INCDIR += -I ../../SDK/APS/middleware/netlink/controller_task
INCDIR += -I ../../SDK/APS/middleware/netlink/data_flow
INCDIR += -I ../../SDK/APS/middleware/netlink/diag_task
INCDIR += -I ../../SDK/APS/middleware/netlink/le_stack
INCDIR += -I ../../SDK/APS/middleware/netlink/msg
INCDIR += -I ../../SDK/APS/middleware/netlink/mw_fim
INCDIR += -I ../../SDK/APS/middleware/netlink/mw_ota
INCDIR += -I ../../SDK/APS/middleware/netlink/ps_task
INCDIR += -I ../../SDK/APS/middleware/netlink/wifi_controller_layer
INCDIR += -I ../../SDK/APS/middleware/netlink/wifi_controller_layer/rom_if
INCDIR += -I ../../SDK/APS/middleware/netlink/wifi_mac
INCDIR += -I ../../SDK/APS/middleware/third_party/lwip-2.0.3
INCDIR += -I ../../SDK/APS/middleware/third_party/lwip-2.0.3/lwip/src/include
INCDIR += -I ../../SDK/APS/middleware/third_party/lwip-2.0.3/ports/freertos/include
INCDIR += -I ../../SDK/APS/middleware/third_party/lwip-2.0.3/ports/freertos/netif
INCDIR += -I ../../SDK/APS/middleware/third_party/lwip-2.0.3/ports/rom_if
INCDIR += -I ../../SDK/APS/middleware/third_party/mbedtls/include
INCDIR += -I ../../SDK/APS/middleware/third_party/tinycrypt/include
INCDIR += -I ../../SDK/APS/middleware/third_party/wpa_supplicant-0.7.3/src/ap
INCDIR += -I ../../SDK/APS/middleware/third_party/wpa_supplicant-0.7.3/src/common
INCDIR += -I ../../SDK/APS/middleware/third_party/wpa_supplicant-0.7.3/src/crypto
INCDIR += -I ../../SDK/APS/middleware/third_party/wpa_supplicant-0.7.3/src/drivers
INCDIR += -I ../../SDK/APS/middleware/third_party/wpa_supplicant-0.7.3/src/eap_common
INCDIR += -I ../../SDK/APS/middleware/third_party/wpa_supplicant-0.7.3/src/eap_peer
INCDIR += -I ../../SDK/APS/middleware/third_party/wpa_supplicant-0.7.3/src/eapol_auth
INCDIR += -I ../../SDK/APS/middleware/third_party/wpa_supplicant-0.7.3/src/eapol_supp
INCDIR += -I ../../SDK/APS/middleware/third_party/wpa_supplicant-0.7.3/src/l2_packet
INCDIR += -I ../../SDK/APS/middleware/third_party/wpa_supplicant-0.7.3/src/radius
INCDIR += -I ../../SDK/APS/middleware/third_party/wpa_supplicant-0.7.3/src/rsn_supp
INCDIR += -I ../../SDK/APS/middleware/third_party/wpa_supplicant-0.7.3/src/utils
INCDIR += -I ../../SDK/APS/middleware/third_party/wpa_supplicant-0.7.3/src/wps
INCDIR += -I ../../SDK/APS/middleware/third_party/wpa_supplicant-0.7.3/wpa_supplicant
INCDIR += -I ../../SDK/APS/middleware/third_party/wpa_supplicant-0.7.3/wpa_supplicant/dbus
INCDIR += -I ../../SDK/APS/project/common
INCDIR += -I ../../SDK/APS/project/opl1000/boot_sequence
INCDIR += -I ../../SDK/APS/project/opl1000/include
INCDIR += -I ../../SDK/APS/project/opl1000/startup

INCDIR += -I ../../SDK/APS_PATCH/apps/le_app/include
INCDIR += -I ../../SDK/APS_PATCH/driver/chip/opl1000/hal_auxadc
INCDIR += -I ../../SDK/APS_PATCH/driver/chip/opl1000/hal_dbg_uart
INCDIR += -I ../../SDK/APS_PATCH/driver/chip/opl1000/hal_i2c
INCDIR += -I ../../SDK/APS_PATCH/driver/chip/opl1000/hal_pwm
INCDIR += -I ../../SDK/APS_PATCH/driver/chip/opl1000/hal_spi
INCDIR += -I ../../SDK/APS_PATCH/driver/chip/opl1000/hal_system
INCDIR += -I ../../SDK/APS_PATCH/driver/chip/opl1000/hal_uart
INCDIR += -I ../../SDK/APS_PATCH/driver/chip/opl1000/hal_vic
INCDIR += -I ../../SDK/APS_PATCH/driver/chip/opl1000/securityipdriver
INCDIR += -I ../../SDK/APS_PATCH/middleware/netlink/at
INCDIR += -I ../../SDK/APS_PATCH/middleware/netlink/ble_controller_layer/inc
INCDIR += -I ../../SDK/APS_PATCH/middleware/netlink/cli
INCDIR += -I ../../SDK/APS_PATCH/middleware/netlink/common/sys_api
INCDIR += -I ../../SDK/APS_PATCH/middleware/netlink/controller_task
INCDIR += -I ../../SDK/APS_PATCH/middleware/netlink/data_flow
INCDIR += -I ../../SDK/APS_PATCH/middleware/netlink/diag_task
INCDIR += -I ../../SDK/APS_PATCH/middleware/netlink/le_stack/patch
INCDIR += -I ../../SDK/APS_PATCH/middleware/netlink/msg
INCDIR += -I ../../SDK/APS_PATCH/middleware/netlink/mw_fim
INCDIR += -I ../../SDK/APS_PATCH/middleware/netlink/ps_task
INCDIR += -I ../../SDK/APS_PATCH/middleware/netlink/wifi_controller_layer
INCDIR += -I ../../SDK/APS_PATCH/middleware/netlink/wifi_controller_layer/rom_if
INCDIR += -I ../../SDK/APS_PATCH/middleware/netlink/wifi_mac
INCDIR += -I ../../SDK/APS_PATCH/middleware/third_party/httpclient
INCDIR += -I ../../SDK/APS_PATCH/middleware/third_party/lwip-2.0.3
INCDIR += -I ../../SDK/APS_PATCH/middleware/third_party/lwip-2.0.3/lwip/src/include
INCDIR += -I ../../SDK/APS_PATCH/middleware/third_party/lwip-2.0.3/ports/rom_if
INCDIR += -I ../../SDK/APS_PATCH/middleware/third_party/wpa_supplicant-0.7.3/src/drivers
INCDIR += -I ../../SDK/APS_PATCH/middleware/third_party/wpa_supplicant-0.7.3/src/rsn_supp
INCDIR += -I ../../SDK/APS_PATCH/middleware/third_party/wpa_supplicant-0.7.3/src/utils
INCDIR += -I ../../SDK/APS_PATCH/middleware/third_party/wpa_supplicant-0.7.3/wpa_supplicant
INCDIR += -I ../../SDK/APS_PATCH/project/opl1000/include
INCDIR += -I ../../SDK/APS_PATCH/project/opl1000/startup

INCDIR += -I ./src
INCDIR += -I ./src/btn_press_ctrl
INCDIR += -I ./src/iot_data
INCDIR += -I ./src/smart_sleep

#############   source    ##############
M_SRC  = ./src/app_at_cmd.c
M_SRC += ./src/blewifi_app.c
M_SRC += ./src/blewifi_ble_api.c
M_SRC += ./src/blewifi_common.c
M_SRC += ./src/blewifi_ctrl.c
M_SRC += ./src/blewifi_ctrl_http_ota.c
M_SRC += ./src/blewifi_data.c
M_SRC += ./src/blewifi_http_ota.c
M_SRC += ./src/blewifi_server_app.c
M_SRC += ./src/blewifi_server_app_gatt.c
M_SRC += ./src/blewifi_wifi_api.c
M_SRC += ./src/main_patch.c
M_SRC += ./src/mw_fim_default_group11_project.c
M_SRC += ./src/btn_press_ctrl/btn_press_ctrl.c
M_SRC += ./src/iot_data/iot_data.c
M_SRC += ./src/iot_data/iot_rb_data.c
M_SRC += ./src/smart_sleep/smart_sleep.c
#############   build   ##############
all: 
	@mkdir $(FOLDER)
	@$(CC) -o $(FOLDER)/$(OUTPUT).elf $(LFLAGS) $(CFLAGS) $(INCDIR) $(M_SRC) $(PATCH_LIB)
	$(OBJCOPY) -O binary $(FOLDER)/$(OUTPUT).elf $(FOLDER)/$(OUTPUT).bin
	@$(CC) -o $(FOLDER)/$(OUTPUT_TRANSPARNET).elf $(LFLAGS_TRANSPARNET) $(DEFS_TRANSPARENT) $(CFLAGS) $(INCDIR) $(M_SRC) $(PATCH_LIB)
	$(OBJCOPY) -O binary $(FOLDER)/$(OUTPUT_TRANSPARNET).elf $(FOLDER)/$(OUTPUT_TRANSPARNET).bin
    
#############   clean   ##############  
clean:
	@rm -rf $(FOLDER)