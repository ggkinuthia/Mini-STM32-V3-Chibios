/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.org/license.html
 */

/**
 * @file    drivers/ginput/touch/ADS7843/ginput_lld_mouse_board_template.h
 * @brief   GINPUT Touch low level driver source for the ADS7843 on the example board.
 *
 * @defgroup Mouse Mouse
 * @ingroup GINPUT
 * @{
 */

#ifndef _GINPUT_LLD_MOUSE_BOARD_H
#define _GINPUT_LLD_MOUSE_BOARD_H

#define TP_IRQ_PORT GPIOC
#define TP_IRQ_PIN 13

#define TP_CS_PORT GPIOA
#define TP_CS_PIN 4

// Peripherial Clock 42MHz SPI2 SPI3
// Peripherial Clock 84MHz SPI1 SPI1 SPI2/3
#define SPI_BaudRatePrescaler_2 ((uint16_t)0x0000) // 42 MHz 21 MHZ
#define SPI_BaudRatePrescaler_4 ((uint16_t)0x0008) // 21 MHz 10.5 MHz
#define SPI_BaudRatePrescaler_8 ((uint16_t)0x0010) // 10.5 MHz 5.25 MHz
#define SPI_BaudRatePrescaler_16 ((uint16_t)0x0018) // 5.25 MHz 2.626 MHz
#define SPI_BaudRatePrescaler_32 ((uint16_t)0x0020) // 2.626 MHz 1.3125 MHz
#define SPI_BaudRatePrescaler_64 ((uint16_t)0x0028) // 1.3125 MHz 656.25 KHz
#define SPI_BaudRatePrescaler_128 ((uint16_t)0x0030) // 656.25 KHz 328.125 KHz
#define SPI_BaudRatePrescaler_256 ((uint16_t)0x0038) // 328.125 KHz 164.06 KHz


static const SPIConfig spicfg = { NULL, TP_CS_PORT, TP_CS_PIN, SPI_BaudRatePrescaler_32 };

/**
 * @brief   Initialise the board for the touch.
 *
 * @notapi
 */
static inline void init_board(void) {

	palSetPadMode(TP_CS_PORT, 5, PAL_MODE_STM32_ALTERNATE_PUSHPULL);     /* SCK. */
	palSetPadMode(TP_CS_PORT, 6, PAL_MODE_STM32_ALTERNATE_PUSHPULL);     /* MISO.*/
	palSetPadMode(TP_CS_PORT, 7, PAL_MODE_STM32_ALTERNATE_PUSHPULL);     /* MOSI.*/
	palSetPadMode(TP_CS_PORT, TP_CS_PIN, PAL_MODE_OUTPUT_PUSHPULL);
	palSetPad(TP_CS_PORT, TP_CS_PIN);
	spiStart(&SPID1, &spicfg);
}

/**
 * @brief   Check whether the surface is currently touched
 * @return	TRUE if the surface is currently touched
 *
 * @notapi
 */
static inline bool_t getpin_pressed(void) {
	return (!palReadPad(TP_IRQ_PORT, TP_IRQ_PIN));
}

/**
 * @brief   Aquire the bus ready for readings
 *
 * @notapi
 */
static inline void aquire_bus(void) {
	spiAcquireBus(&SPID1);
    //TOUCHSCREEN_SPI_PROLOGUE();
    palClearPad(TP_CS_PORT, TP_CS_PIN);

}

/**
 * @brief   Release the bus after readings
 *
 * @notapi
 */
static inline void release_bus(void) {
	palSetPad(TP_CS_PORT, TP_CS_PIN);
	spiReleaseBus(&SPID1);

}

/**
 * @brief   Read a value from touch controller
 * @return	The value read from the controller
 *
 * params[in] port	The controller port to read.
 *
 * @notapi
 */
static inline uint16_t read_value(uint16_t port) {
	static uint8_t txbuf[3] = {0};
	static uint8_t rxbuf[3] = {0};
	uint16_t ret;

	txbuf[0] = port;

	spiExchange(&SPID1, 3, txbuf, rxbuf);

	ret = (rxbuf[1] << 5) | (rxbuf[2] >> 3);

	return ret;

}

#endif /* _GINPUT_LLD_MOUSE_BOARD_H */
/** @} */

