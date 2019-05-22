/*
 * omron.h: driver for OMRON UPS hardware
 *
 * Copyright (C) 2014 OMRON <omron_support@omron.co.jp>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef OMRON_H
#define OMRON_H

#define MAXTRIES	3

/*
 * The driver core will do all the protocol handling and provides
 * the following (interface independent) parts already
 *
 *	upsdrv_updateinfo()
 *	upsdrv_shutdown()
 *
 * Communication with the UPS is done through omron_command() of which
 * the prototype is declared below. It shall send a command and reads
 * a reply if buf is not a NULL pointer and buflen > 0.
 *
 * Returns < 0 on error, 0 on timeout and the number of bytes send/read on
 * success.
 */
int omron_command(const char *cmd, char *buf, size_t buflen);

void omron_makevartable(void);
void omron_initups(void);
void omron_initinfo(void);

/* NUT Variable */
#define UPS_BEEPER_STATUS "ups.beeper.status"
#define UPS_DATE "ups.date"
#define UPS_DELAY_START "ups.delay.start"
#define UPS_DELAY_REBOOT "ups.delay.reboot"
#define UPS_DELAY_SHUTDOWN "ups.delay.shutdown"
#define UPS_FIRMWARE "ups.firmware"
#define UPS_FIRMWARE_AUX "ups.firmware.aux"
#define UPS_LOAD "ups.load"
#define UPS_MFR "ups.mfr"
#define UPS_MODEL "ups.model"
#define UPS_POWER "ups.power"
#define UPS_PRODUCTID "ups.productid"
#define UPS_REALPOWER "ups.realpower"
#define UPS_SERIAL "ups.serial"
#define UPS_START_AUTO "ups.start.auto"
#define UPS_START_BATTERY "ups.start.battery"
#define UPS_TEMPERATURE "ups.temperature"
#define UPS_TIME "ups.time"
#define UPS_TYPE "ups.type"
#define UPS_VENDORID "ups.vendorid"

#define INPUT_VOLTAGE "input.voltage"
#define INPUT_VOLTAGE_FAULT "input.voltage.fault"
#define INPUT_FREQUENCY "input.frequency"
#define INPUT_SENSITIVITY "input.sensitivity"

#define OUTPUT_FREQUENCY "output.frequency"
#define OUTPUT_VOLTAGE "output.voltage"
#define OUTPUT_VOLTAGE_NOMINAL "output.voltage.nominal"

#define BATTERY_CHARGE "battery.charge"
#define BATTERY_CHARGE_LOW "battery.charge.low"
#define BATTERY_CHARGE_RESTART "battery.charge.restart"
#define BATTERY_DATE "battery.date"
#define BATTERY_PACKS "battery.packs"
#define BATTERY_RUNTIME "battery.runtime"
#define BATTERY_TEMPERATURE "battery.temperature"
#define BATTERY_VOLTAGE "battery.voltage"
#define BATTERY_VOLTAGE_NOMINAL "battery.voltage.nominal"

#define OUTLET_1_STATUS "outlet.1.status"
#define OUTLET_1_SWITCHABLE "outlet.1.switchable"
#define OUTLET_2_STATUS "outlet.2.status"
#define OUTLET_2_SWITCHABLE "outlet.2.switchable"
#define OUTLET_3_STATUS "outlet.3.status"
#define OUTLET_3_SWITCHABLE "outlet.3.switchable"

/* NUT Instant Command */
#define BEEPER_ENABLE "beeper.enable"
#define BEEPER_DISABLE "beeper.disable"
#define SHUTDOWN_RETURN "shutdown.return"
#define SHUTDOWN_STAYOFF "shutdown.stayoff"
#define SHUTDOWN_STOP "shutdown.stop"
#define SHUTDOWN_REBOOT "shutdown.reboot"
#define SHUTDOWN_REBOOT_GRACEFUL "shutdown.reboot.graceful"
#define OUTLET_1_LOAD_ON "outlet.1.load.on"
#define OUTLET_1_LOAD_OFF "outlet.1.load.off"
#define OUTLET_2_LOAD_ON "outlet.2.load.on"
#define OUTLET_2_LOAD_OFF "outlet.2.load.off"
#define OUTLET_3_LOAD_ON "outlet.3.load.on"
#define OUTLET_3_LOAD_OFF "outlet.3.load.off"


#endif /* OMRON_H */
