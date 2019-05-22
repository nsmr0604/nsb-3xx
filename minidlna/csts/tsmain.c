/*
 *  Simple MPEG parser to achieve network/service information.
 *
 *  refered standards:
 *
 *    ETSI EN 300 468
 *    ETSI TR 101 211
 *    ETSI ETR 211
 *    ITU-T H.222.0
 *
 * 2005-05-10 - Basic ATSC PSIP parsing support added
 *    ATSC Standard Revision B (A65/B)
 *
 * Thanks to Sean Device from Triveni for providing access to ATSC signals
 *    and to Kevin Fowlks for his independent ATSC scanning tool.
 *
 * Please contribute: It is possible that some descriptors for ATSC are
 *        not parsed yet and thus the result won't be complete.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <assert.h>
#include <glob.h>
#include <ctype.h>

#include "scan.h"
#include "list.h"
#include "ts_driver.h"

#include <linux/dvb/dmx.h>
#define MAX_PES_SIZE   		(31584) //(32712)//(188*348)


struct section_buf {
	struct list_head list;
	const char *dmx_devname;
	unsigned int run_once  : 1;
	unsigned int segmented : 1;	/* segmented by table_id_ext */
	int fd;
	int pid;
	int table_id;
	int table_id_ext;
	int section_version_number;
	uint8_t section_done[32];
	int sectionfilter_done;
	unsigned char buf[1024];
	time_t timeout;
	time_t start_time;
	time_t running_time;
	struct section_buf *next_seg;	/* this is used to handle
					 * segmented tables (like NIT-other)
					 */
};

void usage(void)
{
	fprintf(stderr, "Usage: tsmain\n");
	fprintf(stderr, "\t tsmain 1: DMX_SET_FILTER\n");
    fprintf(stderr, "\t tsmain 2: DMX_SET_PES_FILTER\n");
	fprintf(stderr, "\t tsmain 3: cs_ts_channel_scan\n");
    fprintf(stderr, "\t tsmain 4: cs_ts_set_tv_channel (channel)\n");
	fprintf(stderr, "\t-5 \n");
    fprintf(stderr, "\t-6 \n");
	fprintf(stderr, "\t-7 \n");
    fprintf(stderr, "\t-8 \n");
	exit(1);
}

void setup_filter (struct section_buf* s, const char *dmx_devname,
			  int pid, int tid, int tid_ext,
			  int run_once, int segmented, int timeout)
{

	memset (s, 0, sizeof(struct section_buf));

	s->fd = -1;
	s->dmx_devname = dmx_devname;
	s->pid = pid;
	s->table_id = tid;

	s->run_once = run_once;
	s->segmented = segmented;

	s->timeout = timeout;

	s->table_id_ext = tid_ext;
	s->section_version_number = -1;

}

int setup_pesfilter(int dmxfd, int pid, int pes_type, int dvr)
{
    struct dmx_pes_filter_params pesfilter;

    /* ignore this pid to allow radio services */
    if (pid < 0 ||
	pid >= 0x1fff ||
	(pid == 0 && pes_type != DMX_PES_OTHER))
	return 0;

    if (dvr) {
    /*
	int buffersize = 64 * 1024;
	if (ioctl(dmxfd, DMX_SET_BUFFER_SIZE, buffersize) == -1)
	    perror("DMX_SET_BUFFER_SIZE failed");
	   */ 
    }
      
    pesfilter.pid = pid;
    pesfilter.input = DMX_IN_FRONTEND;
    pesfilter.output = dvr ? DMX_OUT_TAP : DMX_OUT_DECODER;
    pesfilter.pes_type = pes_type;
    pesfilter.flags = DMX_IMMEDIATE_START;

    if (ioctl(dmxfd, DMX_SET_PES_FILTER, &pesfilter) == -1) {
	fprintf(stderr, "DMX_SET_PES_FILTER failed "
	"(PID = 0x%04x): %d %m\n", pid, errno);
	return -1;
    }

    return 0;
}

int start_filter2 (struct section_buf* s)
{
	struct dmx_sct_filter_params f;

	if ((s->fd = open (s->dmx_devname, O_RDWR | O_NONBLOCK)) < 0)
		goto err0;
  printf("%s: open demux %s, fd %d\n", __func__, s->dmx_devname, s->fd);
	verbosedebug("start filter pid 0x%04x table_id 0x%02x\n", s->pid, s->table_id);
  ioctl(s->fd, DMX_SET_BUFFER_SIZE, (188*1024));
	memset(&f, 0, sizeof(f));

	f.pid = (uint16_t) s->pid;

	if (s->table_id < 0x100 && s->table_id > 0) {
		f.filter.filter[0] = (uint8_t) s->table_id;
		f.filter.mask[0]   = 0xff;
	}
	if (s->table_id_ext < 0x10000 && s->table_id_ext > 0) {
		f.filter.filter[1] = (uint8_t) ((s->table_id_ext >> 8) & 0xff);
		f.filter.filter[2] = (uint8_t) (s->table_id_ext & 0xff);
		f.filter.mask[1] = 0xff;
		f.filter.mask[2] = 0xff;
	}

	f.timeout = 0;
	f.flags = DMX_IMMEDIATE_START | DMX_CHECK_CRC;

	if (ioctl(s->fd, DMX_SET_FILTER, &f) == -1) {
		errorn ("ioctl DMX_SET_FILTER failed");
		goto err1;
	}

	s->sectionfilter_done = 0;
	return 0;

err1:
	ioctl (s->fd, DMX_STOP);
	close (s->fd);
err0:
	return -1;
}


int main (int argc, char **argv)
{
	const char *scan_file_name = NULL;
	int dvr_fd, i, rd, dmxfd[12];
	struct list_head *slist, *p;
	struct service *s;
	CS_TS_TV_DEV_INFO *pDevInfo;
	unsigned char buf[MAX_PES_SIZE];
	CS_TS_CH_INFO *chInfo;
	struct section_buf s0,s1,s2,s3;
	int count;
	int timeOut = 300;
	unsigned long	pid=0;


	if (argc < 2)
		usage();

	cs_ts_dev_init();
	log_init(NULL, "general,artwork,database,inotify,scanner,metadata,http,ssdp,tivo=warn", E_TS_DBG_3);

	pDevInfo = cs_ts_get_tv_dev_info();


	if(strcmp(argv[1], "1")== 0)
	{
		//setup_filter(&s1, pDevInfo->demux_data_dev[0], 0x0, 0x0, -1, 1, 0, 5); /* PAT */
		//setup_filter(&s3, pDevInfo->demux_data_dev[0], 0x1ffb, 0xc7, -1, 1, 0, 5); /* PAT */
		setup_filter(&s0, pDevInfo->demux_data_dev[6], 0x0, 0x0, -1, 1, 0, 5); /* PAT */
		setup_filter(&s2, pDevInfo->demux_data_dev[6], 0x1ffb, 0xc9, -1, 1, 0, 5); /* PAT */
    //ioctl(s1.fd, DMX_SET_BUFFER_SIZE, (188*1024));
    //ioctl(s0.fd, DMX_SET_BUFFER_SIZE, (188*1024));
		//start_filter2(&s1);
		start_filter2(&s0);
		//start_filter2(&s3);
		start_filter2(&s2);
		while(timeOut--)
		{
			count = read (s0.fd, s0.buf, 188);
			if(count > 0)
			{
				break;
      		}
			usleep(100000);
		}
		while(timeOut--)
		{
			count = read (s2.fd, s2.buf, 188);
			if(count > 0)
			{
				break;
      }
			usleep(100000);
		}
#ifndef G2_SPEC_DEMUX
			g2_ts_flush_all_pid(WORKING_DEMUX_ID+CS_TS_DEMUX_CTRL_DEV_BEGIN);
#endif	
				printf("%d: ================================== read count %d from fd %d\n", timeOut, count, s0.fd);
				printf("%02x %02x %02x %02x %02x %02x %02x %02x\n", s0.buf[0],s0.buf[1],s0.buf[2],s0.buf[3],s0.buf[4],s0.buf[5],s0.buf[6],s0.buf[7]);
				printf("%d: ================================== read count %d from fd %d\n", timeOut, count, s2.fd);
				printf("%02x %02x %02x %02x %02x %02x %02x %02x\n", s2.buf[0],s2.buf[1],s2.buf[2],s2.buf[3],s2.buf[4],s2.buf[5],s2.buf[6],s2.buf[7]);


	}
	else if(strcmp(argv[1], "2")== 0)
	{
			if ((dmxfd[0] = open ("/dev/dvb/adapter0/demux0", O_RDWR)) < 0)
			{
				printf("open demux fail %s\n", "/dev/dvb/adapter0/demux0");
				return -1;
			}
			if ((dmxfd[6] = open ("/dev/dvb/adapter0/demux6", O_RDWR)) < 0)
			{
				printf("open demux fail %s\n", "/dev/dvb/adapter0/demux6");
				return -1;
			}
      ioctl(dmxfd[0], DMX_SET_BUFFER_SIZE, (188*1024));
      ioctl(dmxfd[6], DMX_SET_BUFFER_SIZE, (188*1024));
			setup_pesfilter(dmxfd[0], 0, DMX_PES_OTHER, 1);
			setup_pesfilter(dmxfd[0], 0x1ffb, DMX_PES_OTHER, 1);
			setup_pesfilter(dmxfd[6], 0, DMX_PES_OTHER, 1);
			setup_pesfilter(dmxfd[6], 0x1ffb, DMX_PES_OTHER, 1);
			//start_filter2(dmxfd[0], 0x1ffb);
			//start_filter2(dmxfd, 0);
			while(timeOut--)
			{
				count = read (dmxfd[6], buf, 188);//MAX_PES_SIZE);
				if(count > 0)
				{
				  printf("%d: read pes filter %d\n", timeOut, count);
				  printf("%02x %02x %02x %02x %02x %02x %02x %02x\n", buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6],buf[7]);
				  break;
				}
				usleep(100000);
			}
#ifndef G2_SPEC_DEMUX
				g2_ts_flush_all_pid(WORKING_DEMUX_ID+CS_TS_DEMUX_CTRL_DEV_BEGIN);
#endif	
	}
	else if(strcmp(argv[1], "3")== 0)
	{
		cs_ts_channel_scan();
	}
	else if(strcmp(argv[1], "4")== 0 && argv[2])
	{
		int sendfh;
		int size = 2*1024*1024;
		char saveFileName[]="/tmp/test.ts";
		char dataBuf[31584];
		int tout=100;
		FILE *outFile;
		
		sendfh = cs_ts_set_tv_channel(argv[2]);
		outFile = fopen(saveFileName, "wb");
		
		while(size && tout--)
		{
			size -= read(sendfh, dataBuf, sizeof(dataBuf));
			write(outFile, dataBuf, sizeof(dataBuf));
			printf("%s: remain size %d\n", __func__, size);
		}
		fclose(outFile);
	}

	//setup_filter(&s1, demux_devname, 0x1ffb, 0xc8, -1, 1, 0, 5); /* terrestrial VCT */
	//add_filter(&s1);


	cs_ts_dev_free();
	
return 0;
}
