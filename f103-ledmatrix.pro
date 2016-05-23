TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

INCLUDEPATH += \
	project \
	lib/spl/inc \
	lib/cmsis \
	lib/sbmp/library \
	/usr/arm-none-eabi/include \
	/usr/lib/gcc/arm-none-eabi/5.3.0/include/

DEFINES += F_CPU=72000000UL \
		   STM32F10X_MD \
		   USE_STDPERIPH_DRIVER \
		   __null=0 \
		   __STATIC_INLINE="static inline" \
		   __INLINE="inline" \
		   __ASM=__asm \
		   __CORTEX_M=3 \
		   VERBOSE_LOGGING=1

HEADERS += \
	lib/cmsis/core_cm3.h \
	lib/cmsis/stm32f10x.h \
	lib/sbmp/library/crc32.h \
	lib/sbmp/library/payload_builder.h \
	lib/sbmp/library/payload_parser.h \
	lib/sbmp/library/sbmp.h \
	lib/sbmp/library/sbmp_bulk.h \
	lib/sbmp/library/sbmp_checksum.h \
	lib/sbmp/library/sbmp_config.example.h \
	lib/sbmp/library/sbmp_datagram.h \
	lib/sbmp/library/sbmp_frame.h \
	lib/sbmp/library/sbmp_session.h \
	lib/sbmp/library/type_coerce.h \
	lib/spl/inc/misc.h \
	lib/spl/inc/stm32f10x_adc.h \
	lib/spl/inc/stm32f10x_bkp.h \
	lib/spl/inc/stm32f10x_can.h \
	lib/spl/inc/stm32f10x_cec.h \
	lib/spl/inc/stm32f10x_crc.h \
	lib/spl/inc/stm32f10x_dac.h \
	lib/spl/inc/stm32f10x_dbgmcu.h \
	lib/spl/inc/stm32f10x_dma.h \
	lib/spl/inc/stm32f10x_exti.h \
	lib/spl/inc/stm32f10x_flash.h \
	lib/spl/inc/stm32f10x_fsmc.h \
	lib/spl/inc/stm32f10x_gpio.h \
	lib/spl/inc/stm32f10x_i2c.h \
	lib/spl/inc/stm32f10x_iwdg.h \
	lib/spl/inc/stm32f10x_pwr.h \
	lib/spl/inc/stm32f10x_rcc.h \
	lib/spl/inc/stm32f10x_rtc.h \
	lib/spl/inc/stm32f10x_sdio.h \
	lib/spl/inc/stm32f10x_spi.h \
	lib/spl/inc/stm32f10x_tim.h \
	lib/spl/inc/stm32f10x_usart.h \
	lib/spl/inc/stm32f10x_wwdg.h \
	project/stm32f10x_conf.h \
	project/stm32f10x_it.h \
	project/system_stm32f10x.h \
	project/sbmp_config.h \
	project/com/com_fileio.h \
	project/com/com_iface.h \
	project/com/datalink.h \
	project/com/debug.h \
	project/com/iface_noop.h \
	project/com/iface_usart.h \
	project/utils/circbuf.h \
	project/utils/minmax.h \
	project/utils/timebase.h \
	project/colorled.h \
	project/malloc_safe.h \
	project/hw_init.h \
	project/utils/debounce.h \
	project/bus/event_handler.h \
	project/bus/event_queue.h \
	project/utils/str_utils.h \
	project/main.h \
	project/utils/matcher.h \
	project/utils/meanbuf.h \
    project/display.h \
    project/max2719.h \
    project/dotmatrix.h

SOURCES += \
	lib/cmsis/core_cm3.c \
	lib/sbmp/library/crc32.c \
	lib/sbmp/library/payload_builder.c \
	lib/sbmp/library/payload_parser.c \
	lib/sbmp/library/sbmp_bulk.c \
	lib/sbmp/library/sbmp_checksum.c \
	lib/sbmp/library/sbmp_datagram.c \
	lib/sbmp/library/sbmp_frame.c \
	lib/sbmp/library/sbmp_session.c \
	lib/spl/src/misc.c \
	lib/spl/src/stm32f10x_adc.c \
	lib/spl/src/stm32f10x_bkp.c \
	lib/spl/src/stm32f10x_can.c \
	lib/spl/src/stm32f10x_cec.c \
	lib/spl/src/stm32f10x_crc.c \
	lib/spl/src/stm32f10x_dac.c \
	lib/spl/src/stm32f10x_dbgmcu.c \
	lib/spl/src/stm32f10x_dma.c \
	lib/spl/src/stm32f10x_exti.c \
	lib/spl/src/stm32f10x_flash.c \
	lib/spl/src/stm32f10x_fsmc.c \
	lib/spl/src/stm32f10x_gpio.c \
	lib/spl/src/stm32f10x_i2c.c \
	lib/spl/src/stm32f10x_iwdg.c \
	lib/spl/src/stm32f10x_pwr.c \
	lib/spl/src/stm32f10x_rcc.c \
	lib/spl/src/stm32f10x_rtc.c \
	lib/spl/src/stm32f10x_sdio.c \
	lib/spl/src/stm32f10x_spi.c \
	lib/spl/src/stm32f10x_tim.c \
	lib/spl/src/stm32f10x_usart.c \
	lib/spl/src/stm32f10x_wwdg.c \
	project/main.c \
	project/stm32f10x_it.c \
	project/system_stm32f10x.c \
	project/com/com_fileio.c \
	project/com/com_iface.c \
	project/com/datalink.c \
	project/com/debug.c \
	project/com/iface_noop.c \
	project/com/iface_usart.c \
	project/utils/circbuf.c \
	project/utils/debounce.c \
	project/utils/timebase.c \
	project/colorled.c \
	project/hw_init.c \
	project/malloc_safe.c \
	project/spl_assert.c \
	project/syscalls.c \
	project/bus/event_handler.c \
	project/bus/event_queue.c \
	project/utils/str_utils.c \
	project/utils/matcher.c \
	project/utils/meanbuf.c \
    project/display.c \
    project/max2719.c \
    project/dotmatrix.c

DISTFILES += \
	style.astylerc \
	Makefile \
	lib/cmsis/startup_stm32f10x_md.s \
	lib/cmsis/stm32_flash.ld
