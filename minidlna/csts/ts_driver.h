#ifndef _TS_DRIVER_H_
#define _TS_DRIVER_H_

#define u8 			unsigned char
#define u16 		unsigned short
#define u32 		unsigned int
#define cs_uint8	unsigned char
#define cs_uint16	unsigned short
#define cs_uint32	unsigned int
#define cs_int32	int

cs_int32 g2_ts_mmap_init(void);
cs_int32 g2_ts_mmap_exit(cs_int32 fd);
cs_int32 g2_ts_replace_pid(cs_uint8	ch_id, cs_uint16 org_pid, cs_uint16 new_pid);
cs_int32 g2_ts_duplicate_pid(cs_uint8 ch_id, cs_uint16 pid, cs_uint8 qid, cs_uint16 new_pid);
cs_int32 g2_ts_add_pid(cs_uint8	ch_id, cs_uint16 pid);
cs_int32 g2_ts_del_pid(cs_uint8	ch_id, cs_uint16 pid);
cs_int32 g2_ts_flush_all_pid(cs_uint8 ch_id);
cs_uint32 g2_ts_get_statistic(cs_uint8 ch_id);
cs_int32 g2_ts_clear_statistic(cs_uint8 ch_id);
cs_int32 g2_ts_set_length(cs_uint8 ch_id, cs_uint16 length);
cs_int32 g2_ts_set_rx_enable(cs_uint8 ch_id, cs_uint8 mode);























#endif
