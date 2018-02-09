################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include zcl-core-cli/subdir.mk
-include zcl-core/subdir.mk
-include version-debug/subdir.mk
-include stack/thread-stack/EFR32/subdir.mk
-include stack/thread-stack/subdir.mk
-include spidisplay/subdir.mk
-include slaac-client/subdir.mk
-include sim-eeprom1/EFR32/subdir.mk
-include serial/EFR32/subdir.mk
-include serial/subdir.mk
-include scan-debug/subdir.mk
-include rutledge_spi_porting/subdir.mk
-include rutledge-epaper-module/subdir.mk
-include rutledge-common/subdir.mk
-include ota-bootload-storage-eeprom/subdir.mk
-include ota-bootload-storage-cli/subdir.mk
-include ota-bootload-core/subdir.mk
-include ota-bootload-client-policy/subdir.mk
-include ota-bootload-client/subdir.mk
-include on-off-server/subdir.mk
-include network-management-cli/subdir.mk
-include main/subdir.mk
-include kit-drivers/subdir.mk
-include idle-sleep/subdir.mk
-include identify-server/subdir.mk
-include icmp-debug/subdir.mk
-include icmp-cli/subdir.mk
-include hal-library/EFR32/subdir.mk
-include glib/subdir.mk
-include external-generated-files/subdir.mk
-include emlib/subdir.mk
-include emdrv/subdir.mk
-include eeprom/subdir.mk
-include diagnostic-stub/subdir.mk
-include dhcp-client/subdir.mk
-include debug-print/subdir.mk
-include debug-channel-stub/EFR32/subdir.mk
-include connection-manager-jib/subdir.mk
-include connection-manager-cli/subdir.mk
-include command-interpreter2/subdir.mk
-include coap-dispatch/subdir.mk
-include coap-cli/subdir.mk
-include cli/subdir.mk
-include basic-server/subdir.mk
-include antenna-stub/subdir.mk
-include address-configuration-debug/subdir.mk
-include adc/EFR32/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: pre-build main-build

# Main-build Target
main-build: smarttag.axf

# Tool invocations
smarttag.axf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: GNU ARM C Linker'
	arm-none-eabi-gcc -g3 -gdwarf-2 -mcpu=cortex-m4 -mthumb -T "C:/SiliconLabs/SimplicityStudio/v4/developer/sdks/gecko_sdk_suite/v2.1//platform/base/hal/micro/cortexm3/efm32/gcc-cfg.ld" -L"C:/SiliconLabs/SimplicityStudio/v4/developer/sdks/gecko_sdk_suite/v2.1//platform/base/hal/micro/cortexm3/" -Xlinker --defsym=FLASH_SIZE=524288 -Xlinker --defsym=RAM_SIZE=65536 -Xlinker --defsym=SIMEEPROM_SIZE=8192 -Xlinker --defsym=APP_GECKO_INFO_PAGE_BTL=1 -Xlinker --gc-sections -Xlinker -Map="smarttag.map" -mfpu=fpv4-sp-d16 -mfloat-abi=softfp --specs=nano.specs -o smarttag.axf -Wl,--start-group "./zcl-core-cli/zcl-core-cli-implementation.o" "./zcl-core/cbor-encoder.o" "./zcl-core/zcl-core-attribute.o" "./zcl-core/zcl-core-binding.o" "./zcl-core/zcl-core-cache.o" "./zcl-core/zcl-core-cli.o" "./zcl-core/zcl-core-command.o" "./zcl-core/zcl-core-endpoint.o" "./zcl-core/zcl-core-ez-mode.o" "./zcl-core/zcl-core-group.o" "./zcl-core/zcl-core-message.o" "./zcl-core/zcl-core-notification.o" "./zcl-core/zcl-core-reporting.o" "./zcl-core/zcl-core-uri.o" "./zcl-core/zcl-core-well-known.o" "./zcl-core/zcl-core.o" "./zcl-core/zclip-struct.o" "./version-debug/version-debug.o" "./stack/thread-stack/EFR32/SEGGER_RTT.o" "./stack/thread-stack/ember-ip-configuration.o" "./slaac-client/slaac-client.o" "./sim-eeprom1/EFR32/sim-eeprom.o" "./serial/EFR32/ember-printf.o" "./serial/EFR32/serial.o" "./serial/com.o" "./ota-bootload-storage-eeprom/ota-bootload-storage-eeprom.o" "./ota-bootload-storage-cli/ota-bootload-storage-cli.o" "./ota-bootload-core/ota-bootload-core.o" "./ota-bootload-client-policy/ota-bootload-client-policy.o" "./ota-bootload-client/ota-bootload-client.o" "./on-off-server/on-off-server.o" "./network-management-cli/network-management-cli.o" "./main/main.o" "./identify-server/identify-server.o" "./icmp-debug/icmp-debug.o" "./icmp-cli/icmp-cli.o" "./hal-library/EFR32/assert-crash-handlers.o" "./hal-library/EFR32/crc.o" "./hal-library/EFR32/cstartup-common.o" "./hal-library/EFR32/endian.o" "./hal-library/EFR32/faults-v7m.o" "./hal-library/EFR32/hal-config-gpio.o" "./hal-library/EFR32/hal-config.o" "./hal-library/EFR32/isr-stubs.o" "./hal-library/EFR32/mem-util.o" "./hal-library/EFR32/mfg-token.o" "./hal-library/EFR32/micro-common.o" "./hal-library/EFR32/micro.o" "./hal-library/EFR32/mpu.o" "./hal-library/EFR32/random.o" "./hal-library/EFR32/sleep-efm32.o" "./hal-library/EFR32/token-def.o" "./hal-library/EFR32/token.o" "./external-generated-files/bootloader-interface-app.o" "./external-generated-files/bootloader-interface.o" "./external-generated-files/btl_interface.o" "./external-generated-files/btl_interface_storage.o" "./external-generated-files/stub-certificates.o" "./emlib/em_adc.o" "./emlib/em_cmu.o" "./emlib/em_core.o" "./emlib/em_emu.o" "./emlib/em_gpio.o" "./emlib/em_i2c.o" "./emlib/em_ldma.o" "./emlib/em_leuart.o" "./emlib/em_mpu.o" "./emlib/em_msc.o" "./emlib/em_prs.o" "./emlib/em_rmu.o" "./emlib/em_rtcc.o" "./emlib/em_system.o" "./emlib/em_timer.o" "./emlib/em_usart.o" "./emlib/em_wdog.o" "./emlib/system_efr32mg13p.o" "./emdrv/dmadrv.o" "./emdrv/gpiointerrupt.o" "./emdrv/rtcdriver.o" "./emdrv/sleep.o" "./emdrv/tempdrv.o" "./emdrv/uartdrv.o" "./emdrv/ustimer.o" "./eeprom/eeprom-cli.o" "./eeprom/eeprom-print.o" "./eeprom/eeprom.o" "./diagnostic-stub/diagnostic-stub.o" "./dhcp-client/dhcp-client.o" "./debug-print/debug-print.o" "./debug-channel-stub/EFR32/debug-jtag-stub.o" "./connection-manager-jib/connection-manager-jib.o" "./connection-manager-cli/connection-manager-cli.o" "./command-interpreter2/command-interpreter2-error.o" "./command-interpreter2/command-interpreter2-util.o" "./command-interpreter2/command-interpreter2.o" "./coap-dispatch/coap-dispatch.o" "./coap-cli/coap-cli.o" "./cli/cli.o" "./basic-server/basic-server.o" "./antenna-stub/antenna-stub.o" "./address-configuration-debug/address-configuration-debug.o" "./adc/EFR32/adc-efr32.o" "./sensor-actuator-implementation.o" "./thread-bookkeeping.o" "./thread-callback-stubs.o" "./thread-callbacks.o" "./thread-cli.o" "./thread-coap-dispatch.o" "./thread-events.o" "./thread-zclip.o" "C:/SiliconLabs/SimplicityStudio/v4/developer/sdks/gecko_sdk_suite/v2.1//protocol/thread_2.5/build/hal-library-cortexm3-gcc-efr32mg13p-rail/hal-library.a" "C:/SiliconLabs/SimplicityStudio/v4/developer/sdks/gecko_sdk_suite/v2.1//protocol/thread_2.5/build/dhcp-library-cortexm3-gcc-efr32mg13p-rail/dhcp-library.a" "C:/SiliconLabs/SimplicityStudio/v4/developer/sdks/gecko_sdk_suite/v2.1//protocol/thread_2.5/build/sim-eeprom1-library-cortexm3-gcc-efr32mg13p-rail/sim-eeprom1-library.a" "C:/SiliconLabs/SimplicityStudio/v4/developer/sdks/gecko_sdk_suite/v2.1//protocol/thread_2.5/build/mbedtls-library-cortexm3-gcc-efr32mg13p-rail/mbedtls-library.a" "C:/SiliconLabs/SimplicityStudio/v4/developer/sdks/gecko_sdk_suite/v2.1//protocol/thread_2.5/build/thread-stack-cortexm3-gcc-efr32mg13p-rail/thread-stack.a" "C:/SiliconLabs/SimplicityStudio/v4/developer/sdks/gecko_sdk_suite/v2.1//platform/radio/rail_lib/autogen/librail_release/librail_efr32xg13_gcc_release.a" -lm "./glib/bmp.o" "./glib/dmd_display.o" "./glib/glib.o" "./glib/glib_bitmap.o" "./glib/glib_circle.o" "./glib/glib_font_narrow_6x8.o" "./glib/glib_font_normal_8x8.o" "./glib/glib_font_number_16x20.o" "./glib/glib_line.o" "./glib/glib_polygon.o" "./glib/glib_rectangle.o" "./glib/glib_string.o" "./idle-sleep/idle-sleep.o" "./kit-drivers/udelay.o" "./rutledge-common/common.o" "./rutledge-epaper-module/Display_EPD_W21.o" "./rutledge-epaper-module/font12epd.o" "./rutledge-epaper-module/font16epd.o" "./rutledge-epaper-module/font20epd.o" "./rutledge-epaper-module/font8epd.o" "./rutledge-epaper-module/hal.o" "./rutledge-epaper-module/image.o" "./rutledge_spi_porting/epd_spi.o" "./scan-debug/scan-debug.o" "./spidisplay/display.o" "./spidisplay/displayls013b7dh03.o" "./spidisplay/displaypalemlib.o" -Wl,--end-group -Wl,--start-group -lgcc -lc -lnosys -Wl,--end-group
	@echo 'Finished building target: $@'
	@echo ' '

	@echo 'Building hex file: smarttag.hex'
	arm-none-eabi-objcopy -O ihex "smarttag.axf" "smarttag.hex"
	@echo ' '

	@echo 'Building bin file: smarttag.bin'
	arm-none-eabi-objcopy -O binary "smarttag.axf" "smarttag.bin"
	@echo ' '

	@echo 'Building s37 file: smarttag.s37'
	arm-none-eabi-objcopy -O srec "smarttag.axf" "smarttag.s37"
	@echo ' '

	@echo 'Running size tool'
	arm-none-eabi-size "smarttag.axf"
	@echo ' '

	$(MAKE) --no-print-directory post-build

# Other Targets
clean:
	-$(RM) $(EXECUTABLES)$(OBJS)$(C_DEPS) smarttag.axf
	-@echo ' '

pre-build:
	-cmd //c 'C:\Users\TERREN~1\SIMPLI~1\V4_WOR~1\smarttag/smarttag-prebuild.bat' 'smarttag' 'C:/Users/Terrence goh/SimplicityStudio/v4_workspace/smarttag' '' 'C:\SiliconLabs\SimplicityStudio\v4'
	-@echo ' '

post-build:
	-cmd //c 'C:\Users\TERREN~1\SIMPLI~1\V4_WOR~1\smarttag/smarttag-postbuild.bat' 'smarttag' 'C:/Users/Terrence goh/SimplicityStudio/v4_workspace/smarttag' '' 'C:\SiliconLabs\SimplicityStudio\v4'
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY: main-build pre-build post-build

-include ../makefile.targets
