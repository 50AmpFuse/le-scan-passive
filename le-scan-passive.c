// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2000-2001  Qualcomm Incorporated
 *  Copyright (C) 2002-2003  Maxim Krasnyansky <maxk@qualcomm.com>
 *  Copyright (C) 2002-2010  Marcel Holtmann <marcel@holtmann.org>
 *  add option hex-adv pvvx 14.04.2021
 *  2025-06, 50AmpFuse:
 *	- stripped down hcitool code to bare minimum for passive LE scanning
 *	- renamed to le-scan-passive
 *	- added some output formatting and colors for convenience
 *
 */

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <signal.h>

#include "lib/bluetooth.h"
#include "lib/hci.h"
#include "lib/hci_lib.h"

#define ANSI_COL_FG_BLK   "\e[30m"
#define ANSI_COL_FG_RED   "\e[31m"
#define ANSI_COL_FG_GRN   "\e[32m"
#define ANSI_COL_FG_YEL   "\e[33m"
#define ANSI_COL_FG_BLU   "\e[34m"
#define ANSI_COL_FG_MAG   "\e[35m"
#define ANSI_COL_FG_CYN   "\e[36m"
#define ANSI_COL_FG_WHT   "\e[37m"

#define ANSI_COL_BG_BLK   "\e[40m"
#define ANSI_COL_BG_RED   "\e[41m"
#define ANSI_COL_BG_GRN   "\e[42m"
#define ANSI_COL_BG_YEL   "\e[43m"
#define ANSI_COL_BG_BLU   "\e[44m"
#define ANSI_COL_BG_MAG   "\e[45m"
#define ANSI_COL_BG_CYN   "\e[46m"
#define ANSI_COL_BG_WHT   "\e[47m"

#define ANSI_RESET           "\e[0m"
#define ANSI_BRIGHT          "\e[1m"
#define ANSI_DIM             "\e[2m"
#define ANSI_UNDERLINE       "\e[3m"
#define ANSI_BLINK           "\e[4m"
#define ANSI_REVERSE         "\e[7m"
#define ANSI_HIDDEN          "\e[8m"

static volatile int signal_received = 0;

static void sigint_handler(int sig)
{
	signal_received = sig;
}

static void parse_elements(uint8_t *eir, size_t eir_len, char *buf, size_t buf_len, int8_t rssi)
{
	size_t offset;

	offset = 0;
	while (offset < eir_len) {
		uint8_t field_len = eir[0];
		uint8_t field_type = eir[1];
		size_t name_len;

		/* Check for the end of EIR */
		if (field_len == 0)
			break;

		if (offset + field_len > eir_len)
			goto failed;

		printf(ANSI_BRIGHT ANSI_COL_FG_GRN);
		switch (field_type) {
			case 0x01:
				printf("<FLAGS>");
				break;
			case 0x02:
				printf("<UUID16_SOME>");
				break;
			case 0x03:
				printf("<UUID16_ALL>");
				break;
			case 0x04:
				printf("<EIR_UUID32_SOME>");
				break;
			case 0x05:
				printf("<EIR_UUID32_ALL>");
				break;
			case 0x06:
				printf("<EIR_UUID128_SOME>");
				break;
			case 0x07:
				printf("<EIR_UUID128_ALL>");
				break;
			case 0x08:
				printf("<NAME_SHORT>");
				break;
			case 0x09:
				printf("<NAME_COMPLETE>");
				break;
			case 0x0A:
				printf("<TX_POWER>");
				break;
			case 0x10:
				printf("<DEVICE_ID>");
				break;
			case 0x16:
				printf("<SERVICE_DATA_UUID16>");
				break;
			case 0x19:
				printf("<APPEARANCE>");
				break;
			case 0x20:
				printf("<SERVICE_DATA_UUID32>");
				break;
			case 0x21:
				printf("<SERVICE_DATA_UUID128>");
				break;
			case 0xFF:
				printf("<MANUFACTURER_SPECIFIC_DATA>");
				break;
			default:
				printf("<%.2X>",field_type);

		}
		printf(ANSI_RESET);

		if (field_type == 0x08 || field_type == 0x09)
		{
			printf(ANSI_REVERSE);
			for (uint8_t i=0; i<field_len-1; ++i)
				printf("%c",eir[2+i]);
			printf(ANSI_RESET);
		}
		else
		{
			for (uint8_t i=0; i<field_len-1; ++i)
				printf("%.2X",eir[2+i]);
		}
		offset += field_len + 1;
		eir += field_len + 1;
	}

failed:
}

static int print_advertising_devices(int dd, uint8_t filter_type)
{
	unsigned char buf[HCI_MAX_EVENT_SIZE], *ptr;
	struct hci_filter nf, of;
	struct sigaction sa;
	socklen_t olen;
	int len;

	olen = sizeof(of);
	if (getsockopt(dd, SOL_HCI, HCI_FILTER, &of, &olen) < 0) {
		fprintf(stderr,"Could not get socket options\n");
		return -1;
	}

	hci_filter_clear(&nf);
	hci_filter_set_ptype(HCI_EVENT_PKT, &nf);
	hci_filter_set_event(EVT_LE_META_EVENT, &nf);

	if (setsockopt(dd, SOL_HCI, HCI_FILTER, &nf, sizeof(nf)) < 0) {
		fprintf(stderr, "Could not set socket options\n");
		return -1;
	}

	memset(&sa, 0, sizeof(sa));
	sa.sa_flags = SA_NOCLDSTOP;
	sa.sa_handler = sigint_handler;
	sigaction(SIGINT, &sa, NULL);

	while (1) {
		evt_le_meta_event *meta;
		le_advertising_info *info;
		char addr[18];
		char adv[32*2+4];

		while ((len = read(dd, buf, sizeof(buf))) < 0) {
			if (errno == EINTR && signal_received == SIGINT) {
				printf("\nSIGINT\n");
				len = 0;
				goto done;
			}

			if (errno == EAGAIN || errno == EINTR)
				continue;
			goto done;
		}

		ptr = buf + (1 + HCI_EVENT_HDR_SIZE);
		len -= (1 + HCI_EVENT_HDR_SIZE);

		meta = (void *) ptr;

		if (meta->subevent != 0x02)
			goto done;

		/* Ignoring multiple reports */
		info = (le_advertising_info *) (meta->data + 1);

		ba2str(&info->bdaddr, addr);
		int8_t rssi = info->data[info->length];
		printf("%s %4d ", addr, rssi);

		memset(adv, 0, sizeof(adv));
		parse_elements(info->data, info->length, adv, sizeof(adv) - 1, rssi);
		printf("\n");

	}

done:
	setsockopt(dd, SOL_HCI, HCI_FILTER, &of, sizeof(of));

	if (len < 0)
		return -1;

	return 0;
}


static void cmd_lescan(int dev_id)
{
	int err, opt, dd;
	uint8_t own_type = LE_PUBLIC_ADDRESS;
	uint8_t scan_type = 0x01;
	uint8_t filter_type = 0;
	uint8_t filter_policy = 0x00;
	uint16_t interval = htobs(0x0010);
	uint16_t window = htobs(0x0010);
	uint8_t filter_dup = 0x01;

	filter_dup = 0x00;
	scan_type = 0x00; /* Passive */

	if (dev_id < 0)
		dev_id = hci_get_route(NULL);

	dd = hci_open_dev(dev_id);
	if (dd < 0) {
		perror("Could not open device");
		exit(1);
	}

	err = hci_le_set_scan_parameters(dd, scan_type, interval, window,
						own_type, filter_policy, 10000);
	if (err < 0) {
		perror("Set scan parameters failed");
		exit(1);
	}

	err = hci_le_set_scan_enable(dd, 0x01, filter_dup, 10000);
	if (err < 0) {
		perror("Enable scan failed");
		exit(1);
	}


	err = print_advertising_devices(dd, filter_type);

	if (err < 0) {
		perror("Could not receive advertising events");
		exit(1);
	}

	err = hci_le_set_scan_enable(dd, 0x00, filter_dup, 10000);
	if (err < 0) {
		perror("Disable scan failed");
		exit(1);
	}

	hci_close_dev(dd);
}

struct option main_options[] = {
	{ "device",	0, 0, 'i' },
	{ 0, 0, 0, 0 }
};

int main(int argc, char *argv[])
{
	int opt, i, dev_id = -1;
	bdaddr_t ba;

	while ((opt=getopt_long(argc, argv, "+i:h", main_options, NULL)) != -1)
	{
		if (opt=='i')
		{
			dev_id = hci_devid(optarg);
			if (dev_id < 0)
			{
				perror("Invalid device");
				exit(1);
			}
		}
	}


	if (dev_id != -1 && hci_devba(dev_id, &ba) < 0) {
		perror("Device is not available");
		exit(1);
	}

	if (setvbuf(stdout, NULL, _IONBF, 0) != 0) {
		perror("Failed to set buffer");
   	}

	cmd_lescan(dev_id);

	return 0;
}
