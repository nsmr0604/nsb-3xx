/*
 * omron.c: driver for OMRON UPS hardware
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

#include "main.h"
#include "omron.h"

#define CMP_NUM    "0123456789"
#define CMP_HEX    CMP_NUM "ABCDEF"

#define true       (1)
#define false      (0)

#define POLLING_TIME (2.0)

#define STR_OK "OK"
#define STR_NAK "NAK"
#define STR_YES "yes"
#define STR_NO "no"
#define STR_ON "on"
#define STR_OFF "off"
#define STR_ENABLE "enable"
#define STR_DISABLE "disable"
#define STR_HIGH_PRECISION "H"
#define STR_NORMAL_PRECISION "N"
#define STR_LOW_PRECISION "L"
#define STR_SETVAR_INIT "-"
#define PARAMETER_SHORTAGE "parameter shortage"

#define UPS_TYPE_OFFLINE "offline / line interactive"
#define UPS_TYPE_ONLINE "online"


#define UPS_TIME_LEN (5)
#define UPS_DATE_LEN (8)
#define BATTERY_DATE_LEN (8)

static int	ondelay = 3;	/* minutes */
static int	offdelay = 30;	/* seconds */

static int	online = 1;

static char     omron_work_buf[128];

/* this enumeration is cmd_info array's index */
typedef enum {
	CMD_Af,
	CMD_An,
	CMD_Bl_q,
	CMD_Bf,
	CMD_Bn,
	CMD_BRR,
	CMD_BRW_yyyymmdd,
	CMD_Bv,
	CMD_C,
	CMD_CSf,
	CMD_CSn,
	CMD_FWV,
	CMD_Lf_q,
	CMD_Oaf,
	CMD_Oan,
	CMD_Obf,
	CMD_Obn,
	CMD_Ocf,
	CMD_Ocn,
	CMD_OS_q,
	CMD_OUT,
	CMD_PSNR,
	CMD_Q1,
	CMD_Rbd_q,
	CMD_RBV,
	CMD_Rbv_q,
	CMD_S_n,
	CMD_S_nR_m,
	CMD_Sf_n,
	CMD_SS,
	CMD_Ss_q,
	CMD_TPb,
	CMD_UN,
	CMD_V,
	CMD_V2,
	CMD_V3,
	CMD_VA,
	CMD_VB,
	CMD_WR,
	CMD_RRTC,
	CMD_BLD,
	CMD_Bld_q,
	CMD_RTS,
	MAX_CMD_NUM
} omron_cmd_id;

static struct {
	enum {
		INACTIVE_CMD = 0,/* default */
		POLLING_CMD = 1,
		WRITE_CMD   = 2,
		INSTANT_CMD = 3,
		INITIAL_CMD = 4,
	} kind;
	int is_supported;
	int error_count;
} cmd_info [MAX_CMD_NUM];

static struct {
	int    prod_id;
	double loadC1;			// C1 load%
	double consumption;		// internal load%
	double runtimeFull;		// runtime by max load(min)
} runtime_table[] = {
//	{ 0x0043, 00.0, 0.0000, 0.0 },	/* OMRON BN240XR */
//	{ 0x0044, 00.0, 0.0000, 0.0 },	/* OMRON BN150XR */
	{ 0x004B, 21.0, 0.0040, 3.0 },	/* OMRON BZ50T */
	{ 0x004F, 21.0, 0.0044, 3.0 },	/* OMRON BZ50LT */
	{ 0x0050, 21.0, 0.0042, 3.0 },	/* OMRON BZ35T */
//	{ 0x0054, 00.0, 0.0000, 0.0 },	/* OMRON BN100XR */
	{ 0x0057, 28.0, 0.0142, 5.0 },	/* OMRON BX50F */
	{ 0x0058, 33.0, 0.0128, 3.5 },	/* OMRON BX35F */
	{ 0x0065, 26.0, 0.0056, 3.5 },	/* OMRON BY50FW */
	{ 0x0066, 33.0, 0.0028, 5.0 },	/* OMRON BY75SW */
//	{ 0x006B, 00.0, 0.0000, 0.0 },	/* OMRON BU1002SW */
//	{ 0x006C, 00.0, 0.0000, 0.0 },	/* OMRON BU3002SW */
//	{ 0x006E, 00.0, 0.0000, 0.0 },	/* OMRON BN50S */
//	{ 0x006F, 00.0, 0.0000, 0.0 },	/* OMRON BN75S */
//	{ 0x0070, 00.0, 0.0000, 0.0 },	/* OMRON BN100S */
//	{ 0x0071, 00.0, 0.0000, 0.0 },	/* OMRON BN150S */
//	{ 0x0072, 00.0, 0.0000, 0.0 },	/* OMRON BN220S */
//	{ 0x0073, 00.0, 0.0000, 0.0 },	/* OMRON BN300S */
//	{ 0x0075, 00.0, 0.0000, 0.0 },	/* OMRON BU100RW */
//	{ 0x0076, 00.0, 0.0000, 0.0 },	/* OMRON BU200RW */
//	{ 0x0077, 00.0, 0.0000, 0.0 },	/* OMRON BU300RW */
	{ 0x0080, 29.0, 0.0044, 6.0 },	/* OMRON BY35S */
	{ 0x0081, 20.0, 0.0044, 3.5 },	/* OMRON BY50S */
//	{ 0x0083, 00.0, 0.0000, 0.0 },	/* OMRON BU75RW */
	{ 0x0086, 21.0, 0.0088, 3.0 },	/* OMRON BZ35LT2 */
	{ 0x0087, 23.0, 0.0066, 3.0 },	/* OMRON BZ50LT2 */
	{ 0x00A1, 25.0, 0.0050, 4.0 },	/* OMRON BY80S */
	{ 0x00A3, 25.0, 0.0044, 4.0 },	/* OMRON BY120S */
//	{ 0x00AD, 00.0, 0.0000, 0.0 },	/* OMRON BN75R */
//	{ 0x00AE, 00.0, 0.0000, 0.0 },	/* OMRON BN150R */
//	{ 0x00AF, 00.0, 0.0000, 0.0 },	/* OMRON BN300R */
//	{ 0x00B4, 00.0, 0.0000, 0.0 },	/* OMRON BN50T */
//	{ 0x00B5, 00.0, 0.0000, 0.0 },	/* OMRON BN75T */
//	{ 0x00B6, 00.0, 0.0000, 0.0 },	/* OMRON BN100T */
//	{ 0x00B7, 00.0, 0.0000, 0.0 },	/* OMRON BN150T */
//	{ 0x00B8, 00.0, 0.0000, 0.0 },	/* OMRON BN220T */
//	{ 0x00B9, 00.0, 0.0000, 0.0 },	/* OMRON BN300T */
	/* end of list */
	{-1, 00, 0.000, 0.0}
};

static double runtime_vol = 98178.0;
static double runtime_exp = -1.334897356;
static double runtime_cons = 0.0044;

/*
 * this function is settings property of all commands
 */
static int omron_initvars() {
	memset(cmd_info, 0, sizeof(cmd_info));

	cmd_info[CMD_Af].kind = WRITE_CMD;
	cmd_info[CMD_An].kind = WRITE_CMD;
	cmd_info[CMD_Bl_q].kind = POLLING_CMD;
	cmd_info[CMD_Bf].kind = INSTANT_CMD;
	cmd_info[CMD_Bn].kind = INSTANT_CMD;
	cmd_info[CMD_BRR].kind = POLLING_CMD;
	cmd_info[CMD_BRW_yyyymmdd].kind = WRITE_CMD;
	cmd_info[CMD_Bv].kind = POLLING_CMD;
	cmd_info[CMD_C].kind = INSTANT_CMD;
	cmd_info[CMD_CSf].kind = WRITE_CMD;
	cmd_info[CMD_CSn].kind = WRITE_CMD;
	cmd_info[CMD_FWV].kind = POLLING_CMD;
	cmd_info[CMD_Lf_q].kind = POLLING_CMD;
	cmd_info[CMD_Oaf].kind = INSTANT_CMD;
	cmd_info[CMD_Oan].kind = INSTANT_CMD;
	cmd_info[CMD_Obf].kind = INSTANT_CMD;
	cmd_info[CMD_Obn].kind = INSTANT_CMD;
	cmd_info[CMD_Ocf].kind = INSTANT_CMD;
	cmd_info[CMD_Ocn].kind = INSTANT_CMD;
	cmd_info[CMD_OS_q].kind = POLLING_CMD;
	cmd_info[CMD_OUT].kind = WRITE_CMD;
	cmd_info[CMD_PSNR].kind = INITIAL_CMD;
	cmd_info[CMD_Q1].kind = POLLING_CMD;
	cmd_info[CMD_Rbd_q].kind = POLLING_CMD;
	cmd_info[CMD_RBV].kind = WRITE_CMD;
	cmd_info[CMD_Rbv_q].kind = POLLING_CMD;
	cmd_info[CMD_S_n].kind = INSTANT_CMD;
	cmd_info[CMD_S_nR_m].kind = INSTANT_CMD;
	cmd_info[CMD_Sf_n].kind = INSTANT_CMD;
	cmd_info[CMD_SS].kind = WRITE_CMD;
	cmd_info[CMD_Ss_q].kind = POLLING_CMD;
	cmd_info[CMD_TPb].kind = POLLING_CMD;
	cmd_info[CMD_UN].kind = POLLING_CMD;
	cmd_info[CMD_V].kind = POLLING_CMD;
	cmd_info[CMD_V2].kind = POLLING_CMD;
	cmd_info[CMD_V3].kind = POLLING_CMD;
	cmd_info[CMD_VA].kind = WRITE_CMD;
	cmd_info[CMD_VB].kind = WRITE_CMD;
	cmd_info[CMD_WR].kind = WRITE_CMD;
	cmd_info[CMD_RRTC].kind = POLLING_CMD;
	cmd_info[CMD_BLD].kind = WRITE_CMD;
	cmd_info[CMD_Bld_q].kind = POLLING_CMD;
	cmd_info[CMD_RTS].kind = POLLING_CMD;

	return 0;
}

/*
 * this function is confirms what isn't contained
 * invalid charactors in a parameter.
 */
static int omron_strspan(const char* str1, const char* str2) {
	if (strcmp(str2, "*")==0) {
		return 1;
	}
	return (strspn(str1, str2) == strlen(str1));
}

static char* omron_strtostr(char* str) {
	return str;
}

static char* omron_strtobatdate(char* str) {
	sprintf(omron_work_buf, "%c%c/%c%c/%c%c", str[4], str[5], str[6], str[7], str[2], str[3]);
	return omron_work_buf;
}

static char* omron_f1tostr(char* str) {
	sprintf(omron_work_buf, "%.1f", strtod(str, NULL));
	return omron_work_buf;
}

static char* omron_strto3d(char* str) {
	sprintf(omron_work_buf, "%03ld", strtol(str, NULL, 10));
	return omron_work_buf;
}

static char* omron_osparser(char* buf) {
	int statusA, switchableA, statusB, switchableB, statusC, switchableC;

	if (sscanf(buf, "A:%1d%1d B:%1d%1d C:%1d%1d",
			&switchableA, &statusA, &switchableB, &statusB,
			&switchableC, &statusC) == 6) {
		dstate_setinfo(OUTLET_1_STATUS, (statusA == 0)? STR_OFF : STR_ON);
		dstate_setinfo(OUTLET_1_SWITCHABLE, (switchableA == 0)? STR_NO : STR_YES);
		dstate_setinfo(OUTLET_2_STATUS, (statusB == 0)? STR_OFF : STR_ON);
		dstate_setinfo(OUTLET_2_SWITCHABLE, (switchableB == 0)? STR_NO : STR_YES);
		dstate_setinfo(OUTLET_3_STATUS, (statusC == 0)? STR_OFF : STR_ON);
		dstate_setinfo(OUTLET_3_SWITCHABLE, (switchableC == 0)? STR_NO : STR_YES);
	}
	return NULL;
}

static char* omron_strtoi(char* str) {
	sprintf(omron_work_buf, "%ld", strtol(str, NULL, 10));
	return omron_work_buf;
}

static char* omron_strtoix60(char* str) {
	sprintf(omron_work_buf, "%ld", strtol(str, NULL, 10)*60);
	return omron_work_buf;
}

static char* omron_strtoss(char* str) {
	if (strcmp(str, "1") == 0) {
		strcpy(omron_work_buf, STR_NORMAL_PRECISION);
	} else if (strcmp(str, "2") == 0) {
		strcpy(omron_work_buf, STR_LOW_PRECISION);
	} else if (strcmp(str, "3") == 0) {
		strcpy(omron_work_buf, STR_HIGH_PRECISION);
	}
	return omron_work_buf;
}

static char* omron_q1parser(char *buf) {
	const struct {
		const char	*var;
		const char	*fmt;
		double	(*conv)(const char *, char **);
	} status[] = {
		{ INPUT_VOLTAGE, "%.1f", strtod },
		{ INPUT_VOLTAGE_FAULT, "%.1f", strtod },
		{ OUTPUT_VOLTAGE, "%.1f", strtod },
		{ UPS_LOAD, "%.1f", strtod },
		{ INPUT_FREQUENCY, "%.2f", strtod },
		{ BATTERY_VOLTAGE, "%.2f", strtod },
		{ UPS_TEMPERATURE, "%.1f", strtod },
		{ NULL }
	};

	char	*val, *last = NULL;
	int	i;

	if (buf[0] != '(') {
		upsdebugx(2, "%s: invalid start character [%02x]", __func__, buf[0]);
		return NULL;
	}

	for (i = 0, val = strtok_r(buf+1, " ", &last); status[i].var; i++, val = strtok_r(NULL, " \r\n", &last)) {

		if (!val) {
			upsdebugx(2, "%s: parsing failed", __func__);
			return NULL;
		}

		if (strspn(val, "0123456789-.") != strlen(val)) {
			upsdebugx(2, "%s: invalid value [%s]", __func__, val);
			continue;
		}

		if (strspn(val, "0123456789.") == strlen(val)) {
			dstate_setinfo(status[i].var, status[i].fmt, status[i].conv(val, NULL));
		} else if (strspn(val, "-.") == strlen(val)) {
			dstate_setinfo(status[i].var, "%s", val);
		}
	}

	/* UPS Status */
	if (!val) {
		upsdebugx(2, "%s: parsing failed", __func__);
		return NULL;
	}

	if (strspn(val, "01") != 8) {
		upsdebugx(2, "Invalid status [%s]", val);
		return NULL;
	}

	/* Bit3: UPS Type */
	if (val[4] == '1') {	/* UPS Type is Standby (0 is On_line) */
		dstate_setinfo(UPS_TYPE, UPS_TYPE_OFFLINE);
	} else {
		dstate_setinfo(UPS_TYPE, UPS_TYPE_ONLINE);
	}

	/* set ups.status */
	status_init();

	/* Bit7: AC Input */
	if (val[0] == '1') {	/* Utility Fail (Immediate) */
		status_set("OB");
		online = 0;
	} else {
		status_set("OL");
		online = 1;
	}

	/* Bit6: Battery Level */
	if (val[1] == '1') {	/* Battery Low */
		status_set("LB");
	}

	/* Bit5: Bypass Active */
	if (val[2] == '1') {	/* Bypass/Boost or Buck Active */
/*
		double	vi, vo;

		vi = strtod(dstate_getinfo(INPUT_VOLTAGE),  NULL);
		vo = strtod(dstate_getinfo(OUTPUT_VOLTAGE), NULL);

		if (vo < 0.5 * vi) {
			upsdebugx(2, "%s: output voltage too low", __func__);
		} else if (vo < 0.95 * vi) {
			status_set("TRIM");
		} else if (vo < 1.05 * vi) {
			status_set("BYPASS");
		} else if (vo < 1.5 * vi) {
			status_set("BOOST");
		} else {
			upsdebugx(2, "%s: output voltage too high", __func__);
		}
*/
	}

	/* Bit2: Test Flag */
	if (val[5] == '1') {	/* Test in Progress */
		status_set("CAL");
	}

	/* set ups.alarm */
	alarm_init();

	/* Bit4: Self Test Result */
	if (val[3] == '1') {	/* UPS Failed */
		alarm_set("UPS selftest failed!");
	}

	/* Bit1: Shutdown Active */
	if (val[6] == '1') {	/* Shutdown Active */
		alarm_set("Shutdown imminent!");
		status_set("FSD");
	}

	alarm_commit();

	status_commit();

	return NULL;
}

static char* omron_fwvparser(char *buf) {
	char *p;
	char *last = NULL;

	if ((p = strtok_r(buf, "(", &last)) != NULL ) {
		dstate_setinfo(UPS_FIRMWARE, "%.2f", strtod(&p[2], NULL));
	}
	if ((p = strtok_r(NULL, ")", &last)) != NULL) {
		dstate_setinfo(UPS_FIRMWARE_AUX, "%.2f", strtod(&p[2], NULL));
	}
	return NULL;
}

static char* omron_rrtcparser(char* buf) {
	int yy, w, mm, dd, hh, nn, ss;

	if (sscanf(buf, "%02d%1d%02d%02d%02d%02d%02d", &yy, &w, &mm, &dd, &hh, &nn, &ss) == 7) {
		dstate_setinfo(UPS_TIME, "%02d:%02d", hh, nn);
		dstate_setinfo(UPS_DATE, "%02d-%02d-%02d", yy, mm, dd);
	}
	return NULL;
}

static char* omron_unparser(char* buf) {
	int ups_realpower, ups_power;
	int battery_voltage_nominal;
	int max_power_consumption, min_power_consumption;

	if (sscanf(buf, "%04dW/%04dVA/%03dV/%03dW/%03dW",
			&ups_realpower, &ups_power,
			&battery_voltage_nominal,
			&max_power_consumption, &min_power_consumption) == 5) {
		dstate_setinfo(UPS_POWER, "%d", ups_power);
		dstate_setinfo(UPS_REALPOWER, "%d", ups_realpower);
		dstate_setinfo(BATTERY_VOLTAGE_NOMINAL, "%03d", battery_voltage_nominal);
	}
	return NULL;
}

static char* omron_vparser(char* buf) {
	dstate_setinfo(UPS_BEEPER_STATUS, (buf[1] == '0')? STR_ENABLE : STR_DISABLE);
	return NULL;
}

static char* omron_v2parser(char* buf) {
	dstate_setinfo(UPS_START_AUTO, (buf[0] == '0')? STR_YES : STR_NO);
	return NULL;
}

static char* omron_v3parser(char* buf) {
	dstate_setinfo(UPS_START_BATTERY, (buf[13] == '0')? STR_NO : STR_YES);
	return NULL;
}

static int omron_set_support(const char* cmd, unsigned long val) {
	const struct support_info {
		const omron_cmd_id supcmd;
		int   bitshift;
	} cf_supinfo[] = {
		{CMD_Q1,    31},
		{CMD_V,     30},
		{CMD_S_n   ,25},
		{CMD_S_nR_m,24},
		{CMD_C,     23},
		{CMD_Bn,    20},
		{CMD_Bf,    19},
		{CMD_BRW_yyyymmdd,   17},
		{CMD_BRR,   16},
		{CMD_Bv,    14},
		{CMD_Lf_q,  13},
		{CMD_Bl_q,  12},
		{CMD_Af,    10},
		{CMD_An,     9},
		{CMD_VA,     6},
		{CMD_VB,     5},
		{CMD_V2,     2},
		{CMD_Sf_n,   1},
		{-1,        -1}
	}, cf2_supinfo[] = {
		{CMD_Obf,   27},
		{CMD_Obn,   26},
		{CMD_Ocf,   25},
		{CMD_Ocn,   24},
		{CMD_RTS,	22},
		{CMD_V3,    18},
		{CMD_FWV,   16},
		{CMD_CSf,   10},
		{CMD_CSn,    9},
		{CMD_OUT,    6},
		{-1,        -1}
	}, cf3_supinfo[] = {
		{CMD_Rbd_q, 28},
		{CMD_WR,    10},
		{CMD_RRTC,   9},
		{CMD_OS_q,   1},
		{-1,        -1}
	}, cf4_supinfo[] = {
		{CMD_RBV,   31},
		{CMD_Rbv_q, 30},
		{CMD_SS,    29},
		{CMD_Ss_q,  28},
		{CMD_PSNR,  19},
		{CMD_Ss_q,  28},
		{CMD_UN,    16},
		{CMD_Oaf,   15},
		{CMD_Oan,   14},
		{CMD_TPb,   10},
		{-1,        -1}
	}, cf5_supinfo[] = {
		{CMD_BLD,   20},
		{CMD_Bld_q, 19},
		{-1,        -1}
	};

	const struct {
		const char* cmd;
		const struct support_info* si;
	} cf_info[] = {
		{"CF\r",  cf_supinfo},
		{"CF2\r", cf2_supinfo},
		{"CF3\r", cf3_supinfo},
		{"CF4\r", cf4_supinfo},
		{"CF5\r", cf5_supinfo},
		{NULL,    NULL}
	};

	int idx;
	int spidx;
	int spflg;

	for (idx = 0; cf_info[idx].cmd >= 0; idx++) {
		if (strncmp(cmd, cf_info[idx].cmd, strlen(cmd)))
			continue;
		for (spidx = 0; cf_info[idx].si[spidx].supcmd != (omron_cmd_id)-1; spidx++) {
			const struct support_info* si = &cf_info[idx].si[spidx];
			spflg = (val >> si->bitshift) & 0x1;
			cmd_info[si->supcmd].is_supported = (spflg!=0);
		}
		break;
	}
	return 0;
}

static int omron_support() {
	const char* cf_cmd[] = {"CF\r", "CF2\r", "CF3\r", "CF4\r", "CF5\r", NULL};

	unsigned long usable_function;

	char buf[SMALLBUF];
	int reply_length;
	int cf_idx = 0;
	char* p;

	for (cf_idx = 0; cf_cmd[cf_idx] != NULL; cf_idx++) {
		usable_function = 0;

		/* CFx command execute */
		reply_length = omron_command(cf_cmd[cf_idx], buf, sizeof(buf));

		/*
		 * response from UPS :
                 *  b7b6b5b4b3b2b1b0 b7b6b5b4b3b2b1b0 b7b6b5b4b3b2b1b0 b7b6b5b4b3b2b1b0 *n<cr>
		 * b[7-0] = "1" | "0"
                 */
		if (strncmp(buf, STR_NAK, 3) == 0) {
			continue;
		}

		if (reply_length < 40) {
			upsdebugx(2, "%s: short reply", __func__);
			continue;
		}

		buf[SMALLBUF-1] = '\0';
		for (p = strtok(buf, " ");; p = strtok(NULL, " ")) {
			if (p == NULL || p[0] == '*') {
				break;
			}
			usable_function <<= 8;
			usable_function |= strtol(p, NULL, 2);
		}

		/* set support flag for connected UPS */
		omron_set_support(cf_cmd[cf_idx], usable_function);
	}

	return 0;
}

static int omron_status(const omron_cmd_id id) {

	const struct {
		const omron_cmd_id id;
		const char* cmd;
		const int   reply_size;
		int (*check)(const char*, const char*);
		const char* check_words;
		const char* nut_var;
		char* (*conv)(char*);
	} cmd_map[] = {
		{CMD_Bl_q,  "Bl ?\r",  4, omron_strspan, CMP_NUM,           BATTERY_CHARGE, omron_f1tostr},
		{CMD_BRR,   "BRR\r",   9, omron_strspan, CMP_NUM,           BATTERY_DATE,   omron_strtobatdate},
		{CMD_Bv,    "Bv\r",    4, omron_strspan, CMP_NUM ".",       BATTERY_VOLTAGE_NOMINAL,   omron_strto3d},
		{CMD_FWV,   "FWV\r",  15, omron_strspan, CMP_NUM "MS.:() ", NULL, omron_fwvparser},
		{CMD_Lf_q,  "Lf ?\r",  4, omron_strspan, CMP_NUM ".",       OUTPUT_FREQUENCY,   omron_f1tostr},
		{CMD_OS_q,  "OS ?\r", 15, omron_strspan, CMP_NUM "ABC: ",   NULL,   omron_osparser},
		{CMD_PSNR,  "PSNR\r", 17, omron_strspan, "*",               UPS_SERIAL, omron_strtostr},
		{CMD_Q1,    "Q1\r",   46, omron_strspan, CMP_HEX "( -.",     NULL,         omron_q1parser},
		{CMD_Rbv_q, "Rbv ?\r", 4, omron_strspan, CMP_NUM,           BATTERY_CHARGE_RESTART,  omron_strtoi},
		{CMD_Ss_q,  "Ss ?\r",  2, omron_strspan, "123",             INPUT_SENSITIVITY,   omron_strtoss},
		{CMD_TPb,   "TPb\r",   5, omron_strspan, CMP_NUM ".+-",     BATTERY_TEMPERATURE,  omron_strtostr},
		{CMD_UN,    "UN\r",   27, omron_strspan, CMP_NUM "WVA/",    NULL,        omron_unparser},
		{CMD_V,     "V\r",     8, omron_strspan, "01 ",             NULL,        omron_vparser},
		{CMD_V2,    "V2\r",    8, omron_strspan, "01 ",             NULL,        omron_v2parser},
		{CMD_V3,    "V3\r",   17, omron_strspan, "01 ",             NULL,        omron_v3parser},
		{CMD_RRTC,  "RRTC\r", 13, omron_strspan, CMP_NUM,           NULL,        omron_rrtcparser},
		{CMD_Rbd_q, "Rbd ?\r", 4, omron_strspan, CMP_NUM,           UPS_DELAY_REBOOT,  omron_strtoi},
		{CMD_Bld_q, "Bld ?\r", 4, omron_strspan, CMP_NUM,           BATTERY_CHARGE_LOW,  omron_strtoi},
		{CMD_RTS,   "RTS\r",   4, omron_strspan, CMP_NUM,           BATTERY_RUNTIME,  omron_strtoix60},
		{-1,        NULL,      0, NULL,          NULL,              NULL,        NULL}
	};

	char buf[SMALLBUF];
	int reply_length;
	int idx = 0;

	for (idx = 0; cmd_map[idx].id >= 0; idx++) {
		if (id == cmd_map[idx].id) {
			break;
		}
	}
	if (cmd_map[idx].id < 0) {
		return -1;
	}

	reply_length = omron_command(cmd_map[idx].cmd, buf, sizeof(buf));
	if (reply_length < 0 ) {
		upsdebugx(2, "%s: lost connection(%d)", __func__, reply_length);
		return -2;
	}
	if (reply_length < cmd_map[idx].reply_size) {
		upsdebugx(2, "%s: short reply(%d)", __func__, reply_length);
		return -1;
	}
	buf[SMALLBUF-1] = '\0';

	if (strtok(buf, "\r") != NULL ) {
		char* val = NULL;
		if (!cmd_map[idx].check(buf, cmd_map[idx].check_words)) {
			upsdebugx(2, "%s: illegal string value [%s] rule [%s]",
					__func__, buf, cmd_map[idx].check_words);
			return -1;
		}
		if (cmd_map[idx].conv != NULL) {
			val = cmd_map[idx].conv(buf);
		}
		if (cmd_map[idx].nut_var != NULL && val != NULL) {
			dstate_setinfo(cmd_map[idx].nut_var, "%s", val);
		}
	}
	return 0;
}

static char* on_shutdown_return(const char *extra) {
	if (offdelay < 60) {
		snprintf(omron_work_buf, sizeof(omron_work_buf), "S.%d\r", offdelay / 6);
	} else {
		snprintf(omron_work_buf, sizeof(omron_work_buf), "S%02d\r", offdelay / 60);
	}
	return omron_work_buf;
}

static char* on_shutdown_stayoff(const char *extra) {
	if (offdelay < 60) {
		snprintf(omron_work_buf, sizeof(omron_work_buf),
			"Sf.%d\r", offdelay / 6);
	} else {
		snprintf(omron_work_buf, sizeof(omron_work_buf),
			"Sf%02d\r", offdelay / 60);
	}
	return omron_work_buf;
}

static char* on_shutdown_stop(const char *extra) {
	strcpy(omron_work_buf, "C\r");
	return omron_work_buf;
}

static char* on_shutdown_reboot(const char *extra) {
	snprintf(omron_work_buf, sizeof(omron_work_buf),
		"S.%dR%05d\r", 2, ondelay);
	return omron_work_buf;
}

static char* on_shutdown_reboot_graceful(const char *extra) {
	if (offdelay < 60) {
		snprintf(omron_work_buf, sizeof(omron_work_buf),
			"S.%dR%05d\r", offdelay / 6, ondelay);
	} else {
		snprintf(omron_work_buf, sizeof(omron_work_buf),
			"S%02dR%05d\r", offdelay / 60, ondelay);
	}
	return omron_work_buf;
}

static char* on_beeper_enable(const char *extra) {
	strcpy(omron_work_buf, "Bn\r");
	return omron_work_buf;
}

static char* on_beeper_disable(const char *extra) {
	strcpy(omron_work_buf, "Bf\r");
	return omron_work_buf;
}

static char* on_outlet_n_load_on(int n) {
	switch (n){
	case 1:
		strcpy(omron_work_buf, "Oan\r");
		break;
	case 2:
		strcpy(omron_work_buf, "Obn\r");
		break;
	case 3:
		strcpy(omron_work_buf, "Ocn\r");
		break;
	default:
		return NULL;
	}
	return omron_work_buf;
}

static char* on_outlet_n_load_off(int n) {
	switch (n){
	case 1:
		strcpy(omron_work_buf, "Oaf\r");
		break;
	case 2:
		strcpy(omron_work_buf, "Obf\r");
		break;
	case 3:
		strcpy(omron_work_buf, "Ocf\r");
		break;
	default:
		return NULL;
	}
	return omron_work_buf;
}

static char* on_outlet_1_load_on(const char *extra) {
	return on_outlet_n_load_on(1);
}

static char* on_outlet_1_load_off(const char *extra) {
	return on_outlet_n_load_off(1);
}

static char* on_outlet_2_load_on(const char *extra) {
	return on_outlet_n_load_on(2);
}

static char* on_outlet_2_load_off(const char *extra) {
	return on_outlet_n_load_off(2);
}

static char* on_outlet_3_load_on(const char *extra) {
	return on_outlet_n_load_on(3);
}

static char* on_outlet_3_load_off(const char *extra) {
	return on_outlet_n_load_off(3);
}

static int omron_instcmd(const char *cmdname, const char *extra) {
	struct {
		const char *name;
		char* (*build_cmd)(const char* extra);
		char* nut_var;
		char* val;
	} handlers[] = {
		{SHUTDOWN_RETURN, on_shutdown_return, NULL, NULL},
		{SHUTDOWN_STAYOFF, on_shutdown_stayoff, NULL, NULL},
		{SHUTDOWN_STOP, on_shutdown_stop, NULL, NULL},
		{SHUTDOWN_REBOOT, on_shutdown_reboot, NULL, NULL},
		{SHUTDOWN_REBOOT_GRACEFUL, on_shutdown_reboot_graceful, NULL, NULL},
		{BEEPER_ENABLE, on_beeper_enable, UPS_BEEPER_STATUS, STR_ENABLE},
		{BEEPER_DISABLE, on_beeper_disable, UPS_BEEPER_STATUS, STR_DISABLE},
		{OUTLET_1_LOAD_ON, on_outlet_1_load_on, OUTLET_1_STATUS, STR_ON},
		{OUTLET_1_LOAD_OFF, on_outlet_1_load_off, OUTLET_1_STATUS, STR_OFF},
		{OUTLET_2_LOAD_ON, on_outlet_2_load_on, OUTLET_2_STATUS, STR_ON},
		{OUTLET_2_LOAD_OFF, on_outlet_2_load_off, OUTLET_2_STATUS, STR_OFF},
		{OUTLET_3_LOAD_ON, on_outlet_3_load_on, OUTLET_3_STATUS, STR_ON},
		{OUTLET_3_LOAD_OFF, on_outlet_3_load_off, OUTLET_3_STATUS, STR_OFF},
		{NULL, NULL},
	};
 	char buf[SMALLBUF] = "";
	char *cmd = NULL;
	char *nut_var = NULL;
	char *val = NULL;
	int idx;

	for (idx = 0; handlers[idx].name != NULL; idx++) {
		if (strcasecmp(handlers[idx].name, cmdname))
			continue;
		if (handlers[idx].build_cmd != NULL) {
			cmd = (handlers[idx].build_cmd)(extra);
		}
		if (handlers[idx].nut_var != NULL && handlers[idx].val != NULL) {
			nut_var = handlers[idx].nut_var;
			val = handlers[idx].val;
		}
		break;
	}

	if (cmd != NULL) {
		if (strcasecmp(cmdname, SHUTDOWN_RETURN) == 0) {
			/* Set "on" to "ups.start.auto" before "S<n>" command execution */
			if (cmd_info[CMD_An].is_supported) {
				if (omron_command("An\r", buf, sizeof(buf)) > 0) {
					if (strncmp(buf, STR_OK, 2) == 0) {
						dstate_setinfo(UPS_START_AUTO, STR_YES);
					} else {
						upslogx(LOG_ERR, "instcmd: command [An] failed");
						return STAT_INSTCMD_FAILED;
					}
				}
			}
		} else if (strcasecmp(cmdname, SHUTDOWN_STAYOFF) == 0) {
			/* Set "off" to "ups.start.auto" before "Sf<n>" command execution */
			if (cmd_info[CMD_Af].is_supported) {
				if (omron_command("Af\r", buf, sizeof(buf)) > 0) {
					if (strncmp(buf, STR_OK, 2) == 0) {
						dstate_setinfo(UPS_START_AUTO, STR_NO);
					} else {
						upslogx(LOG_ERR, "instcmd: command [Af] failed");
						return STAT_INSTCMD_FAILED;
					}
				}
			}
		}
		if (omron_command(cmd, buf, sizeof(buf)) > 0) {
			if (strncmp(buf, STR_OK, 2) == 0) {
				if (nut_var != NULL && val != NULL) {
					dstate_setinfo(nut_var, val);
				}
			} else {
				upslogx(LOG_ERR, "instcmd: command [%s] failed", cmdname);
				return STAT_INSTCMD_FAILED;
			}
		} else {
			upslogx(LOG_ERR, "instcmd: command [%s] not found", cmdname);
			return STAT_INSTCMD_UNKNOWN;
		}
	}

	upslogx(LOG_INFO, "instcmd: command [%s] handled", cmdname);
	return STAT_INSTCMD_HANDLED;
}

void omron_makevartable(void) {
#if 0
	addvar(VAR_VALUE, "ondelay", "Delay before UPS startup (minutes)");
	addvar(VAR_VALUE, "offdelay", "Delay before UPS shutdown (seconds)");
#endif
}

static int truncate_ondelay(int nVal) {
	int retVal = nVal;

	retVal -= (nVal % 60);

	/* ondelay 's range is (1 * 60) seconds - (99999 * 60)  seconds */
	if (retVal < (1 * 60)) {
		retVal = (1 * 60);
	}
	if (retVal > (99999 * 60)) {
		retVal = (99999 * 60);
	}
	return retVal;
}

static int truncate_offdelay(int nVal) {
	int retVal = nVal;

	if (nVal < 60) {
		retVal -= (nVal % 6);
	} else {
		retVal -= (nVal % 60);
	}
	/* offdelay 's range is (2 * 6) seconds -  (30 * 60)  seconds */
	if (retVal < (2 * 6)) {
		retVal = (2 * 6);
	}
	if (retVal > (30 * 60)) {
		retVal = (30 * 60);
	}
	return retVal;
}

void omron_initups(void) {
	/* no process */
	return;
}

static int on_time_check(const char* val) {
	int hour = 0;
	int minute = 0;

	if (strlen(val) != UPS_TIME_LEN) {
		return STAT_SET_UNKNOWN;
	}

	if (sscanf(val, "%02d:%02d", &hour, &minute) != 2) {
		return STAT_SET_UNKNOWN;
	}

	if (hour < 0 || hour > 23) {
		return STAT_SET_UNKNOWN;
	}

	if (minute < 0 || minute > 59) {
		return STAT_SET_UNKNOWN;
	}

	return STAT_SET_HANDLED;
}

static int IsLeapYear(int year) {
	return ((year % 400) == 0) || (((year % 100) != 0) && ((year % 4) == 0));
}

static int date_check(int year, int month, int day) {
	const int daysinMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

	if (year < 0 || year > 99) {
		return STAT_SET_UNKNOWN;
	}

	if (month < 1 || month > 12) {
		return STAT_SET_UNKNOWN;
	}

	if (day < 1) {
		return STAT_SET_UNKNOWN;
	} else if (day > daysinMonth[month -1]) {
		/* Leap Year check */
		if ((month == 2) && IsLeapYear(year)) {
			if (day > 29) {
				return STAT_SET_UNKNOWN;
			}
		} else {
			return STAT_SET_UNKNOWN;
		}
	}

	return STAT_SET_HANDLED;
}

static int on_ups_date_check(const char* val) {
	int year = 0;
	int month = 0;
	int day = 0;

	if (strlen(val) != UPS_DATE_LEN) {
		return STAT_SET_UNKNOWN;
	}

	if (sscanf(val, "%02d-%02d-%02d", &year, &month, &day) != 3) {
		return STAT_SET_UNKNOWN;
	}

	return date_check(year, month, day);
}

static int GetDayOfWeek(int year, int month, int day) {
	struct tm this_date;

	this_date.tm_year = year;
	this_date.tm_mon = month - 1;
	this_date.tm_mday = day;
	this_date.tm_hour = 0;
	this_date.tm_min = 0;
	this_date.tm_sec = 0;
	this_date.tm_isdst = -1;
	if (mktime(&this_date) == (time_t)-1) {
		return -1;
	}

	return ((this_date.tm_wday == 0)? 7 : this_date.tm_wday);
}

static char* on_date_time_changed(const char* val) {
	enum DateTimeUpdate { DATE, TIME } proc;
	int valLen = 0;
	int year = 0;
	int month = 0;
	int day = 0;
	int hour = 0;
	int minute = 0;
	int dayOfWeek = -1;

	if (!(cmd_info[CMD_RRTC].is_supported &&
			cmd_info[CMD_WR].is_supported)){
		return NULL;
	}

	valLen = strlen(val);
	if (valLen == 5) {
		proc = TIME;
	} else if (valLen == 8) {
		proc = DATE;
	} else {
		return NULL;
	}

	if (proc == DATE) {
		if (strcmp(dstate_getinfo(UPS_TIME), STR_SETVAR_INIT) == 0) {
			sprintf(omron_work_buf, "%s", PARAMETER_SHORTAGE);
			return omron_work_buf;
		}

		if (sscanf(val, "%02d-%02d-%02d", &year, &month, &day) != 3) {
			return NULL;
		}

		if (sscanf(dstate_getinfo(UPS_TIME), "%02d:%02d", &hour, &minute) != 2) {
			return NULL;
		}

	} else {
		/* TIME */
		if (strcmp(dstate_getinfo(UPS_DATE), STR_SETVAR_INIT) == 0) {
			sprintf(omron_work_buf, "%s", PARAMETER_SHORTAGE);
			return omron_work_buf;
		}

		if (sscanf(val, "%02d:%02d", &hour, &minute) != 2) {
			return NULL;
		}

		if (sscanf(dstate_getinfo(UPS_DATE), "%02d-%02d-%02d", &year, &month, &day) != 3) {
			return NULL;
		}
	}

	dayOfWeek = GetDayOfWeek(year+100, month, day);
	if (dayOfWeek == -1) {
		return NULL;
	}

	sprintf(omron_work_buf, "WR%02d%01d%02d%02d%02d%02d00\r",
			year, dayOfWeek, month, day, hour, minute);
	return omron_work_buf;
}

static int on_ups_delay_check(const char* val) {
	if (strspn(val, CMP_NUM) != strlen(val)) {
		return STAT_SET_UNKNOWN;
	}
	return STAT_SET_HANDLED;
}

static char* on_auto_restart_changed(const char* val) {
	if (!(cmd_info[CMD_Af].is_supported &&
			cmd_info[CMD_An].is_supported)){
		return NULL;
	}

	if (!strcasecmp(val, STR_YES)) {
		sprintf(omron_work_buf, "An\r");
	} else if (!strcasecmp(val, STR_NO)) {
		sprintf(omron_work_buf, "Af\r");
	} else {
		return NULL;
	}
	return omron_work_buf;
}

static char* on_start_battery_changed(const char* val) {
	if (!(cmd_info[CMD_CSf].is_supported &&
			cmd_info[CMD_CSn].is_supported)){
		return NULL;
	}

	if (!strcasecmp(val, STR_YES)) {
		sprintf(omron_work_buf, "CSn\r");
	} else if (!strcasecmp(val, STR_NO)) {
		sprintf(omron_work_buf, "CSf\r");
	} else {
		return NULL;
	}
	return omron_work_buf;
}

static int on_battery_date_check(const char* val) {
	int year = 0;
	int month = 0;
	int day = 0;

	if (strlen(val) != BATTERY_DATE_LEN) {
		return STAT_SET_UNKNOWN;
	}

	if (sscanf(val, "%02d/%02d/%02d", &month, &day, &year) != 3) {
		return STAT_SET_UNKNOWN;
	}

	return date_check(year, month, day);
}

static char* on_battery_date_changed(const char* val) {
	int year=0;
	int month=0;
	int day=0;

	if (!cmd_info[CMD_BRW_yyyymmdd].is_supported) {
		return NULL;
	}

	if (sscanf(val, "%d/%d/%d", &month, &day, &year) != 3) {
		return NULL;
	}

	sprintf(omron_work_buf, "BRW20%02d%02d%02d\r", year, month, day);
	return omron_work_buf;
}

static char* on_input_sensitivity_changed(const char* val) {
	int ss = 0;

	if ((!cmd_info[CMD_SS].is_supported) &&
			!(cmd_info[CMD_VA].is_supported && cmd_info[CMD_VB].is_supported)){
		return NULL;
	}

	if (!strcasecmp(val, STR_HIGH_PRECISION)) {
		ss = 3;
	} else if (!strcasecmp(val, STR_NORMAL_PRECISION)) {
		ss = 1;
	} else if (!strcasecmp(val, STR_LOW_PRECISION)) {
		ss = 2;
	} else {
		return NULL;
	}

	if (cmd_info[CMD_SS].is_supported) {
		sprintf(omron_work_buf, "SS%d\r", ss);
	} else if (cmd_info[CMD_VA].is_supported && cmd_info[CMD_VB].is_supported) {
		if (ss != 3) {
			sprintf(omron_work_buf, "%s\r", (ss == 1)? "VA" : "VB");
		} else {
			return NULL;
		}
	}

	return omron_work_buf;
}

static char* on_output_voltage_nominal_changed(const char* val) {
	int voltage = 0;

	if (!cmd_info[CMD_OUT].is_supported) {
		return NULL;
	}

	if (strlen(val) != 3) {
		return NULL;
	}
	if (strspn(val, CMP_NUM) != 3) {
		return NULL;
	}
	if (sscanf(val, "%d", &voltage) != 1) {
		return NULL;
	}

	sprintf(omron_work_buf, "OUT%03d\r", voltage);
	return omron_work_buf;
}

static char* on_battery_charge_restart_changed(const char* val) {

	if (!cmd_info[CMD_RBV].is_supported) {
		return NULL;
	}

	if (strspn(val, CMP_NUM) != strlen(val)) {
		return NULL;
	}

	sprintf(omron_work_buf, "RBV%03ld\r", strtol(val, NULL, 10));
	return omron_work_buf;
}

static char* on_battery_charge_low_changed(const char* val) {

	if (!cmd_info[CMD_BLD].is_supported) {
		return NULL;
	}

	if (strspn(val, CMP_NUM) != strlen(val)) {
		return NULL;
	}

	sprintf(omron_work_buf, "BLD%03ld\r", strtol(val, NULL, 10));
	return omron_work_buf;
}

static int omron_setvar(const char *varname, const char *val) {
	struct {
		const char* var;
		int   (*check_val)(const char* val);
		char* (*build_cmd)(const char* val);
	} handlers[] = {
		{UPS_TIME, on_time_check, on_date_time_changed},
		{UPS_DATE, on_ups_date_check, on_date_time_changed},
		{UPS_DELAY_START, on_ups_delay_check, NULL},
		{UPS_DELAY_SHUTDOWN, on_ups_delay_check, NULL},
		{UPS_START_AUTO, NULL, on_auto_restart_changed},
		{UPS_START_BATTERY, NULL, on_start_battery_changed},
		{INPUT_SENSITIVITY, NULL, on_input_sensitivity_changed},
		{OUTPUT_VOLTAGE_NOMINAL, NULL, on_output_voltage_nominal_changed},
		{BATTERY_CHARGE_RESTART, NULL, on_battery_charge_restart_changed},
		{BATTERY_DATE, on_battery_date_check, on_battery_date_changed},
		{BATTERY_CHARGE_LOW, NULL, on_battery_charge_low_changed},
		{NULL, NULL, NULL}
	};
	int idx;
	volatile int ret = STAT_SET_HANDLED;
	char buf[SMALLBUF];
	char *cmd = NULL;
	int ondelay_seconds = 0;

	for (idx = 0; handlers[idx].var != NULL; idx++) {
		if (strcasecmp(handlers[idx].var, varname))
			continue;

		if (handlers[idx].check_val != NULL) {
			ret = (handlers[idx].check_val)(val);
			if (ret != STAT_SET_HANDLED) {
				upslogx(LOG_ERR, "setvar: varname[%s] val[%s] val invalid. "
						, varname, (val != NULL)? val : "none");
				break;
			}
		}
		if (handlers[idx].build_cmd != NULL) {
			cmd = (handlers[idx].build_cmd)(val);
			if (cmd == NULL) {
				/* cmd edit failed */
				upslogx(LOG_ERR, "setvar: varname[%s] val[%s] failed. "
						, varname, (val != NULL)? val : "none");
				ret = STAT_SET_UNKNOWN;
			}
		}
		break;
	}

	if (cmd != NULL && strcmp(cmd, PARAMETER_SHORTAGE) != 0) {
		if (omron_command(cmd, buf, sizeof(buf)) > 0) {
			if (strncmp(buf, STR_OK, 2)) {
				upslogx(LOG_ERR, "instcmd: command [%s] failed", buf);
				ret = STAT_SET_UNKNOWN;
			} else {
				ret = STAT_SET_HANDLED;
			}
		} else {
			ret = STAT_SET_UNKNOWN;
		}
	}

	if (ret == STAT_SET_HANDLED) {
		if (strncmp(varname, UPS_DELAY_START, strlen(UPS_DELAY_START)) == 0) {
			ondelay_seconds = atoi(val);
			ondelay_seconds = truncate_ondelay(ondelay_seconds);
			ondelay = ondelay_seconds / 60;
			dstate_setinfo(varname, "%d", ondelay_seconds);
		} else if(strncmp(varname, UPS_DELAY_SHUTDOWN, strlen(UPS_DELAY_SHUTDOWN)) == 0) {
			offdelay = atoi(val);
			offdelay = truncate_offdelay(offdelay);
			dstate_setinfo(varname, "%d", offdelay);
		} else {
			dstate_setinfo(varname, val);
		}
	}

	return ret;
}

void omron_initinfo(void) {
	int	retry;
	int	ret = -1;
	int entry;
	long prodid;

	omron_initvars();
	omron_support();

	for (retry = 1; retry <= MAXTRIES; retry++) {

		if (cmd_info[CMD_Q1].is_supported) {
			ret = omron_status(CMD_Q1);
			if (ret < 0) {
				upsdebugx(2, "Status read %d failed", retry);
				continue;
			}
		}

		if (cmd_info[CMD_PSNR].is_supported) {
			ret = omron_status(CMD_PSNR);
			if (ret < 0) {
				upsdebugx(2, "Serial Number read %d failed", retry);
				continue;
			}
		}

		upsdebugx(2, "Status read in %d tries", retry);
		break;
	}

	if (!ret) {
		upslogx(LOG_INFO, "Supported UPS detected");
	}

	/* Truncate to nearest setable value */
	offdelay = truncate_offdelay(offdelay);

	/* set initial value */
	dstate_setinfo(UPS_DELAY_START, "%d", 60 * ondelay);
	dstate_setinfo(UPS_DELAY_SHUTDOWN, "%d", offdelay);
	dstate_setinfo(BATTERY_CHARGE, "0");
	dstate_setinfo(BATTERY_RUNTIME, "0");

	dstate_setflags(UPS_DELAY_START, ST_FLAG_RW | ST_FLAG_STRING);
	dstate_setaux(UPS_DELAY_START, 7);
	dstate_setflags(UPS_DELAY_SHUTDOWN, ST_FLAG_RW | ST_FLAG_STRING);
	dstate_setaux(UPS_DELAY_SHUTDOWN, 4);

	/* set range */
	dstate_addrange(UPS_DELAY_START, 1 * 60, 99999 * 60);
	dstate_addrange(UPS_DELAY_SHUTDOWN, 12, 1800);

	if (cmd_info[CMD_Af].is_supported &&
			cmd_info[CMD_An].is_supported) {
		dstate_setinfo(UPS_START_AUTO, STR_SETVAR_INIT);
		dstate_setflags(UPS_START_AUTO, ST_FLAG_RW | ST_FLAG_STRING);
		dstate_setaux(UPS_START_AUTO, 3);
		dstate_addenum(UPS_START_AUTO, STR_YES);
		dstate_addenum(UPS_START_AUTO, STR_NO);
	}

	if (cmd_info[CMD_BRW_yyyymmdd].is_supported) {
		dstate_setinfo(BATTERY_DATE, STR_SETVAR_INIT);
		dstate_setflags(BATTERY_DATE, ST_FLAG_RW | ST_FLAG_STRING);
		dstate_setaux(BATTERY_DATE, 8);
	}

	if (cmd_info[CMD_RBV].is_supported) {
		dstate_setinfo(BATTERY_CHARGE_RESTART, "0");
		dstate_setflags(BATTERY_CHARGE_RESTART, ST_FLAG_RW | ST_FLAG_STRING);
		dstate_setaux(BATTERY_CHARGE_RESTART, 3);
		dstate_addrange(BATTERY_CHARGE_RESTART, 0, 100);
	}

	if (cmd_info[CMD_CSf].is_supported &&
			cmd_info[CMD_CSn].is_supported) {
		dstate_setinfo(UPS_START_BATTERY, STR_SETVAR_INIT);
		dstate_setflags(UPS_START_BATTERY, ST_FLAG_RW | ST_FLAG_STRING);
		dstate_setaux(UPS_START_BATTERY, 3);
		dstate_addenum(UPS_START_BATTERY, STR_YES);
		dstate_addenum(UPS_START_BATTERY, STR_NO);
	}

	if (cmd_info[CMD_OUT].is_supported) {
		dstate_setinfo(OUTPUT_VOLTAGE_NOMINAL, STR_SETVAR_INIT);
		dstate_setflags(OUTPUT_VOLTAGE_NOMINAL, ST_FLAG_RW | ST_FLAG_STRING);
		dstate_setaux(OUTPUT_VOLTAGE_NOMINAL, 3);
		dstate_addrange(OUTPUT_VOLTAGE_NOMINAL, 100, 240);
	}

	if ((cmd_info[CMD_SS].is_supported) ||
			(cmd_info[CMD_VA].is_supported && cmd_info[CMD_VB].is_supported)){
		dstate_setinfo(INPUT_SENSITIVITY, STR_SETVAR_INIT);
		dstate_setflags(INPUT_SENSITIVITY, ST_FLAG_RW | ST_FLAG_STRING);
		dstate_setaux(INPUT_SENSITIVITY, 6);
		dstate_addenum(INPUT_SENSITIVITY, STR_HIGH_PRECISION);
		dstate_addenum(INPUT_SENSITIVITY, STR_LOW_PRECISION);
		dstate_addenum(INPUT_SENSITIVITY, STR_NORMAL_PRECISION);
	}

	if (cmd_info[CMD_RRTC].is_supported &&
			cmd_info[CMD_WR].is_supported) {
		dstate_setinfo(UPS_TIME, STR_SETVAR_INIT);
		dstate_setflags(UPS_TIME, ST_FLAG_RW | ST_FLAG_STRING);
		dstate_setaux(UPS_TIME, 5);
		dstate_setinfo(UPS_DATE, STR_SETVAR_INIT);
		dstate_setflags(UPS_DATE, ST_FLAG_RW | ST_FLAG_STRING);
		dstate_setaux(UPS_DATE, 8);
	}

	if (cmd_info[CMD_BLD].is_supported) {
		dstate_setinfo(BATTERY_CHARGE_LOW, STR_SETVAR_INIT);
		dstate_setflags(BATTERY_CHARGE_LOW, ST_FLAG_RW | ST_FLAG_STRING);
		dstate_setaux(BATTERY_CHARGE_LOW, 3);
		dstate_addrange(BATTERY_CHARGE_LOW, 0, 100);
	}

	if (cmd_info[CMD_S_n].is_supported) {
		dstate_addcmd(SHUTDOWN_RETURN);
	}

	if (cmd_info[CMD_Sf_n].is_supported) {
		dstate_addcmd(SHUTDOWN_STAYOFF);
	}

	if (cmd_info[CMD_C].is_supported) {
		dstate_addcmd(SHUTDOWN_STOP);
	}

	if (cmd_info[CMD_S_nR_m].is_supported) {
		dstate_addcmd(SHUTDOWN_REBOOT);
		dstate_addcmd(SHUTDOWN_REBOOT_GRACEFUL);
	}

	if (cmd_info[CMD_Bn].is_supported &&
			cmd_info[CMD_Bf].is_supported) {
		dstate_addcmd(BEEPER_ENABLE);
		dstate_addcmd(BEEPER_DISABLE);
	}

	if (cmd_info[CMD_Oan].is_supported &&
			cmd_info[CMD_Oaf].is_supported) {
		dstate_addcmd(OUTLET_1_LOAD_ON);
		dstate_addcmd(OUTLET_1_LOAD_OFF);
	}

	if (cmd_info[CMD_Obn].is_supported &&
			cmd_info[CMD_Obf].is_supported) {
		dstate_addcmd(OUTLET_2_LOAD_ON);
		dstate_addcmd(OUTLET_2_LOAD_OFF);
	}

	if (cmd_info[CMD_Ocn].is_supported &&
			cmd_info[CMD_Ocf].is_supported) {
		dstate_addcmd(OUTLET_3_LOAD_ON);
		dstate_addcmd(OUTLET_3_LOAD_OFF);
	}

	upsh.instcmd = omron_instcmd;
	upsh.setvar = omron_setvar;

	if (!cmd_info[CMD_RTS].is_supported) {
		prodid = strtol(dstate_getinfo(UPS_PRODUCTID), NULL, 16);
		for (entry=0; runtime_table[entry].prod_id != -1; entry++) {
			if (runtime_table[entry].prod_id == prodid) {
				runtime_exp = ( 3.2552725 - log10(runtime_table[entry].runtimeFull * 60.0) ) / ( log10(runtime_table[entry].loadC1) - 2 );	//log(1800), log(100)
				runtime_vol = pow( 10, log10(runtime_table[entry].runtimeFull*60) - (runtime_exp * 2) );
				runtime_cons = runtime_table[entry].consumption;
				break;
			}
		}
	}

}

void upsdrv_updateinfo(void) {
	static int retry = 0;
	static int cmd_index = 0;
	int start_cmd_index = 0;
	time_t start, now;
	int runtime = 0;
	double load = 0;
	double charge = 0;

	/*
	 * 
	 */
	for (time(&start); difftime(now, start) < POLLING_TIME; time(&now), cmd_index++) {
		if (MAX_CMD_NUM <= cmd_index) {
			cmd_index = 0;
		}

		if (start_cmd_index == 0) {
			start_cmd_index = cmd_index;
		} else if (start_cmd_index == cmd_index) {
			break;
		}

		if (!cmd_info[cmd_index].is_supported ||
			cmd_info[cmd_index].kind != POLLING_CMD) {
			continue;
		}

		int status_result = omron_status(cmd_index);
		if (status_result == -2) {
			dstate_datastale();
			upslogx(LOG_WARNING, "omron_status failed");
			return;
		}
		if (status_result < 0) {

			if (retry < MAXTRIES) {
				upsdebugx(1, "Communications with UPS lost: status read failed!");
				retry++;
			} else if (retry == MAXTRIES) {
				upslogx(LOG_WARNING, "Communications with UPS lost: status read failed!");
				retry++;
			} else {
				dstate_datastale();
			}
			cmd_info[cmd_index].error_count++;
			if (cmd_info[cmd_index].error_count >= MAXTRIES) {
				cmd_info[cmd_index].is_supported = false;
			}
			cmd_index++;
			upslogx(LOG_WARNING, "omron_status failed %d", retry);
			return;
		} else {
			upsdebugx(3, "omron_status %d\n", cmd_index);
			retry = 0;
		}
	}

	if (!cmd_info[CMD_RTS].is_supported) {
		load = strtod(dstate_getinfo(UPS_LOAD), NULL);
		charge = strtod(dstate_getinfo(BATTERY_CHARGE), NULL);
		if (load) {
			runtime = runtime_vol * pow(load, runtime_exp);
			runtime = charge / (100.0/runtime + runtime_cons/100.0);
		} else {
			runtime = charge / runtime_cons;
		}
		dstate_setinfo(BATTERY_RUNTIME, "%d", runtime);
	}

	if (retry > MAXTRIES) {
		upslogx(LOG_NOTICE, "Communications with UPS re-established");
	}

	dstate_dataok();
}

void upsdrv_shutdown(void) {
	int	retry;

	for (retry = 1; retry <= MAXTRIES; retry++) {

		if (omron_instcmd(SHUTDOWN_STOP, NULL) != STAT_INSTCMD_HANDLED) {
			continue;
		}

		if (omron_instcmd(SHUTDOWN_RETURN, NULL) != STAT_INSTCMD_HANDLED) {
			continue;
		}

		fatalx(EXIT_SUCCESS, "Shutting down in %d seconds", offdelay);
	}

	fatalx(EXIT_FAILURE, "Shutdown failed!");
}
