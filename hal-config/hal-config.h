#ifndef HAL_CONFIG_H
#define HAL_CONFIG_H

#include "em_device.h"
#include "hal-config-types.h"

// This file is auto-generated by Hardware Configurator in Simplicity Studio.
// Any content between $[ and ]$ will be replaced whenever the file is regenerated.
// Content outside these regions will be preserved.

// $[ACMP0]
// [ACMP0]$

// $[ACMP1]
// [ACMP1]$

// $[ADC0]
// [ADC0]$

// $[ANTDIV]
// [ANTDIV]$

// $[BATTERYMON]
// [BATTERYMON]$

// $[BTL_BUTTON]
// [BTL_BUTTON]$

// $[BULBPWM]
// [BULBPWM]$

// $[BULBPWM_COLOR]
// [BULBPWM_COLOR]$

// $[BUTTON]
// [BUTTON]$

// $[CMU]
#define HAL_CLK_HFCLK_SOURCE                          (HAL_CLK_HFCLK_SOURCE_HFXO)
#define HAL_CLK_LFECLK_SOURCE                         (HAL_CLK_LFCLK_SOURCE_LFRCO)
#define HAL_CLK_LFBCLK_SOURCE                         (HAL_CLK_LFCLK_SOURCE_LFRCO)
#define BSP_CLK_LFXO_PRESENT                          (1)
#define BSP_CLK_HFXO_PRESENT                          (1)
#define BSP_CLK_LFXO_INIT                              CMU_LFXOINIT_DEFAULT
#define BSP_CLK_LFXO_CTUNE                            (0U)
#define BSP_CLK_LFXO_FREQ                             (32768U)
#define HAL_CLK_LFACLK_SOURCE                         (HAL_CLK_LFCLK_SOURCE_LFRCO)
#define BSP_CLK_HFXO_FREQ                             (38400000UL)
#define BSP_CLK_HFXO_CTUNE                            (349)
#define BSP_CLK_HFXO_INIT                              CMU_HFXOINIT_DEFAULT
#define BSP_CLK_HFXO_CTUNE_TOKEN                      (1)
#define HAL_CLK_HFXO_AUTOSTART                        (HAL_CLK_HFXO_AUTOSTART_NONE)
// [CMU]$

// $[COEX]
// [COEX]$

// $[CS5463]
// [CS5463]$

// $[CSEN0]
// [CSEN0]$

// $[DCDC]
#define BSP_DCDC_PRESENT                              (1)

#define HAL_DCDC_BYPASS                               (0)
#define BSP_DCDC_INIT                                  EMU_DCDCINIT_DEFAULT
// [DCDC]$

// $[EMU]
// [EMU]$

// $[EXTFLASH]
// [EXTFLASH]$

// $[EZRADIOPRO]
// [EZRADIOPRO]$

// $[GPIO]
#define PORTIO_GPIO_SWV_PIN                           (2U)
#define PORTIO_GPIO_SWV_PORT                          (gpioPortF)
#define PORTIO_GPIO_SWV_LOC                           (0U)

// [GPIO]$

// $[I2C0]
#define PORTIO_I2C0_SCL_PIN                           (10U)
#define PORTIO_I2C0_SCL_PORT                          (gpioPortC)
#define PORTIO_I2C0_SCL_LOC                           (14U)

#define PORTIO_I2C0_SDA_PIN                           (11U)
#define PORTIO_I2C0_SDA_PORT                          (gpioPortC)
#define PORTIO_I2C0_SDA_LOC                           (16U)

// [I2C0]$

// $[I2C1]
// [I2C1]$

// $[I2CSENSOR]
// [I2CSENSOR]$

// $[IDAC0]
// [IDAC0]$

// $[IOEXP]
// [IOEXP]$

// $[LED]
// [LED]$

// $[LESENSE]
// [LESENSE]$

// $[LETIMER0]
// [LETIMER0]$

// $[LEUART0]
// [LEUART0]$

// $[LFXO]
// [LFXO]$

// $[LNA]
// [LNA]$

// $[PA]
#define HAL_PA_ENABLE                                 (1)

#define HAL_PA_RAMP                                   (10UL)
#define HAL_PA_2P4_LOWPOWER                           (0)
#define HAL_PA_POWER                                  (252U)
#define HAL_PA_VOLTAGE                                (3300U)
#define HAL_PA_CURVE_HEADER                            "pa_curves_efr32.h"
// [PA]$

// $[PCNT0]
// [PCNT0]$

// $[PORTIO]
// [PORTIO]$

// $[PRS]
#define PORTIO_PRS_CH4_PIN                            (13U)
#define PORTIO_PRS_CH4_PORT                           (gpioPortD)
#define PORTIO_PRS_CH4_LOC                            (4U)

// [PRS]$

// $[PTI]
#define PORTIO_PTI_DCLK_PIN                           (11U)
#define PORTIO_PTI_DCLK_PORT                          (gpioPortB)
#define PORTIO_PTI_DCLK_LOC                           (6U)

#define PORTIO_PTI_DFRAME_PIN                         (13U)
#define PORTIO_PTI_DFRAME_PORT                        (gpioPortB)
#define PORTIO_PTI_DFRAME_LOC                         (6U)

#define PORTIO_PTI_DOUT_PIN                           (12U)
#define PORTIO_PTI_DOUT_PORT                          (gpioPortB)
#define PORTIO_PTI_DOUT_LOC                           (6U)

#define HAL_PTI_ENABLE                                (0)

#define BSP_PTI_DFRAME_PIN                            (13U)
#define BSP_PTI_DFRAME_PORT                           (gpioPortB)
#define BSP_PTI_DFRAME_LOC                            (6U)

#define BSP_PTI_DOUT_PIN                              (12U)
#define BSP_PTI_DOUT_PORT                             (gpioPortB)
#define BSP_PTI_DOUT_LOC                              (6U)

#define HAL_PTI_MODE                                  (HAL_PTI_MODE_UART)
#define HAL_PTI_BAUD_RATE                             (1600000UL)
// [PTI]$

// $[PYD1698]
// [PYD1698]$

// $[SERIAL]
#define HAL_SERIAL_USART0_ENABLE                      (0) //default 1?
#define BSP_SERIAL_APP_PORT                           (HAL_SERIAL_PORT_USART0)
#define HAL_SERIAL_LEUART0_ENABLE                     (0)
#define HAL_SERIAL_USART1_ENABLE                      (0)
#define HAL_SERIAL_USART2_ENABLE                      (0)
#define HAL_SERIAL_RXWAKE_ENABLE                      (0)
#define BSP_SERIAL_APP_CTS_PIN                        (15U)
#define BSP_SERIAL_APP_CTS_PORT                       (gpioPortD)
#define BSP_SERIAL_APP_CTS_LOC                        (19U)

#define BSP_SERIAL_APP_RX_PIN                         (13U)
#define BSP_SERIAL_APP_RX_PORT                        (gpioPortD)
#define BSP_SERIAL_APP_RX_LOC                         (20U)

#define BSP_SERIAL_APP_TX_PIN                         (14U)
#define BSP_SERIAL_APP_TX_PORT                        (gpioPortD)
#define BSP_SERIAL_APP_TX_LOC                         (22U)

#define BSP_SERIAL_APP_RTS_PIN                        (12U)
#define BSP_SERIAL_APP_RTS_PORT                       (gpioPortD)
#define BSP_SERIAL_APP_RTS_LOC                        (15U)

#define HAL_SERIAL_APP_RX_QUEUE_SIZE                  (128UL)
#define HAL_SERIAL_APP_BAUD_RATE                      (115200UL)
#define HAL_SERIAL_APP_RXSTOP                         (16UL)
#define HAL_SERIAL_APP_RXSTART                        (16UL)
#define HAL_SERIAL_APP_TX_QUEUE_SIZE                  (128UL)
#define HAL_SERIAL_APP_FLOW_CONTROL                   (HAL_USART_FLOW_CONTROL_NONE)
// [SERIAL]$

// $[SPIDISPLAY]
#define HAL_SPIDISPLAY_ENABLE                         (0)

#define BSP_SPIDISPLAY_EXTCOMIN_PIN                   (13U)
#define BSP_SPIDISPLAY_EXTCOMIN_PORT                  (gpioPortD)

#define BSP_SPIDISPLAY_ENABLE_PIN                     (15U)
#define BSP_SPIDISPLAY_ENABLE_PORT                    (gpioPortD)

#define BSP_SPIDISPLAY_CS_PIN                         (14U)
#define BSP_SPIDISPLAY_CS_PORT                        (gpioPortD)

#define HAL_SPIDISPLAY_EXTCOMIN_CALLBACK               rtcIntCallbackRegister
#define BSP_SPIDISPLAY_USART                          (HAL_SPI_PORT_USART1)
#define BSP_SPIDISPLAY_DISPLAY                        (HAL_DISPLAY_SHARP_LS013B7DH03)
#define HAL_SPIDISPLAY_EXTMODE_EXTCOMIN               (1)
#define HAL_SPIDISPLAY_EXTMODE_SPI                    (0)
#define HAL_SPIDISPLAY_EXTCOMIN_USE_PRS               (0)
#define HAL_SPIDISPLAY_EXTCOMIN_USE_CALLBACK          (1)
#define BSP_SPIDISPLAY_EXTCOMIN_CHANNEL               (4)
#define BSP_SPIDISPLAY_CLK_PIN                        (8U)
#define BSP_SPIDISPLAY_CLK_PORT                       (gpioPortC)
#define BSP_SPIDISPLAY_CLK_LOC                        (11U)

#define BSP_SPIDISPLAY_MISO_PIN                       (7U)
#define BSP_SPIDISPLAY_MISO_PORT                      (gpioPortC)
#define BSP_SPIDISPLAY_MISO_LOC                       (11U)

#define BSP_SPIDISPLAY_MOSI_PIN                       (6U)
#define BSP_SPIDISPLAY_MOSI_PORT                      (gpioPortC)
#define BSP_SPIDISPLAY_MOSI_LOC                       (11U)

#define HAL_SPIDISPLAY_FREQUENCY                      (1000000UL)
// [SPIDISPLAY]$

// $[SPINCP]
// [SPINCP]$

// $[TIMER0]
// [TIMER0]$

// $[TIMER1]
// [TIMER1]$

// $[UARTNCP]
// [UARTNCP]$

// $[USART0]
#define PORTIO_USART0_CTS_PIN                         (15U)
#define PORTIO_USART0_CTS_PORT                        (gpioPortD)
#define PORTIO_USART0_CTS_LOC                         (19U)

#define PORTIO_USART0_RTS_PIN                         (12U)
#define PORTIO_USART0_RTS_PORT                        (gpioPortD)
#define PORTIO_USART0_RTS_LOC                         (15U)

#define PORTIO_USART0_RX_PIN                          (13U)
#define PORTIO_USART0_RX_PORT                         (gpioPortD)
#define PORTIO_USART0_RX_LOC                          (20U)

#define PORTIO_USART0_TX_PIN                          (14U)
#define PORTIO_USART0_TX_PORT                         (gpioPortD)
#define PORTIO_USART0_TX_LOC                          (22U)

#define HAL_USART0_ENABLE                             (1)

#define BSP_USART0_CTS_PIN                            (15U)
#define BSP_USART0_CTS_PORT                           (gpioPortD)
#define BSP_USART0_CTS_LOC                            (19U)

#define BSP_USART0_RX_PIN                             (13U)
#define BSP_USART0_RX_PORT                            (gpioPortD)
#define BSP_USART0_RX_LOC                             (20U)

#define BSP_USART0_TX_PIN                             (14U)
#define BSP_USART0_TX_PORT                            (gpioPortD)
#define BSP_USART0_TX_LOC                             (22U)

#define BSP_USART0_RTS_PIN                            (12U)
#define BSP_USART0_RTS_PORT                           (gpioPortD)
#define BSP_USART0_RTS_LOC                            (15U)

#define HAL_USART0_RX_QUEUE_SIZE                      (128UL)
#define HAL_USART0_BAUD_RATE                          (115200UL)
#define HAL_USART0_RXSTOP                             (16UL)
#define HAL_USART0_RXSTART                            (16UL)
#define HAL_USART0_TX_QUEUE_SIZE                      (128UL)
#define HAL_USART0_FLOW_CONTROL                       (HAL_USART_FLOW_CONTROL_NONE)
// [USART0]$

// $[USART1]
// [USART1]$

//Epaper display port using bit-bang SPI
#define  EPD_PORT_MOSI                        (gpioPortA)
#define  EPD_PIN_MOSI                         (0U)
#define  EPD_PORT_DC                          (gpioPortA)
#define  EPD_PIN_DC                           (1U)
#define  EPD_PORT_CLK                         (gpioPortA)
#define  EPD_PIN_CLK                          (2U)
#define  EPD_PORT_CS                          (gpioPortA)
#define  EPD_PIN_CS                           (3U)

#define  EPD_PORT_BUSY                        (gpioPortB)
#define  EPD_PIN_BUSY                         (11U)

#define  EPD_PORT_RESET                       (gpioPortB)
#define  EPD_PIN_RESET                        (12U)

#define EPD_USART                             (USART1)
#define EPD_USART_CLK                         (cmuClock_USART1)

#define EPD_POWER_ON_PORT                     (gpioPortF)
#define EPD_POWER_ON_PIN                      (5U)

// $[USART2]
// [USART2]$

// $[VCOM]
#define HAL_VCOM_ENABLE                               (1)

#define BSP_VCOM_ENABLE_PIN                           (5U)
#define BSP_VCOM_ENABLE_PORT                          (gpioPortA)

// [VCOM]$

// $[VDAC0]
// [VDAC0]$

// $[VUART]
// [VUART]$

// $[WDOG]
#define HAL_WDOG_ENABLE                               (1)

// [WDOG]$

// $[WTIMER0]
// [WTIMER0]$

#endif /* HAL_CONFIG_H */


