/* fuji-hid.c - subdriver to monitor Fuji USB/HID devices with NUT
 *
 *  Copyright (C)
 *  2003 - 2012	Arnaud Quette <ArnaudQuette@Eaton.com>
 *  2005 - 2006	Peter Selinger <selinger@users.sourceforge.net>
 *  2008 - 2009	Arjen de Korte <adkorte-guest@alioth.debian.org>
 *  2013 Charles Lepple <clepple+nut@gmail.com>
 *
 *  Note: this subdriver was initially generated as a "stub" by the
 *  gen-usbhid-subdriver script. It must be customized.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "usbhid-ups.h"
#include "fuji-hid.h"
#include "main.h"	/* for getval() */
#include "usb-common.h"

#define FUJI_HID_VERSION	"Fuji Electric HID 0.2"
/* FIXME: experimental flag to be put in upsdrv_info */

/* Fuji Electric Co., Ltd.*/
#define FUJI_VENDORID	0x05e5

/* USB IDs device table */
static char fe_scratch_buf[32];

static usb_device_id_t fuji_usb_device_table[] = {
	/* USB Card (RRACWG13) */
	{ USB_DEVICE(FUJI_VENDORID, 0x0200), NULL },
	/* UX100 Series (PET) */
	{ USB_DEVICE(FUJI_VENDORID, 0x5002), NULL },

	/* Terminating entry */
	{ -1, -1, NULL }
};

static info_lkp_t fe_upstype_conversion[] = {
	{ 1, "offline", NULL },
	{ 2, "online", NULL },
	{ 0, NULL, NULL }
};

static info_lkp_t fe_volt_hi[] = {
	{ 1, "voltage too high!", NULL },
	{ 0, NULL, NULL }
};

static info_lkp_t fe_volt_lo[] = {
	{ 1, "voltage too low!", NULL },
	{ 0, NULL, NULL }
};

static info_lkp_t fe_short_circuit[] = {
	{ 1, "Output short circuit!", NULL },
	{ 0, NULL, NULL }
};

info_lkp_t fe_infail_info[] = {
	{ 1, "internal failure!", NULL },
	{ 0, NULL, NULL }
};

info_lkp_t fe_cfgfail_info[] = {
	{ 1, "EEPROM fault!", NULL },
	{ 0, NULL, NULL }
};

info_lkp_t fe_vrange_info[] = {
	{ 0, "vrange", NULL },
	{ 1, "!vrange", NULL },
	{ 0, NULL, NULL }
};

static const char *fe_test_period_fun(double value)
{
	int	i = value;

	snprintf(fe_scratch_buf, sizeof(fe_scratch_buf), "%d", i);
	upsdebugx(3, "%s: value = %.0f, buf = %s", __func__, value, fe_scratch_buf);

	return fe_scratch_buf;
}

static double fe_test_period_nuf(const char *value)
{
	const char	*s = dstate_getinfo("UPS.BatterySystem.Battery.TestPeriod");
	uint32_t	sec;
    uint16_t    day;

	sec = atoi(value ? value : s);

    day = ((((sec / 24) / 60) / 60) * 10);

    if(140 >= day)
    {
        day = 14;
    }
    else if(1120 <= day)
    {
        day = 112;
    }
    else
    {
        day += ((day%10) ? 10 : 0);
        day = (day / 10);
    }

    sec = (((day * 24) * 60) * 60);

	upsdebugx(3, "%s: value = %s, command = %08X(%d)", __func__, value, sec, day);

	return sec;
}

static info_lkp_t fe_test_period[] = {
	{ 0, NULL, fe_test_period_fun, fe_test_period_nuf }
};

static const char *fe_voltage_hi_fun(double value)
{
	int	i = value;

	snprintf(fe_scratch_buf, sizeof(fe_scratch_buf), "%d", i);
	upsdebugx(3, "%s: value = %.0f, buf = %s", __func__, value, fe_scratch_buf);

	return fe_scratch_buf;
}

static double fe_voltage_hi_nuf(const char *value)
{
	const char	*s = dstate_getinfo("UPS.Flow.[1].ConfigVoltage");
	uint8_t	base, lo, hi, val;

	base = atoi(s);
	val  = atoi(value);

    lo = (base + 10);
    hi = (base + 17);

    if(val < lo)
    {
        val = lo;   // Rated + 10V
    }
    else if(val > hi)
    {
        val = hi;   // Rated + 17V
    }

	upsdebugx(3, "%s: value = %s, command = %04X", __func__, value, val);

	return val;
}

static info_lkp_t fe_voltage_hi[] = {
	{ 0, NULL, fe_voltage_hi_fun, fe_voltage_hi_nuf }
};

static const char *fe_voltage_lo_fun(double value)
{
	int	i = value;

	snprintf(fe_scratch_buf, sizeof(fe_scratch_buf), "%d", i);
	upsdebugx(3, "%s: value = %.0f, buf = %s", __func__, value, fe_scratch_buf);

	return fe_scratch_buf;
}

static double fe_voltage_lo_nuf(const char *value)
{
	const char	*s = dstate_getinfo("UPS.Flow.[1].ConfigVoltage");
	uint8_t	base, lo, hi, val;

	base = atoi(s);
	val  = atoi(value);

    lo = (base - 20);
    hi = (base - 10);

    if(val < lo)
    {
        val = lo;   // Rated - 20V
    }
    else if(val > hi)
    {
        val = hi;   // Rated - 10V
    }

	upsdebugx(3, "%s: value = %s, command = %04X", __func__, value, val);

	return val;
}

static info_lkp_t fe_voltage_lo[] = {
	{ 0, NULL, fe_voltage_lo_fun, fe_voltage_lo_nuf }
};


/* nominal voltage */
static const char *nominal_voltage_fun(double value)
{
	switch ((long)value)
	{
		case 100:
		case 110:
		case 120:
			break;

		default:
			return NULL;
	}

	snprintf(fe_scratch_buf, sizeof(fe_scratch_buf), "%.0f", value);

	return fe_scratch_buf;
}

static info_lkp_t nominal_voltage_info[] = {
	/* rated voltage models */
	{ 100, "100", nominal_voltage_fun },
	{ 110, "110", nominal_voltage_fun },
	{ 120, "120", nominal_voltage_fun },
	{ 0, NULL, NULL }
};

/* --------------------------------------------------------------- */
/*      Vendor-specific usage table */
/* --------------------------------------------------------------- */

/* Fuji Electric usage table */
static usage_lkp_t fuji_usage_lkp[] = {
	{ "Vendor",					0xffff0010 },
	{ "Inverter",				0xffff0014 },
	{ "InverterID",				0xffff0015 },
	{ "ConfigurationFailure",	0xffff002d },
	{ "ConverterType",			0xffff0041 },
	{ "TestPeriod",				0xffff0045 },
	{ "StartOnBattery",			0xffff0047 },
	{ "ShortCircuit",			0xffff004a },
	{ "VoltageTooHigh",			0xffff0074 },
	{ "VoltageTooLow",			0xffff0075 },
	{ "FanFailure",				0xffff0077 },
	{ "Floating",				0xffff0079 },
	{ "Threshold",				0xffff007d },
	{ "OverThreshold",			0xffff007e },
	{ "Country",				0xffff0095 },
	{ "iModel",					0xffff00f0 },
	{ "iVersion",				0xffff00f1 },
	{ "iUSBVersion",			0xffff00f2 },
	{ "iPartNumber",			0xffff00f3 },
	{ "iReferenceNumber",		0xffff00f4 },
	{ "Unknown",				0xffff00ff },
	{  NULL, 0 }
};

static usage_tables_t fuji_utab[] = {
	fuji_usage_lkp,
	hid_usage_lkp,
	NULL,
};

/* --------------------------------------------------------------- */
/* HID2NUT lookup table                                            */
/* --------------------------------------------------------------- */

static hid_info_t fuji_hid2nut[] = {

  /* Battery page */
  { "battery.type", 0, 0, "UPS.PowerSummary.iDeviceChemistry", NULL, "%s", HU_FLAG_STATIC, stringid_conversion },
  { "battery.voltage", 0, 0, "UPS.PowerSummary.Voltage", NULL, "%.1f", 0, NULL },
  { "battery.capacity", 0, 0, "UPS.PowerSummary.DesignCapacity", NULL, "%.0f", HU_FLAG_STATIC, NULL },
  { "battery.runtime", 0, 0, "UPS.PowerSummary.RunTimeToEmpty", NULL, "%.0f", HU_FLAG_QUICK_POLL, NULL },
  { "battery.charge", 0, 0, "UPS.PowerSummary.RemainingCapacity", NULL, "%.0f", HU_FLAG_QUICK_POLL, NULL },
  { "battery.floating", 0, 0, "UPS.BatterySystem.Charger.PresentStatus.Floating", NULL, "%.0f", 0, on_off_info },

  /* UPS page */
  { "ups.type", 0, 0, "UPS.PowerConverter.ConverterType", NULL, "%.0f", HU_FLAG_STATIC, fe_upstype_conversion },
  { "ups.id", 0, 0, "UPS.PowerSummary.iProduct", NULL, "%s", HU_FLAG_STATIC, stringid_conversion },
  { "ups.firmware", 0, 0, "UPS.PowerSummary.iVersion", NULL, "%s", HU_FLAG_STATIC, stringid_conversion },
  { "ups.usb.firmware", 0, 0, "UPS.PowerSummary.iUSBVersion", NULL, "%s", HU_FLAG_STATIC, stringid_conversion },
  { "ups.power.nominal", 0, 0, "UPS.Flow.[4].ConfigApparentPower", NULL, "%.0f", HU_FLAG_STATIC, NULL },
  { "ups.power", 0, 0, "UPS.PowerConverter.Output.ApparentPower", NULL, "%.0f", 0, NULL },
  { "ups.realpower.nominal", 0, 0, "UPS.Flow.[4].ConfigActivePower", NULL, "%.0f", HU_FLAG_STATIC, NULL },
  { "ups.realpower", 0, 0, "UPS.PowerConverter.Output.ActivePower", NULL, "%.0f", 0, NULL },
  { "ups.load", 0, 0, "UPS.PowerSummary.PercentLoad", NULL, "%.0f", 0, NULL },
  { "ups.load.high", 0, 0, "UPS.PowerConverter.Output.Overload.[1].Threshold", NULL, "%.0f", HU_FLAG_STATIC, NULL },
  { "ups.test.result", 0, 0, "UPS.BatterySystem.Battery.Test", NULL, "%s", 0, test_read_info },
  { "ups.test.interval", 0, 0, "UPS.BatterySystem.Battery.TestPeriod", NULL, "%.0f", HU_FLAG_SEMI_STATIC, fe_test_period },
  { "ups.beeper.status", 0 ,0, "UPS.PowerSummary.AudibleAlarmControl", NULL, "%.0f", 0, beeper_info },
//{ "ups.start.battery", 0, 0, "UPS.PowerConverter.Input[3].StartOnBattery", NULL, 0, 0,  yes_no_info },
  { "ups.inverter", 0, 0, "UPS.PowerConverter.Inverter.PresentStatus.Used", NULL, "%.0f", 0, on_off_info },
  { "ups.converter", 0, 0, "UPS.PowerConverter.Input.[3].PresentStatus.Used", NULL, "%.0f", 0, on_off_info },
  { "ups.delay.start",    ST_FLAG_RW | ST_FLAG_STRING, 10, "UPS.PowerSummary.DelayBeforeStartup",  NULL, "180", HU_FLAG_ABSENT, NULL },
  { "ups.delay.shutdown", ST_FLAG_RW | ST_FLAG_STRING, 10, "UPS.PowerSummary.DelayBeforeShutdown", NULL, "120", HU_FLAG_ABSENT, NULL },
  { "ups.timer.start",    0, 0, "UPS.PowerSummary.DelayBeforeStartup",  NULL, "%.0f", 0, NULL },
  { "ups.timer.shutdown", 0, 0, "UPS.PowerSummary.DelayBeforeShutdown", NULL, "%.0f", 0, NULL },
  { "ups.timer.reboot",   0, 0, "UPS.PowerSummary.DelayBeforeReboot",   NULL, "%.0f", 0, NULL },

  /* Input page */
  { "input.voltage", 0, 0, "UPS.PowerConverter.Input.[1].Voltage", NULL, "%.1f", 0, NULL },
  { "input.voltage.nominal",ST_FLAG_RW | ST_FLAG_STRING, 3, "UPS.Flow.[1].ConfigVoltage", NULL, "%.0f", HU_FLAG_SEMI_STATIC | HU_FLAG_ENUM, nominal_voltage_info },
  { "input.frequency", 0, 0, "UPS.PowerConverter.Input.[1].Frequency", NULL, "%.1f", 0, NULL },
  { "input.frequency.nominal", 0, 0, "UPS.Flow.[1].ConfigFrequency", NULL, "%.0f", 0, NULL },
  { "input.voltagetoohigh", 0, 0, "UPS.PowerConverter.Input.[1].PresentStatus.VoltageTooHigh", NULL, "%.0f", 0, fe_vrange_info },
  { "input.voltagetoolow", 0, 0, "UPS.PowerConverter.Input.[1].PresentStatus.VoltageTooLow", NULL, "%.0f", 0, fe_vrange_info },
  { "input.transfer.low", 0, 0, "UPS.PowerConverter.Output.LowVoltageTransfer", NULL, "%.0f", HU_FLAG_SEMI_STATIC, fe_voltage_lo },
  { "input.transfer.high", 0, 0, "UPS.PowerConverter.Output.HighVoltageTransfer", NULL, "%.0f", HU_FLAG_SEMI_STATIC, fe_voltage_hi },

  /* Output page */
  { "output.voltage", 0, 0, "UPS.PowerConverter.Output.Voltage", NULL, "%.1f", 0, NULL },
  { "output.voltage.nominal", 0, 0, "UPS.Flow.[4].ConfigVoltage", NULL, "%.0f", 0, NULL },
  { "output.current", 0, 0, "UPS.PowerConverter.Output.Current", NULL, "%.1f", 0, NULL },
  { "output.frequency", 0, 0, "UPS.PowerConverter.Output.Frequency", NULL, "%.1f", 0, NULL },
  { "output.frequency.nominal", 0, 0, "UPS.Flow.[4].ConfigFrequency", NULL, "%.0f", 0, NULL },

  /* Special case: ups.status */
  { "BOOL", 0, 0, "UPS.PowerSummary.PresentStatus.ACPresent", NULL, NULL, HU_FLAG_QUICK_POLL, online_info },
  { "BOOL", 0, 0, "UPS.PowerSummary.PresentStatus.BelowRemainingCapacityLimit", NULL, NULL, HU_FLAG_QUICK_POLL, lowbatt_info },
  { "BOOL", 0, 0, "UPS.PowerSummary.PresentStatus.Charging", NULL, NULL, HU_FLAG_QUICK_POLL, charging_info },
  { "BOOL", 0, 0, "UPS.PowerSummary.PresentStatus.Discharging", NULL, NULL, HU_FLAG_QUICK_POLL, discharging_info },
  { "BOOL", 0, 0, "UPS.PowerSummary.PresentStatus.Good", NULL, NULL, HU_FLAG_QUICK_POLL, off_info },

  /* Special case: ups.alarm */
  { "BOOL", 0, 0, "UPS.PowerConverter.Output.Overload.[1].PresentStatus.OverThreshold", NULL, NULL, 0, overload_info },
  { "BOOL", 0, 0, "UPS.PowerConverter.Output.Overload.[2].PresentStatus.OverThreshold", NULL, NULL, 0, overload_info },
  { "BOOL", 0, 0, "UPS.PowerSummary.PresentStatus.Overload", NULL, NULL, 0, overload_info },
  { "BOOL", 0, 0, "UPS.PowerSummary.PresentStatus.NeedReplacement", NULL, NULL, 0, replacebatt_info },
  { "BOOL", 0, 0, "UPS.PowerConverter.Input.[1].PresentStatus.VoltageOutOfRange", NULL, NULL, 0, vrange_info },
  { "BOOL", 0, 0, "UPS.PowerConverter.Input.[1].PresentStatus.FrequencyOutOfRange", NULL, NULL, 0, frange_info },
  { "BOOL", 0, 0, "UPS.PowerSummary.PresentStatus.OverTemperature", NULL, NULL, 0, overheat_info },
  { "BOOL", 0, 0, "UPS.PowerSummary.PresentStatus.FanFailure", NULL, NULL, 0, fanfail_info },
  { "BOOL", 0, 0, "UPS.BatterySystem.Charger.PresentStatus.InternalFailure", NULL, NULL, 0, chargerfail_info },

  /* Special case: ups.alarm */
  { "ups.alarm", 0, 0, "UPS.PowerConverter.Inverter.PresentStatus.VoltageTooHigh", NULL, "%.0f", 0, fe_volt_hi },
  { "ups.alarm", 0, 0, "UPS.PowerConverter.Inverter.PresentStatus.VoltageTooLow", NULL, "%.0f", 0, fe_volt_lo },
  { "ups.alarm", 0, 0, "UPS.PowerSummary.PresentStatus.ConfigurationFailure", NULL, "%.0f", 0, fe_cfgfail_info },
  { "ups.alarm", 0, 0, "UPS.PowerSummary.PresentStatus.InternalFailure", NULL, "%.0f", 0, fe_infail_info },
  { "ups.alarm", 0, 0, "UPS.PowerConverter.Output.PresentStatus.ShortCircuit", NULL, "%.0f", 0, fe_short_circuit },

  /* Delay Shutdown & Startup */
  { "load.off.delay", 0, 0, "UPS.PowerSummary.DelayBeforeShutdown", NULL, "60", HU_TYPE_CMD, NULL },
  { "load.on.delay",  0, 0, "UPS.PowerSummary.DelayBeforeStartup",  NULL, "60", HU_TYPE_CMD, NULL },
  { "shutdown.stop",    0, 0, "UPS.PowerSummary.DelayBeforeShutdown", NULL, "-1",  HU_TYPE_CMD, NULL },
  { "shutdown.stayoff", 0, 0, "UPS.PowerSummary.DelayBeforeShutdown", NULL, "60", HU_TYPE_CMD, NULL },
//{ "shutdown.return",  0, 0, "UPS.PowerSummary.DelayBeforeReboot", NULL, "10", HU_TYPE_CMD, NULL },
  { "shutdown.reboot",  0, 0, "UPS.PowerSummary.DelayBeforeReboot", NULL, "10", HU_TYPE_CMD, NULL },

  /* Beep */
  { "beeper.mute", 0, 0, "UPS.PowerSummary.AudibleAlarmControl", NULL, "3", HU_TYPE_CMD, NULL },
  { "beeper.disable", 0, 0, "UPS.PowerSummary.AudibleAlarmControl", NULL, "1", HU_TYPE_CMD, NULL },
  { "beeper.enable", 0, 0, "UPS.PowerSummary.AudibleAlarmControl", NULL, "2", HU_TYPE_CMD, NULL },

  /* Battery Test */
  { "test.battery.start.quick", 0, 0, "UPS.BatterySystem.Battery.Test", NULL, "1", HU_TYPE_CMD, NULL },
  { "test.battery.start.deep", 0, 0, "UPS.BatterySystem.Battery.Test", NULL, "2", HU_TYPE_CMD, NULL },

  /* end of structure. */
  { NULL, 0, 0, NULL, NULL, NULL, 0, NULL }
};

static const char *fuji_format_model(HIDDevice_t *hd) {

	if((strncmp("UX100", hd->Product, 5)))
	{
	    dstate_setinfo("ups.id", "USB CARD");
		return hd->Product;
	}

	/* try UPS.PowerSummary.iModel */
	HIDGetItemString(udev, "UPS.PowerSummary.iModel", fe_scratch_buf, sizeof(fe_scratch_buf), fuji_utab);

	if (strlen(fe_scratch_buf) < 1) {
		return hd->Product;
	}

	/* free(hd->Serial); not needed, we already know it is NULL */
	hd->Product = strdup(fe_scratch_buf);

	return hd->Product;
}

static const char *fuji_format_mfr(HIDDevice_t *hd) {
	return hd->Vendor ? hd->Vendor : "Fuji Electric";
}

static const char *fuji_format_serial(HIDDevice_t *hd) {
	return hd->Serial;
}

/* this function allows the subdriver to "claim" a device: return 1 if
 * the device is supported by this subdriver, else 0. */
static int fuji_claim(HIDDevice_t *hd)
{
	int status = is_usb_device_supported(fuji_usb_device_table, hd);

	switch (status)
	{
	case SUPPORTED:
		return 1;

	case NOT_SUPPORTED:
	case POSSIBLY_SUPPORTED:
	default:
		return 0;
	}
}

subdriver_t fuji_subdriver = {
	FUJI_HID_VERSION,
	fuji_claim,
	fuji_utab,
	fuji_hid2nut,
	fuji_format_model,
	fuji_format_mfr,
	fuji_format_serial,
};
