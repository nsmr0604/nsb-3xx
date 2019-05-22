/*
 * omron_usb.c: driver for UPS with OMRON dry-contact to USB
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
#include "libusb.h"
#include "usb-common.h"
#include "omron.h"

#define DRIVER_NAME	"OMRON USB driver"
#define DRIVER_VERSION	"1.02"

/* driver description structure */
upsdrv_info_t upsdrv_info = {
	DRIVER_NAME,
	DRIVER_VERSION,
	"omron customer support <omron_support@omron.co.jp>",
	DRV_BETA,
	{ NULL }
};

static usb_communication_subdriver_t *usb = &usb_subdriver;
static usb_dev_handle		*udev = NULL;
static USBDevice_t		usbdevice;
static USBDeviceMatcher_t	*reopen_matcher = NULL;
static USBDeviceMatcher_t	*regex_matcher = NULL;
static int					langid_fix = -1;

static int omron_command_exec(const char *cmd, char *buf, size_t buflen)
{
	char	tmp[64];
	int	ret;
	size_t	i;

	snprintf(tmp, sizeof(tmp), "%s", cmd);

	for (i = 0; i < strlen(tmp); i += ret) {

		/* Write data in 8-byte chunks */
		ret = usb_control_msg(udev, USB_ENDPOINT_OUT + USB_TYPE_CLASS + USB_RECIP_INTERFACE,
			0x09, 0x2, 0, &tmp[i], 16, 1000);

		if (ret <= 0) {
			upsdebugx(3, "send: %s", (ret != -ETIMEDOUT) ? usb_strerror() : "Connection timed out");
			return ret;
		}
	}

	upsdebugx(3, "send: %.*s", (int)strcspn(tmp, "\r"), tmp);

	/* Read all 64 bytes of the reply in one large chunk */
	ret = usb_interrupt_read(udev, 0x81, tmp, sizeof(tmp), 1000);

	/*
	 * Any errors here mean that we are unable to read a reply (which
	 * will happen after successfully writing a command to the UPS)
	 */
	if (ret <= 0) {
		upsdebugx(3, "read: %s", (ret != -ETIMEDOUT) ? usb_strerror() : "Connection timed out");
		return ret;
	}

	snprintf(buf, buflen, "%.*s", ret, tmp);

	upsdebugx(3, "read: %.*s", (int)strcspn(buf, "\r"), buf);
	return ret;
}

static usb_device_id_t omron_usb_id[] = {
//	{ USB_DEVICE(0x0590, 0x0043), NULL },	/* OMRON BN240XR */
//	{ USB_DEVICE(0x0590, 0x0044), NULL },	/* OMRON BN150XR */
	{ USB_DEVICE(0x0590, 0x004B), NULL },	/* OMRON BZ50T */
	{ USB_DEVICE(0x0590, 0x004F), NULL },	/* OMRON BZ50LT */
	{ USB_DEVICE(0x0590, 0x0050), NULL },	/* OMRON BZ35T */
//	{ USB_DEVICE(0x0590, 0x0054), NULL },	/* OMRON BN100XR */
	{ USB_DEVICE(0x0590, 0x0057), NULL },	/* OMRON BX50F */
	{ USB_DEVICE(0x0590, 0x0058), NULL },	/* OMRON BX35F */
	{ USB_DEVICE(0x0590, 0x0065), NULL },	/* OMRON BY50FW */
	{ USB_DEVICE(0x0590, 0x0066), NULL },	/* OMRON BY75SW */
	{ USB_DEVICE(0x0590, 0x006B), NULL },	/* OMRON BU1002SW */
	{ USB_DEVICE(0x0590, 0x006C), NULL },	/* OMRON BU3002SW */
	{ USB_DEVICE(0x0590, 0x006E), NULL },	/* OMRON BN50S */
	{ USB_DEVICE(0x0590, 0x006F), NULL },	/* OMRON BN75S */
	{ USB_DEVICE(0x0590, 0x0070), NULL },	/* OMRON BN100S */
	{ USB_DEVICE(0x0590, 0x0071), NULL },	/* OMRON BN150S */
	{ USB_DEVICE(0x0590, 0x0072), NULL },	/* OMRON BN220S */
	{ USB_DEVICE(0x0590, 0x0073), NULL },	/* OMRON BN300S */
	{ USB_DEVICE(0x0590, 0x0075), NULL },	/* OMRON BU100RW */
	{ USB_DEVICE(0x0590, 0x0076), NULL },	/* OMRON BU200RW */
	{ USB_DEVICE(0x0590, 0x0077), NULL },	/* OMRON BU300RW */
	{ USB_DEVICE(0x0590, 0x0080), NULL },	/* OMRON BY35S */
	{ USB_DEVICE(0x0590, 0x0081), NULL },	/* OMRON BY50S */
	{ USB_DEVICE(0x0590, 0x0083), NULL },	/* OMRON BU75RW */
	{ USB_DEVICE(0x0590, 0x0086), NULL },	/* OMRON BZ35LT2 */
	{ USB_DEVICE(0x0590, 0x0087), NULL },	/* OMRON BZ50LT2 */
	{ USB_DEVICE(0x0590, 0x00A1), NULL },	/* OMRON BY80S */
	{ USB_DEVICE(0x0590, 0x00A3), NULL },	/* OMRON BY120S */
	{ USB_DEVICE(0x0590, 0x00AD), NULL },	/* OMRON BN75R */
	{ USB_DEVICE(0x0590, 0x00AE), NULL },	/* OMRON BN150R */
	{ USB_DEVICE(0x0590, 0x00AF), NULL },	/* OMRON BN300R */
	{ USB_DEVICE(0x0590, 0x00B4), NULL },	/* OMRON BN50T */
	{ USB_DEVICE(0x0590, 0x00B5), NULL },	/* OMRON BN75T */
	{ USB_DEVICE(0x0590, 0x00B6), NULL },	/* OMRON BN100T */
	{ USB_DEVICE(0x0590, 0x00B7), NULL },	/* OMRON BN150T */
	{ USB_DEVICE(0x0590, 0x00B8), NULL },	/* OMRON BN220T */
	{ USB_DEVICE(0x0590, 0x00B9), NULL },	/* OMRON BN300T */
	/* end of list */
	{-1, -1, NULL}
};


static int device_match_func(USBDevice_t *hd, void *privdata)
{

	switch (is_usb_device_supported(omron_usb_id, hd))
	{
	case SUPPORTED:
	case POSSIBLY_SUPPORTED:
		return 1;

	case NOT_SUPPORTED:
	default:
		return 0;
	}
}

static USBDeviceMatcher_t device_matcher = {
	&device_match_func,
	NULL,
	NULL
};

/*
 * Generic command processing function. Send a command and read a reply.
 * Returns < 0 on error, 0 on timeout and the number of bytes read on
 * success.
 */
int omron_command(const char *cmd, char *buf, size_t buflen)
{
	int	ret;

	if (udev == NULL) {
		ret = usb->open(&udev, &usbdevice, reopen_matcher, NULL);

		if (ret < 1) {
			return ret;
		}
	}

	ret = omron_command_exec(cmd, buf, buflen);
	if (ret >= 0) {
		return ret;
	}

	switch (ret)
	{
	case -EBUSY:		/* Device or resource busy */
		fatal_with_errno(EXIT_FAILURE, "Got disconnected by another driver");

	case -EPERM:		/* Operation not permitted */
		fatal_with_errno(EXIT_FAILURE, "Permissions problem");

	case -EPIPE:		/* Broken pipe */
		if (usb_clear_halt(udev, 0x81) == 0) {
			upsdebugx(1, "Stall condition cleared");
			break;
		}
#ifdef ETIME
	case -ETIME:		/* Timer expired */
#endif
		if (usb_reset(udev) == 0) {
			upsdebugx(1, "Device reset handled");
		}
	case -ENODEV:		/* No such device */
	case -EACCES:		/* Permission denied */
	case -EIO:		/* I/O error */
	case -ENXIO:		/* No such device or address */
	case -ENOENT:		/* No such file or directory */
		/* Uh oh, got to reconnect! */
		usb->close(udev);
		udev = NULL;
		break;

	case -ETIMEDOUT:	/* Connection timed out */
	case -EOVERFLOW:	/* Value too large for defined data type */
	case -EPROTO:		/* Protocol error */
	default:
		break;
	}

	return ret;
}


void upsdrv_help(void)
{
	printf("Read The Fine Manual ('man 8 omron')\n");
}


void upsdrv_makevartable(void)
{
//#if 0
	addvar(VAR_VALUE, "subdriver", "Serial-over-USB subdriver selection");
	addvar(VAR_VALUE, "vendorid", "Regular expression to match UPS Manufacturer numerical ID (4 digits hexadecimal)");
	addvar(VAR_VALUE, "productid", "Regular expression to match UPS Product numerical ID (4 digits hexadecimal)");
	addvar(VAR_VALUE, "vendor", "Regular expression to match UPS Manufacturer string");
	addvar(VAR_VALUE, "product", "Regular expression to match UPS Product string");
	addvar(VAR_VALUE, "serial", "Regular expression to match UPS Serial number");
	addvar(VAR_VALUE, "bus", "Regular expression to match USB bus name");
//#endif
	omron_makevartable();
}


void upsdrv_initups(void)
{
	int	ret, langid;
	char	tbuf[255]; /* Some devices choke on size > 255 */
	char	*regex_array[6];

	regex_array[0] = getval("vendorid");
	regex_array[1] = getval("productid");
	regex_array[2] = getval("vendor");
	regex_array[3] = getval("product");
	regex_array[4] = getval("serial");
	regex_array[5] = getval("bus");

	ret = USBNewRegexMatcher(&regex_matcher, regex_array, REG_ICASE | REG_EXTENDED);
	switch (ret)
	{
	case -1:
		fatal_with_errno(EXIT_FAILURE, "USBNewRegexMatcher");
	case 0:
		break;	/* all is well */
	default:
		fatalx(EXIT_FAILURE, "invalid regular expression: %s", regex_array[ret]);
	}

	/* link the matchers */
	regex_matcher->next = &device_matcher;

	ret = usb->open(&udev, &usbdevice, regex_matcher, NULL);
	if (ret < 0) {
		fatalx(EXIT_FAILURE,
			"No supported devices found. Please check your device availability with 'lsusb'\n"
			"and make sure you have an up-to-date version of NUT.\n");
	}

	/* create a new matcher for later reopening */
	ret = USBNewExactMatcher(&reopen_matcher, &usbdevice);
	if (ret) {
		fatal_with_errno(EXIT_FAILURE, "USBNewExactMatcher");
	}

	/* link the matchers */
	reopen_matcher->next = regex_matcher;

	dstate_setinfo(UPS_VENDORID, "%04x", usbdevice.VendorID);
	dstate_setinfo(UPS_PRODUCTID, "%04x", usbdevice.ProductID);
	dstate_setinfo(UPS_MFR, "%s", usbdevice.Vendor);
	dstate_setinfo(UPS_MODEL, "%s", usbdevice.Product);

	/* check for language ID workaround (#2) */
	if (langid_fix != -1) {
		/* Future improvement:
		 *   Asking for the zero'th index is special - it returns a string
		 *   descriptor that contains all the language IDs supported by the
		 *   device. Typically there aren't many - often only one. The
		 *   language IDs are 16 bit numbers, and they start at the third byte
		 *   in the descriptor. See USB 2.0 specification, section 9.6.7, for
		 *   more information on this.
		 * This should allow automatic application of the workaround */
		ret = usb_get_string(udev, 0, 0, tbuf, sizeof(tbuf));
		if (ret >= 4) {
			langid = tbuf[2] | (tbuf[3] << 8);
			upsdebugx(1, "First supported language ID: 0x%x (please report to the NUT maintainer!)", langid);
		}
	}
	omron_initups();
}

void upsdrv_initinfo(void)
{
	omron_initinfo();
}

void upsdrv_cleanup(void)
{
	usb->close(udev);
	USBFreeExactMatcher(reopen_matcher);
	USBFreeRegexMatcher(regex_matcher);
	free(usbdevice.Vendor);
	free(usbdevice.Product);
	free(usbdevice.Serial);
	free(usbdevice.Bus);
}
