#ifndef __SCAN_H__
#define __SCAN_H__

#include <stdio.h>
#include <errno.h>
#include <linux/dvb/frontend.h>
#include "list.h"
#include "log.h"

/*
#define fatal(fmt, arg...) do { log_err(E_FATAL, L_TSSCAN, __FILE__, __LINE__, fmt, ##arg); exit(1); } while(0)
#define error(fmt, arg...) do{ log_err(E_ERROR, L_TSSCAN, __FILE__, __LINE__, fmt, ##arg); }while(0)
#define errorn(fmt, arg...) do{ log_err(E_ERROR, L_TSSCAN, __FILE__, __LINE__, fmt, ##arg); }while(0)
#define warning(fmt, arg...) do{ log_err(E_WARN, L_TSSCAN, __FILE__, __LINE__, fmt, ##arg); }while(0)
#define info(fmt, arg...) do{ log_err(E_INFO, L_TSSCAN, __FILE__, __LINE__, fmt, ##arg); }while(0)
#define verbose(fmt, arg...) do{ log_err(E_INFO, L_TSSCAN, __FILE__, __LINE__, fmt, ##arg); }while(0)
#define moreverbose(fmt, arg...) do{ log_err(E_INFO, L_TSSCAN, __FILE__, __LINE__, fmt, ##arg); }while(0)
#define debug(fmt, arg...) do{ log_err(E_DEBUG, L_TSSCAN, __FILE__, __LINE__, fmt, ##arg); }while(0)
#define verbosedebug(fmt, arg...) do{ log_err(E_DEBUG, L_TSSCAN, __FILE__, __LINE__, fmt, ##arg); }while(0)
*/
#define fatal(fmt, arg...) do { log_err(E_FATAL, L_TSSCAN, __FILE__, __LINE__, fmt, ##arg); exit(1); } while(0)
#define error(fmt, arg...) do{ log_err(E_ERROR, L_TSSCAN, __FILE__, __LINE__, fmt, ##arg); }while(0)
#define errorn(fmt, arg...) do{ log_err(E_ERROR, L_TSSCAN, __FILE__, __LINE__, fmt, ##arg); }while(0)
#define warning(fmt, arg...) do{ log_err(E_WARN, L_TSSCAN, __FILE__, __LINE__, fmt, ##arg); }while(0)
#define info(fmt, arg...) do{ log_err(E_DEBUG, L_TSSCAN, __FILE__, __LINE__, fmt, ##arg); }while(0)
#define verbose(fmt, arg...) do{ log_err(E_DEBUG, L_TSSCAN, __FILE__, __LINE__, fmt, ##arg); }while(0)
#define moreverbose(fmt, arg...) do{ log_err(E_DEBUG, L_TSSCAN, __FILE__, __LINE__, fmt, ##arg); }while(0)
#define debug(fmt, arg...) do{ log_err(E_DEBUG, L_TSSCAN, __FILE__, __LINE__, fmt, ##arg); }while(0)
#define verbosedebug(fmt, arg...) do{ log_err(E_DEBUG, L_TSSCAN, __FILE__, __LINE__, fmt, ##arg); }while(0)

#define AUDIO_CHAN_MAX (32)
#define CA_SYSTEM_ID_MAX (16)
#define WORKING_DEMUX_ID 0
#define CS_TS_DEV_NAME_LEN       80
#define CS_TS_MAX_DEMUX_DEVICE   1/*6 vincent debug, for x86 and DVB tuner*/
#define CS_TS_DEMUX_DATA_DEV_BEGIN       0    /* data demux number : 0 ~5  */
#define CS_TS_DEMUX_CTRL_DEV_BEGIN       6    /* ctrl demux number : 6 ~11  */ 
#define FTOK_NAME	"/tmp"
#define FTOK_ID_TV	5
#define FTOK_ID_CH	6
#define LIVE_DIR_ID "4"
#define INTERNET_VIDEO_DIR_ID "5"
#define NBR_CHANNEL_INFO 128
#define CS_TS_PROGRAM_NAME_LENGTH 256

typedef unsigned char      uint8_t;
typedef unsigned short int uint16_t;
typedef unsigned int        uint32_t;

enum running_mode {
	RM_NOT_RUNNING = 0x01,
	RM_STARTS_SOON = 0x02,
	RM_PAUSING     = 0x03,
	RM_RUNNING     = 0x04
};

struct service {
	struct list_head list;
	int transport_stream_id;
	int service_id;
	char *provider_name;
	char *service_name;
	uint16_t pmt_pid;
	uint16_t pcr_pid;
	uint16_t video_pid;
	uint16_t audio_pid[AUDIO_CHAN_MAX];
	char audio_lang[AUDIO_CHAN_MAX][4];
	int audio_num;
	uint16_t ca_id[CA_SYSTEM_ID_MAX];
	int ca_num;
	uint16_t teletext_pid;
	uint16_t subtitling_pid;
	uint16_t ac3_pid;
	unsigned int type         : 8;
	unsigned int scrambled	  : 1;
	enum running_mode running;
	void *priv;
	int channel_num;
	int source_id;
	unsigned char current_program_name[CS_TS_PROGRAM_NAME_LENGTH];
};

typedef struct _CS_TS_CH_INFO {
	unsigned int  ts_pid;
    unsigned int  pmt_pid;
    unsigned int  video_pid;
    unsigned int  audio_pid[AUDIO_CHAN_MAX];
    unsigned char  prog_short_name[CS_TS_PROGRAM_NAME_LENGTH];
    unsigned char  prog_path[32];
	struct dvb_frontend_parameters tuner_param;
}CS_TS_CH_INFO, *PCS_TS_CH_INFO;

typedef struct _CS_TS_TV_DEV_INFO {
	int fd_frontend[CS_TS_MAX_DEMUX_DEVICE];
	int fd_dvr[CS_TS_MAX_DEMUX_DEVICE];
	int fd_ts_driver_mmap;
	char dvr_dev[CS_TS_MAX_DEMUX_DEVICE][CS_TS_DEV_NAME_LEN];
	char demux_data_dev[CS_TS_MAX_DEMUX_DEVICE][CS_TS_DEV_NAME_LEN];
	char demux_ctrl_dev[CS_TS_MAX_DEMUX_DEVICE][CS_TS_DEV_NAME_LEN];
	char frontend_dev[CS_TS_MAX_DEMUX_DEVICE][CS_TS_DEV_NAME_LEN];
	CS_TS_CH_INFO *pChInfo;
}CS_TS_TV_DEV_INFO, *PCS_TS_TV_DEV_INFO;

void scan_network (int, const char *);
void dump_lists (void);
int cs_ts_set_tv_channel(char *);

CS_TS_CH_INFO *cs_ts_channel_scan(void);
int cs_ts_dev_init();
void cs_ts_dev_free();
CS_TS_TV_DEV_INFO *cs_ts_get_tv_dev_info(void);
void cs_ts_show_debug_info(int);
int cs_ts_release_tv_channel(int fd);

#endif
