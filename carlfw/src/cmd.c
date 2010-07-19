/*
 * carl9170 firmware - used by the ar9170 wireless device
 *
 * Code to handle commands from the host driver.
 *
 * Copyright (c) 2000-2005 ZyDAS Technology Corporation
 * Copyright (c) 2007-2009 Atheros Communications, Inc.
 * Copyright	2009	Johannes Berg <johannes@sipsolutions.net>
 * Copyright 2009, 2010 Christian Lamparter <chunkeey@googlemail.com>
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
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "carl9170.h"
#include "io.h"
#include "cam.h"
#include "rf.h"
#include "printf.h"
#include "timer.h"
#include "wl.h"

void handle_cmd(struct carl9170_rsp *resp)
{
	struct carl9170_cmd *cmd = &dma_mem.reserved.cmd.cmd;
	unsigned int i;

	/* copies cmd, len and extra fields */
	resp->hdr.hdr_data = cmd->hdr.hdr_data;

	switch (cmd->hdr.cmd) {
	case CARL9170_CMD_RREG:
		for (i = 0; i < (cmd->hdr.len / 4); i++)
			resp->rreg_res.vals[i] = get(cmd->rreg.regs[i]);
		break;

	case CARL9170_CMD_WREG:
		resp->hdr.len = 0;
		for (i = 0; i < (cmd->hdr.len / 8); i++)
			set(cmd->wreg.regs[i].addr, cmd->wreg.regs[i].val);
		break;

	case CARL9170_CMD_ECHO:
		memcpy(resp->echo.vals, cmd->echo.vals, cmd->hdr.len);
		break;

	case CARL9170_CMD_SWRST:
		resp->hdr.len = 0;
		fw.wlan.mac_reset = CARL9170_MAC_RESET_FORCE;
		break;

	case CARL9170_CMD_REBOOT:
		/*
		 * reboot does not return and generates no response
		 * resp->len = 0;
		 */

		reboot();
		break;

	case CARL9170_CMD_READ_TSF:
		resp->hdr.len = 8;
		read_tsf((uint32_t *)resp->tsf.tsf);
		break;

#ifdef CONFIG_CARL9170FW_CAB_QUEUE
	case CARL9170_CMD_FLUSH_CAB:
		resp->hdr.len = 0;
		fw.wlan.cab_flush_trigger = CARL9170_CAB_TRIGGER_ARMED;
		fw.wlan.cab_flush_time = get_clock_counter() +
					 CARL9170_TBTT_DELTA;
		break;
#endif /* CONFIG_CARL9170FW_CAB_QUEUE */

#ifdef CONFIG_CARL9170FW_SECURITY_ENGINE
	case CARL9170_CMD_EKEY:
		resp->hdr.len = 1;
		set_key(&cmd->setkey);
		break;

	case CARL9170_CMD_DKEY:
		/* Disable Key */
		resp->hdr.len = 1;
		disable_key(&cmd->disablekey);
		break;
#endif /* CONFIG_CARL9170FW_SECURIT_ENGINE */

#ifdef CONFIG_CARL9170FW_RADIO_FUNCTIONS
	case CARL9170_CMD_FREQUENCY:
	case CARL9170_CMD_RF_INIT:
		rf_cmd(cmd, resp);
		break;

	case CARL9170_CMD_FREQ_START:
		resp->hdr.len = 0;
		rf_notify_set_channel();
		break;

# ifdef CONFIG_CARL9170FW_PSM
	case CARL9170_CMD_PSM:
		resp->hdr.len = 0;
		fw.phy.psm.state = le32_to_cpu(cmd->psm.state);
		rf_psm();
		break;
# endif /* CONFIG_CARL9170FW_PSM */
#endif /* CONFIG_CARL9170FW_RADIO_FUNCTIOS */

#ifdef CONFIG_CARL9170FW_USB_WATCHDOG
	case CARL9170_CMD_USB_WD:
		resp->hdr.len = 4;
		fw.usb.watchdog.state = le32_to_cpu(cmd->watchdog.state);
		break;

#endif /* CONFIG_CARL9170FW_USB_WATCHDOG */

	default:
		break;
	}
}