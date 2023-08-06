/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2023 Nikita Burnashev <nikita.burnashev@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <string.h>

#include "flash.h"
#include "chipdrivers.h"
#include "spi.h"

int print_nrf24_fsr(struct flashctx *flash)
{
	uint8_t status;
	int ret = spi_read_register(flash, STATUS1, &status);
	if (ret)
		return ret;

	msg_cdbg("Chip status register: Enable HW debugger (DBG) is %sset\n",
		 (status & (1 << 7)) ? "" : "not ");
	msg_cdbg("Chip status register: Start from protected program memory (STP) is %sset\n",
		 (status & (1 << 6)) ? "" : "not ");
	msg_cdbg("Chip status register: Flash write (or erase) enable (WEN) is %sset\n",
		 (status & (1 << 5)) ? "" : "not ");
	msg_cdbg("Chip status register: Flash interface not ready (RDYN) is %sset\n",
		 (status & (1 << 4)) ? "" : "not ");
	msg_cdbg("Chip status register: InfoPage enable (INFEN) is %sset\n",
		 (status & (1 << 3)) ? "" : "not ");
	msg_cdbg("Chip status register: SPI read-back disable of MainBlock (RDISMB) is %sset\n",
		 (status & (1 << 2)) ? "" : "not ");
	msg_cdbg("Chip status register: SPI read-back disable of InfoPage (RDISIP) is %sset\n",
		 (status & (1 << 1)) ? "" : "not ");
	return 0;
}

/* There's no official way to detect an NRF24 MCU via SPI!
 * We must resort to a hack: switch to InfoPage and back
 * and compare 5 bytes CHIPID (addresses 0x0B to 0x0F). 
 * FIXME other flash devices use bit 3 (INFEN) for block protection */
int probe_nrf24(struct flashctx *flash)
{
	uint8_t status, id[5], pgm[5];

	if (spi_read_register(flash, STATUS1, &status))
		return 0;

	if (spi_write_register(flash, STATUS1, status | SPI_SR_INFEN))
		return 0;
	if (spi_nbyte_read(flash, 0x0B, id, sizeof id))
		return 0;

	if (spi_write_register(flash, STATUS1, status & ~SPI_SR_INFEN))
		return 0;
	if (spi_nbyte_read(flash, 0x0B, pgm, sizeof pgm))
		return 0;

	/* CHIPID is unlikely to collide with program memory */
	if (memcmp(id, pgm, sizeof id) == 0)
		return 0;

	return 1;
}
