/* vi: set sw=4 ts=4: */
/*
 * udhcp client
 *
 * Russ Dill <Russ.Dill@asu.edu> July 2001
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <syslog.h>
/* Override ENABLE_FEATURE_PIDFILE - ifupdown needs our pidfile to always exist */
#define WANT_PIDFILE 1
#include "common.h"
#include "dhcpd.h"
#include "dhcpc.h"

#include <asm/types.h>
#if (defined(__GLIBC__) && __GLIBC__ >= 2 && __GLIBC_MINOR__ >= 1) || defined(_NEWLIB_VERSION)
# include <netpacket/packet.h>
# include <net/ethernet.h>
#else
# include <linux/if_packet.h>
# include <linux/if_ether.h>
#endif
#include <linux/filter.h>

/* struct client_config_t client_config is in bb_common_bufsiz1 */


/*** Script execution code ***/

/* get a rough idea of how long an option will be (rounding up...) */
static const uint8_t len_of_option_as_string[] = {
	[OPTION_IP              ] = sizeof("255.255.255.255 "),
	[OPTION_IP_PAIR         ] = sizeof("255.255.255.255 ") * 2,
	[OPTION_STATIC_ROUTES   ] = sizeof("255.255.255.255/32 255.255.255.255 "),
	[OPTION_6RD             ] = sizeof("32 128 FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF 255.255.255.255 "),
	[OPTION_STRING          ] = 1,
#if ENABLE_FEATURE_UDHCP_RFC3397
	[OPTION_DNS_STRING      ] = 1, /* unused */
	/* Hmmm, this severely overestimates size if SIP_SERVERS option
	 * is in domain name form: N-byte option in binary form
	 * mallocs ~16*N bytes. But it is freed almost at once.
	 */
	[OPTION_SIP_SERVERS     ] = sizeof("255.255.255.255 "),
#endif
//	[OPTION_BOOLEAN         ] = sizeof("yes "),
	[OPTION_U8              ] = sizeof("255 "),
	[OPTION_U16             ] = sizeof("65535 "),
//	[OPTION_S16             ] = sizeof("-32768 "),
	[OPTION_U32             ] = sizeof("4294967295 "),
	[OPTION_S32             ] = sizeof("-2147483684 "),
};

/* note: ip is a pointer to an IP in network order, possibly misaliged */
static int sprint_nip(char *dest, const char *pre, const uint8_t *ip)
{
	return sprintf(dest, "%s%u.%u.%u.%u", pre, ip[0], ip[1], ip[2], ip[3]);
}

static int sprint_nip6(char *dest, const char *pre, const uint8_t *ip)
{
	int len = 0;
	int off;
	uint16_t word;

	len += sprintf(dest, "%s", pre);

	for (off = 0; off < 16; off += 2)
	{
		move_from_unaligned16(word, &ip[off]);
		len += sprintf(dest+len, "%s%04X", off ? ":" : "", htons(word));
	}

	return len;
}

/* really simple implementation, just count the bits */
static int mton(uint32_t mask)
{
	int i = 0;
	mask = ntohl(mask); /* 111110000-like bit pattern */
	while (mask) {
		i++;
		mask <<= 1;
	}
	return i;
}

#ifdef FEATURE_OP60_OP125_SUPPORT
 
#define MAX_IP_LEN 16
#define IP_CONFIG_PATH "/var/run"
#define PID_CFG_PATH   "/var/run/netconfig"
#define PID_CONFIG     "/var/run/netconfig/%s/pid"

/* IP address*/
#define NIPQUAD(addr) \
    ((unsigned char *)&addr)[0], \
    ((unsigned char *)&addr)[1], \
    ((unsigned char *)&addr)[2], \
    ((unsigned char *)&addr)[3]
#define NIPQUAD_FMT "%u.%u.%u.%u"

/* MAC address */
#define NMACQUAD(addr) \
    ((unsigned char *)addr)[0], \
    ((unsigned char *)addr)[1], \
    ((unsigned char *)addr)[2], \
    ((unsigned char *)addr)[3], \
    ((unsigned char *)addr)[4], \
    ((unsigned char *)addr)[5]
#define NMACQUAD_FMT "%02x:%02x:%02x:%02x:%02x:%02x"

typedef struct {
    uint8_t sub_code;//mode 1/2
    uint8_t sub_len;//total len including code(1bytes)+len(1bytes) + others
    uint8_t data[0];
}__attribute__ ((__packed__))dh4op125_subopt_t; 

typedef struct {
    uint32_t enterprise_number;//
    uint8_t data_len;//length of data below
    uint8_t data[0];//dh4op125_subopt_t
}__attribute__ ((__packed__))dh4op125_opt_t; 


typedef struct {
    uint8_t type;//mode 1/2
    uint8_t len;//total len including code(1bytes)+len(1bytes) + others
    uint8_t data[0];//125|len|dh4op125_opt_t
}__attribute__ ((__packed__))dh4op125_t; 

typedef struct {
	char dev[MAX_IP_LEN];
	char ip[MAX_IP_LEN];
	char netmask[MAX_IP_LEN];
	char gateway[MAX_IP_LEN];
	char dns1[MAX_IP_LEN];
	char dns2[MAX_IP_LEN];
}dhcpc_ip_info_t;


dhcpc_ip_info_t dhcp_ipinfo;

int dh4op125_subopt_cmp(uint8_t len, dh4op125_subopt_t *src, dh4op125_subopt_t *dst)
{
    uint8_t offset = 0, start = 0, found = 0; 
    dh4op125_subopt_t *tmp;
    
    if(!src || !dst) {
        //printf("%s: %d\n\r", __func__, __LINE__);
        return -1;
    }

    while(offset < len) {
        found = 0;
        start = 0;
        src = (dh4op125_subopt_t *)((uint8_t*)src + offset);
        //printf("%s: src : sub code %d, sub len %d, %d\n\r", __func__, src->sub_code , src->sub_len ,__LINE__);
        while(start < len) {
            tmp = (dh4op125_subopt_t *)((uint8_t*)dst + start);
            //printf("%s: dst: sub code %d, sub len %d, %d\n\r", __func__, tmp->sub_code , tmp->sub_len ,__LINE__);
            if(src->sub_code == tmp->sub_code &&
                src->sub_len == tmp->sub_len) {
                found = 1;
                break;
            }
            start += (OPT_DATA + tmp->sub_len);
        }
        if(!found) {
            //printf("%s: %d\n\r", __func__, __LINE__);
            goto NO_MATCH;
        }
        if(memcmp(src->data, tmp->data, src->sub_len)) {
            //printf("%s: %d\n\r", __func__, __LINE__);
            goto NO_MATCH;
        }
        offset += (OPT_DATA + src->sub_len);
    }

    return 0;
    
NO_MATCH:
    return 1;
}

/*
keystr: opcode(125)|oplen|payload

*/
static int check_message_option125(const dh4op125_t *op125, const uint8_t *keystr)
{
    uint8_t offset = 0, start = 0, found = 0, end = 0; 
    uint16_t enterp = 0;
    dh4op125_subopt_t *subopt = NULL;
    if(!op125 || !keystr) 
        return -1;

    if(op125->type == 2) {
        return memcmp(op125->data, keystr,  op125->len);
    }
    //total length comparing
    if(keystr[OPT_LEN] != op125->len - OPT_DATA) {
        //printf("%s: %d\n\r", __func__, __LINE__);
        goto NO_MATCH;
    }

    offset = OPT_DATA;
#if 1
{
    dh4op125_opt_t *src = (dh4op125_opt_t *)&(op125->data[offset]);
    dh4op125_opt_t *dst = (dh4op125_opt_t *)&keystr[OPT_DATA];
    dh4op125_subopt_t *s_src = (dh4op125_subopt_t *)src->data;
    dh4op125_subopt_t *s_dst = (dh4op125_subopt_t *)dst->data;
    
    if(src->enterprise_number != ntohl(dst->enterprise_number)) {
        bb_error_msg("%s: opt->enterprise_number 0x%x,  %d not match\n\r", __func__, src->enterprise_number, dst->enterprise_number);
        goto NO_MATCH;
    }
    if(src->data_len != dst->data_len) {
        bb_error_msg("%s: opt->data_len 0x%x, %d, not match\n\r", __func__, src->data_len, dst->data_len);
        goto NO_MATCH;
    }
    if(s_src->sub_code != s_dst->sub_code) {
        bb_error_msg("%s: sub_code %d/%d  not match\n\r", __func__, s_src->sub_code, s_dst->sub_code);
        goto NO_MATCH;
    }
    if(s_src->sub_len != s_dst->sub_len) {
        bb_error_msg("%s: sub_code %d/%d  not match\n\r", __func__, s_src->sub_len, s_dst->sub_len);
        goto NO_MATCH;
    }
    if(memcmp(s_src->data, s_dst->data, s_src->sub_len) != 0) {
        bb_error_msg("%s: sub_data  not match\n\r", __func__);
        goto NO_MATCH;
    }
}
    bb_error_msg("%s: option matched \n\r", __func__);
    return 0;
#else    
    while(offset < op125->len) {
        dh4op125_opt_t *opt = (dh4op125_opt_t *)&(op125->data[offset]);
        //1comparing enterprise number and find the data block
        found = 0;
        start = OPT_DATA;
        //printf("%s: opt->enterprise_number 0x%x,offset %d,  %d\n\r", __func__, opt->enterprise_number, __LINE__);
        while(start < keystr[OPT_LEN]) {
            memcpy(&enterp, &keystr[start], sizeof(enterp));
            //printf("%s: enterp 0x%x, len %d/%d, %d\n\r", __func__, enterp, opt->data_len, keystr[start+sizeof(enterp)], __LINE__);
            if(opt->enterprise_number == enterp &&
                opt->data_len == keystr[start+sizeof(enterp)]) {
                found = 1;
                break;
            }
            start += sizeof(enterp);
            start += (keystr[start] + 1);
        }
        if(!found) {
            //printf("%s: %d\n\r", __func__, __LINE__);
            goto NO_MATCH;
        }
        //2.based the above result, comparing each sub code
        
        if(dh4op125_subopt_cmp(opt->data_len, opt->data, &keystr[start+sizeof(enterp)+1])) {
            //printf("%s: %d\n\r", __func__, __LINE__);
            goto NO_MATCH;
        }
        offset += (opt->data_len +2/*enterprise number field*/+1/*data len field*/);
        //printf("%s: offset %d, %d\n\r", __func__, offset, __LINE__);
    }
    return 0;
#endif    
NO_MATCH:
    printf("%s: op125 not match\n\r", __func__);
    return 1;
}  

/* Get an option with bounds checking (warning, result is not aligned) */
uint8_t* FAST_FUNC udhcp_get_option125(struct dhcp_packet *packet, int code)
{
	uint8_t *optionptr;
	int len;
	int rem;
	int overload = 0;
	enum {
		FILE_FIELD101  = FILE_FIELD  * 0x101,
		SNAME_FIELD101 = SNAME_FIELD * 0x101,
	};

	/* option bytes: [code][len][data1][data2]..[dataLEN] */
	optionptr = packet->options;
	rem = sizeof(packet->options);
	while (1) {
		if (rem <= 0) {
			bb_error_msg("bad packet, malformed option field");
			return NULL;
		}
		if (optionptr[OPT_CODE] == DHCP_PADDING) {
			rem--;
			optionptr++;
			continue;
		}
		if (optionptr[OPT_CODE] == DHCP_END) {
			if ((overload & FILE_FIELD101) == FILE_FIELD) {
				/* can use packet->file, and didn't look at it yet */
				overload |= FILE_FIELD101; /* "we looked at it" */
				optionptr = packet->file;
				rem = sizeof(packet->file);
				continue;
			}
			if ((overload & SNAME_FIELD101) == SNAME_FIELD) {
				/* can use packet->sname, and didn't look at it yet */
				overload |= SNAME_FIELD101; /* "we looked at it" */
				optionptr = packet->sname;
				rem = sizeof(packet->sname);
				continue;
			}
			break;
		}
		len = 2 + optionptr[OPT_LEN];
		rem -= len;
		if (rem < 0)
			continue; /* complain and return NULL */

		if (optionptr[OPT_CODE] == code) {
			//log_option("Option found", optionptr);
                        if(DHCP_VENDOR_INFO == code) {
                            return optionptr;
                        }
                        else {
                            return optionptr + OPT_DATA;
                        }
		}

		if (optionptr[OPT_CODE] == DHCP_OPTION_OVERLOAD) {
			overload |= optionptr[OPT_DATA];
			/* fall through */
		}
		optionptr += len;
	}

	/* log3 because udhcpc uses it a lot - very noisy */
	log3("Option 0x%02x not found", code);
	return NULL;
}

int dhcpc_op125_checking(const uint8_t *dh4op125, struct dhcp_packet *pkt)
{
    uint8_t *temp = NULL;

    if(!dh4op125 ) {
        return 0;
    }
    
    temp = udhcp_get_option125(pkt, DHCP_VENDOR_INFO);
    if (!temp) {
        bb_error_msg("not match the option 125 message in message");
        return -1;
    }
    
    return check_message_option125((const char *)dh4op125, (char*)temp);
}

static int write_ip_info_to_file(char *filename,dhcpc_ip_info_t *ipinfo)
{
	int ret = -1;
	FILE *fp = NULL;
	unsigned int up_time;
	
	if(access(filename,F_OK) == 0) {
		unlink(filename);
	}

	fp = fopen(filename,"w+");
	if(fp != NULL) {
		up_time = (unsigned int)time(NULL);
		fprintf(fp,"dev=%s\n",ipinfo->dev);
		fprintf(fp,"ip=%s\n",ipinfo->ip);
		fprintf(fp,"netmask=%s\n",ipinfo->netmask);
		fprintf(fp,"gateway=%s\n",ipinfo->gateway);
		fprintf(fp,"dns1=%s\n",ipinfo->dns1);
		fprintf(fp,"dns2=%s\n",ipinfo->dns2);
		fprintf(fp,"uptime=%u\n",up_time);
		ret = 0;
	}
	
	if(fp != NULL) {
		fclose(fp);
	}

	return ret;
}
#define IP_INFO_FILE_NAME "/var/run/dh6c/netcfg_%s.conf"
#define IP_STATE_FILE_NAME "/var/run/dh4c/netstate_%s.conf"

static int write_state_to_file(const struct client_config_t*client, int state)
{
	int ret = -1;
	FILE *fp = NULL;
	unsigned int up_time;
        char filename[128] = {0};

        if(!client)
            return -1;
        
        sprintf(filename, IP_STATE_FILE_NAME, client->interface);

	if(access(filename,F_OK) == 0) {
		unlink(filename);
	}

	fp = fopen(filename,"w+");
	if(fp != NULL) {
		up_time = (unsigned int)time(NULL);
		fprintf(fp,"state=%d\n",state);
		ret = 0;
	}
	
	if(fp != NULL) {
		fclose(fp);
	}

	return ret;
}

int mkdir_conf(void)
{
    char cmd[128] = {0};
    sprintf(cmd,"mkdir -p /var/run/dh4c");
    system(cmd); 

    return 0;
}
int dhcpc_info_write(struct dhcp_packet *packet)
{
    uint8_t *temp = NULL;
    char cmd[128];
    int rc= 0;
    
    memset(&dhcp_ipinfo, 0, sizeof(dhcpc_ip_info_t));
    //current CPE IP address
    sprintf(dhcp_ipinfo.ip, NIPQUAD_FMT, NIPQUAD(packet->yiaddr));

    //Gateway IP address
    if (!(temp = udhcp_get_option(packet, DHCP_ROUTER))) {
        sprintf(dhcp_ipinfo.gateway,"-");
    } else {
        sprintf(dhcp_ipinfo.gateway,"%d.%d.%d.%d",temp[0],temp[1],temp[2],temp[3]);
    }
    //DNS IP address
    if (!(temp = udhcp_get_option(packet, DHCP_DNS_SERVER))) {
        sprintf(dhcp_ipinfo.dns1, "-");
        sprintf(dhcp_ipinfo.dns2, "-");
    } 
    else 
    {
        if (*(temp - 1) == 4 || *(temp - 1) == 8) {
            sprintf(dhcp_ipinfo.dns1,"%d.%d.%d.%d",temp[0],temp[1],temp[2],temp[3]);
            if (*(temp - 1) == 8) {
                sprintf(dhcp_ipinfo.dns2,"%d.%d.%d.%d",temp[4],temp[5],temp[6],temp[7]);
            }
        } 
        else {
            bb_error_msg("dns server len error with ACK !!!");
        }
    }

    if (!(temp = udhcp_get_option(packet, DHCP_SUBNET))) {
        sprintf(dhcp_ipinfo.netmask,"-");
    } 
    else {
        sprintf(dhcp_ipinfo.netmask,"%d.%d.%d.%d",temp[0],temp[1],temp[2],temp[3]);
    }               
    if(client_config.interface != NULL) {
        sprintf(cmd,"%s/dh4c/netcfg_%s.conf",IP_CONFIG_PATH,client_config.interface);
        sprintf(dhcp_ipinfo.dev,"%s",client_config.interface);
        rc = write_ip_info_to_file(cmd, &dhcp_ipinfo);
        if( rc == 0) {
            //printf("write done\n\r");
            /*
            sig_val.sival_int = client_config.wanid;
            if(client_config.wand_pid != 0 || client_config.wanid != 0) {
                sigqueue(client_config.wand_pid,SIGNAL_UPDATE_ROUTE,sig_val);
            }
            */
        }
    }

}

int opt125_parse(char *opt125_str, dh4op125_t *opt125)
{
    char *src = opt125_str;
    char *str;
    int mode = 0;



/*
    printf("source string %s\n\r", src);
    printf("***************************\n\r");
    printf("after parse from string\n\r");
    printf("***************************\n\r");
    */
    //type
    str = strsep(&src, ":");
    //printf("%s: %s, src %s, %d\n\r", __func__, str,src, __LINE__);
    if(str) {
        opt125->type = atoi(str);
    }
    else 
        goto error;
    //len
    str = strsep(&src, ":");
    //printf("%s: %s, src %s, %d\n\r", __func__, str,src, __LINE__);
    if(str) {
        opt125->len = atoi(str);
    }
    else 
        goto error;

    if(opt125->type == 1) {
        dh4op125_opt_t *opt;
        dh4op125_subopt_t *subOpt;
        opt = (dh4op125_opt_t *)&opt125->data[OPT_DATA];
        subOpt = (dh4op125_subopt_t *)opt->data;
        /*snprintf(op125_str,128,"%1d:%1d:%1d:%1d:%1d:%1d:%1d:%1d:%s ", 
            op125->type, op125->len, 
            op125->data[OPT_CODE], op125->data[OPT_LEN], 
            opt->enterprise_number,opt->data_len,
            subOpt->sub_code, subOpt->sub_len , subOpt->data);*/

       //OPT_CODE
        str = strsep(&src, ":");
        //printf("%s: %s, src %s, %d\n\r", __func__, str,src, __LINE__);
        if(str) {
            opt125->data[OPT_CODE] = atoi(str);
            //printf("%s: %s, src %s, %d\n\r", __func__, str, src, __LINE__);
        }
        else 
            goto error;
        //OPT_LEN
        str = strsep(&src, ":");
        //printf("%s: %s, src %s, %d\n\r", __func__, str,src, __LINE__);
        if(str) {
            opt125->data[OPT_LEN] = atoi(str);
        }
        else 
            goto error;
        opt = (dh4op125_opt_t *)&opt125->data[OPT_DATA];
        //enterprise_number
        str = strsep(&src, ":");
        //printf("%s: %s, src %s, %d\n\r", __func__, str,src, __LINE__);
        if(str) {
            opt->enterprise_number = atoi(str);
        }
        else 
            goto error;
        //data_len
        str = strsep(&src, ":");
        //printf("%s: %s, src %s, %d\n\r", __func__, str,src, __LINE__);
        if(str) {
            opt->data_len = atoi(str);
        }
        else 
            goto error;
        //subOpt->sub_code
        str = strsep(&src, ":");
        //printf("%s: %s, src %s, %d\n\r", __func__, str,src, __LINE__);
        if(str) {
            subOpt->sub_code = atoi(str);
        }
        else 
            goto error;

        //subOpt->sub_len
        //printf("%s: %s, src %s, %d\n\r", __func__, str,src, __LINE__);
        str = strsep(&src, ":");
        if(str) {
            subOpt->sub_len = atoi(str);
        }
        else 
            goto error;
        //printf("%s: %s, src %s, %d\n\r", __func__, str,src, __LINE__);
        //subOpt->data
        if(src) {
            strcpy(subOpt->data, src);
        }
        /*
        printf("type 1\n\r");
        printf("type %d\n\r", opt125->type);
        printf("len %d\n\r", opt125->len);
        printf("OPT_CODE %d\n\r", opt125->data[OPT_CODE]);
        printf("OPT_LEN %d\n\r", opt125->data[OPT_LEN]);
        printf("enterprise_number %d\n\r", opt->enterprise_number);
        printf("data_len %d\n\r", opt->data_len);
        printf("sub_code %d\n\r", subOpt->sub_code);
        printf("sub_len %d\n\r", subOpt->sub_len);
        printf("data %s\n\r", subOpt->data);
        */
    }
    else {

        /*snprintf(op125_str,128,"%1d:%1d:%1d:%1d:%1d:%1d:%s ", 
            op125->type, op125->len, 
            op125->data[OPT_CODE], op125->data[OPT_LEN], 
            opt->enterprise_number,opt->data_len,
            subOpt->data);*/
            

        dh4op125_opt_t *opt;
        opt = (dh4op125_opt_t *)&opt125->data[OPT_DATA];


        //OPT_CODE
        str = strsep(&src, ":");
        if(str) {
            opt125->data[OPT_CODE] = atoi(str);
        }
        else 
            goto error;
        //OPT_LEN
        str = strsep(&src, ":");
        if(str) {
            opt125->data[OPT_LEN] = atoi(str);
        }
        else 
            goto error;
        opt = (dh4op125_opt_t *)&opt125->data[OPT_DATA];
        //enterprise_number
        str = strsep(&src, ":");
        if(str) {
            opt->enterprise_number = atoi(str);
        }
        else 
            goto error;
        //data_len
        str = strsep(&src, ":");
        if(str) {
            opt->data_len = atoi(str);
        }
        else 
            goto error;
        //subOpt->data
        if(src) {
            strcpy(opt->data, src);
        }
        /*
        printf("type 2\n\r");
        printf("type %d\n\r", opt125->type);
        printf("len %d\n\r", opt125->len);
        printf("OPT_CODE %d\n\r", opt125->data[OPT_CODE]);
        printf("OPT_LEN %d\n\r", opt125->data[OPT_LEN]);
        printf("enterprise_number %d\n\r", opt->enterprise_number);
        printf("data_len %d\n\r", opt->data_len);
        printf("data %s\n\r", opt->data);
        */
    }
    
error:
    return 0;
}
typedef struct {
    uint8_t code;//60
    uint8_t len;//total len below
    uint16_t enterprise_number;//
    uint8_t field_type;//34, etc
    uint8_t field_len;//total len below
    uint8_t data[0];//125|len|dh4op125_opt_t
}__attribute__ ((__packed__))dh4op60_t; 

int opt60_parse(char *opt60_str, dh4op60_t *op60)
{
    char *src = opt60_str;
    char *str;
/*
    printf("source string %s\n\r", src);
    printf("***************************\n\r");
    printf("after parse from string\n\r");
    printf("***************************\n\r");
    */
    //op60->code
    str = strsep(&src, ":");
    //printf("%s: %s, src %s, %d\n\r", __func__, str,src, __LINE__);
    if(str) {
        op60->code = atoi(str);
    }
    else 
        goto error;
    //op60->len
    str = strsep(&src, ":");
    //printf("%s: %s, src %s, %d\n\r", __func__, str,src, __LINE__);
    if(str) {
        op60->len = atoi(str);
    }
    else 
        goto error;

    //op60->enterprise_number
    str = strsep(&src, ":");
    //printf("%s: %s, src %s, %d\n\r", __func__, str,src, __LINE__);
    if(str) {
        op60->enterprise_number = atoi(str);
    }
    else 
        goto error;

    //op60->field_type
    str = strsep(&src, ":");
    //printf("%s: %s, src %s, %d\n\r", __func__, str,src, __LINE__);
    if(str) {
        op60->field_type = atoi(str);
    }
    else 
        goto error;

    //op60->field_len
    str = strsep(&src, ":");
    //printf("%s: %s, src %s, %d\n\r", __func__, str,src, __LINE__);
    if(str) {
        op60->field_len = atoi(str);
    }
    else 
        goto error;

    //op60->field_len
    if(src) {
        strcpy(op60->data, src);
    }
    else 
        goto error;
    /*
    printf("code %d\n\r", op60->code);
    printf("len %d\n\r", op60->len);
    printf("enterprise_number %d\n\r", op60->enterprise_number);
    printf("field_len %d\n\r", op60->field_type);
    printf("field_len %d\n\r", op60->field_len);
    printf("data %s\n\r", op60->data);
        
    */
error:
    return 0;
}

#endif /* FEATURE_OP60_OP125_SUPPORT */

/* Create "opt_name=opt_value" string */
static NOINLINE char *xmalloc_optname_optval(uint8_t *option, const struct dhcp_optflag *optflag, const char *opt_name)
{
	unsigned upper_length;
	int len, type, optlen;
	char *dest, *ret;

	/* option points to OPT_DATA, need to go back and get OPT_LEN */
	len = option[OPT_LEN - OPT_DATA];
	type = optflag->flags & OPTION_TYPE_MASK;
	optlen = dhcp_option_lengths[type];
	upper_length = len_of_option_as_string[type] * ((unsigned)len / (unsigned)optlen);

	dest = ret = xmalloc(upper_length + strlen(opt_name) + 2);
	dest += sprintf(ret, "%s=", opt_name);

	while (len >= optlen) {
		switch (type) {
		case OPTION_IP_PAIR:
			dest += sprint_nip(dest, "", option);
			*dest++ = '/';
			option += 4;
			optlen = 4;
		case OPTION_IP:
			dest += sprint_nip(dest, "", option);
// TODO: it can be a list only if (optflag->flags & OPTION_LIST).
// Should we bail out/warn if we see multi-ip option which is
// not allowed to be such? For example, DHCP_BROADCAST...
			break;
//		case OPTION_BOOLEAN:
//			dest += sprintf(dest, *option ? "yes" : "no");
//			break;
		case OPTION_U8:
			dest += sprintf(dest, "%u", *option);
			break;
//		case OPTION_S16:
		case OPTION_U16: {
			uint16_t val_u16;
			move_from_unaligned16(val_u16, option);
			dest += sprintf(dest, "%u", ntohs(val_u16));
			break;
		}
		case OPTION_S32:
		case OPTION_U32: {
			uint32_t val_u32;
			move_from_unaligned32(val_u32, option);
			dest += sprintf(dest, type == OPTION_U32 ? "%lu" : "%ld", (unsigned long) ntohl(val_u32));
			break;
		}
		case OPTION_STRING:
			memcpy(dest, option, len);
			dest[len] = '\0';
			return ret;	 /* Short circuit this case */
		case OPTION_STATIC_ROUTES: {
			/* Option binary format:
			 * mask [one byte, 0..32]
			 * ip [big endian, 0..4 bytes depending on mask]
			 * router [big endian, 4 bytes]
			 * may be repeated
			 *
			 * We convert it to a string "IP/MASK ROUTER IP2/MASK2 ROUTER2"
			 */
			const char *pfx = "";

			while (len >= 1 + 4) { /* mask + 0-byte ip + router */
				uint32_t nip;
				uint8_t *p;
				unsigned mask;
				int bytes;

				mask = *option++;
				if (mask > 32)
					break;
				len--;

				nip = 0;
				p = (void*) &nip;
				bytes = (mask + 7) / 8; /* 0 -> 0, 1..8 -> 1, 9..16 -> 2 etc */
				while (--bytes >= 0) {
					*p++ = *option++;
					len--;
				}
				if (len < 4)
					break;

				/* print ip/mask */
				dest += sprint_nip(dest, pfx, (void*) &nip);
				pfx = " ";
				dest += sprintf(dest, "/%u ", mask);
				/* print router */
				dest += sprint_nip(dest, "", option);
				option += 4;
				len -= 4;
			}

			return ret;
		}
		case OPTION_6RD: {
			/* Option binary format:
			 *  0                   1                   2                   3
			 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
			 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			 *  |  OPTION_6RD   | option-length |  IPv4MaskLen  |  6rdPrefixLen |
			 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			 *  |                                                               |
			 *  |                           6rdPrefix                           |
			 *  |                          (16 octets)                          |
			 *  |                                                               |
			 *  |                                                               |
			 *  |                                                               |
			 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			 *  |                     6rdBRIPv4Address(es)                      |
			 *  .                                                               .
			 *  .                                                               .
			 *  .                                                               .
			 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			 *
			 * We convert it to a string "IPv4MaskLen 6rdPrefixLen 6rdPrefix 6rdBRIPv4Address"
			 */

			/* Sanity check: ensure that our length is at least 22 bytes, that
			 * IPv4MaskLen is <= 32, 6rdPrefixLen <= 128 and that the sum of
			 * (32 - IPv4MaskLen) + 6rdPrefixLen is less than or equal to 128.
			 * If any of these requirements is not fulfilled, return with empty
			 * value.
			 */
			if ((len >= 22) && (*option <= 32) && (*(option+1) <= 128) &&
			    (((32 - *option) + *(option+1)) <= 128))
			{
				/* IPv4MaskLen */
				dest += sprintf(dest, "%u ", *option++);
				len--;

				/* 6rdPrefixLen */
				dest += sprintf(dest, "%u ", *option++);
				len--;

				/* 6rdPrefix */
				dest += sprint_nip6(dest, "", option);
				option += 16;
				len -= 16;

				/* 6rdBRIPv4Addresses */
				while (len >= 4)
				{
					dest += sprint_nip(dest, " ", option);
					option += 4;
					len -= 4;

					/* the code to determine the option size fails to work with
					 * lengths that are not a multiple of the minimum length,
					 * adding all advertised 6rdBRIPv4Addresses here would
					 * overflow the destination buffer, therefore skip the rest
					 * for now
					 */
					break;
				}
			}

			return ret;
		}
#if ENABLE_FEATURE_UDHCP_RFC3397
		case OPTION_DNS_STRING:
			/* unpack option into dest; use ret for prefix (i.e., "optname=") */
			dest = dname_dec(option, len, ret);
			if (dest) {
				free(ret);
				return dest;
			}
			/* error. return "optname=" string */
			return ret;
		case OPTION_SIP_SERVERS:
			/* Option binary format:
			 * type: byte
			 * type=0: domain names, dns-compressed
			 * type=1: IP addrs
			 */
			option++;
			len--;
			if (option[-1] == 0) {
				dest = dname_dec(option, len, ret);
				if (dest) {
					free(ret);
					return dest;
				}
			} else
			if (option[-1] == 1) {
				const char *pfx = "";
				while (1) {
					len -= 4;
					if (len < 0)
						break;
					dest += sprint_nip(dest, pfx, option);
					pfx = " ";
					option += 4;
				}
			}
			return ret;
#endif
		} /* switch */
		option += optlen;
		len -= optlen;
		if (len <= 0)
			break;
		*dest++ = ' ';
		*dest = '\0';
	}
	return ret;
}

/* put all the parameters into the environment */
static char **fill_envp(struct dhcp_packet *packet)
{
	int envc;
	int i;
	char **envp, **curr;
	const char *opt_name;
	uint8_t *temp;
	uint8_t overload = 0;

	/* We need 6 elements for:
	 * "interface=IFACE"
	 * "ip=N.N.N.N" from packet->yiaddr
	 * "siaddr=IP" from packet->siaddr_nip (unless 0)
	 * "boot_file=FILE" from packet->file (unless overloaded)
	 * "sname=SERVER_HOSTNAME" from packet->sname (unless overloaded)
	 * terminating NULL
	 */
	envc = 6;
	/* +1 element for each option, +2 for subnet option: */
	if (packet) {
		for (i = 0; dhcp_optflags[i].code; i++) {
			if (udhcp_get_option(packet, dhcp_optflags[i].code)) {
				if (dhcp_optflags[i].code == DHCP_SUBNET)
					envc++; /* for mton */
				envc++;
			}
		}
		temp = udhcp_get_option(packet, DHCP_OPTION_OVERLOAD);
		if (temp)
			overload = *temp;
	}
	curr = envp = xzalloc(sizeof(char *) * envc);

	*curr = xasprintf("interface=%s", client_config.interface);
	putenv(*curr++);

	if (!packet)
		return envp;

	*curr = xmalloc(sizeof("ip=255.255.255.255"));
	sprint_nip(*curr, "ip=", (uint8_t *) &packet->yiaddr);
	putenv(*curr++);

	opt_name = dhcp_option_strings;
	i = 0;
	while (*opt_name) {
		temp = udhcp_get_option(packet, dhcp_optflags[i].code);
		if (!temp)
			goto next;
		*curr = xmalloc_optname_optval(temp, &dhcp_optflags[i], opt_name);
		putenv(*curr++);
		if (dhcp_optflags[i].code == DHCP_SUBNET) {
			/* Subnet option: make things like "$ip/$mask" possible */
			uint32_t subnet;
			move_from_unaligned32(subnet, temp);
			*curr = xasprintf("mask=%d", mton(subnet));
			putenv(*curr++);
		}
 next:
		opt_name += strlen(opt_name) + 1;
		i++;
	}
	if (packet->siaddr_nip) {
		*curr = xmalloc(sizeof("siaddr=255.255.255.255"));
		sprint_nip(*curr, "siaddr=", (uint8_t *) &packet->siaddr_nip);
		putenv(*curr++);
	}
	if (!(overload & FILE_FIELD) && packet->file[0]) {
		/* watch out for invalid packets */
		*curr = xasprintf("boot_file=%."DHCP_PKT_FILE_LEN_STR"s", packet->file);
		putenv(*curr++);
	}
	if (!(overload & SNAME_FIELD) && packet->sname[0]) {
		/* watch out for invalid packets */
		*curr = xasprintf("sname=%."DHCP_PKT_SNAME_LEN_STR"s", packet->sname);
		putenv(*curr++);
	}
	return envp;
}

/* Call a script with a par file and env vars */
static void udhcp_run_script(struct dhcp_packet *packet, const char *name)
{
	char **envp, **curr;
	char *argv[3];

	if (client_config.script == NULL)
		return;

	envp = fill_envp(packet);

	/* call script */
	log1("Executing %s %s", client_config.script, name);
	argv[0] = (char*) client_config.script;
	argv[1] = (char*) name;
	argv[2] = NULL;
	spawn_and_wait(argv);

	for (curr = envp; *curr; curr++) {
		log2(" %s", *curr);
		bb_unsetenv_and_free(*curr);
	}
	free(envp);
}


/*** Sending/receiving packets ***/

static ALWAYS_INLINE uint32_t random_xid(void)
{
	return rand();
}

/* Initialize the packet with the proper defaults */
static void init_packet(struct dhcp_packet *packet, char type)
{
	udhcp_init_header(packet, type);
	memcpy(packet->chaddr, client_config.client_mac, 6);
	if (client_config.clientid)
		udhcp_add_binary_option(packet, client_config.clientid);
	if (client_config.hostname)
		udhcp_add_binary_option(packet, client_config.hostname);
	if (client_config.fqdn)
		udhcp_add_binary_option(packet, client_config.fqdn);
	if (type != DHCPDECLINE
	 && type != DHCPRELEASE
	 && client_config.vendorclass
	) {
		udhcp_add_binary_option(packet, client_config.vendorclass);
	}
}

static void add_client_options(struct dhcp_packet *packet)
{
	/* Add a "param req" option with the list of options we'd like to have
	 * from stubborn DHCP servers. Pull the data from the struct in common.c.
	 * No bounds checking because it goes towards the head of the packet. */
	uint8_t c;
	int end = udhcp_end_option(packet->options);
	int i, len = 0;

	for (i = 0; (c = dhcp_optflags[i].code) != 0; i++) {
		if ((   (dhcp_optflags[i].flags & OPTION_REQ)
		     && !client_config.no_default_options
		    )
		 || (client_config.opt_mask[c >> 3] & (1 << (c & 7)))
		) {
			packet->options[end + OPT_DATA + len] = c;
			len++;
		}
	}
	if (len) {
		packet->options[end + OPT_CODE] = DHCP_PARAM_REQ;
		packet->options[end + OPT_LEN] = len;
		packet->options[end + OPT_DATA + len] = DHCP_END;
	}

	/* Add -x options if any */
	{
		struct option_set *curr = client_config.options;
		while (curr) {
			udhcp_add_binary_option(packet, curr->data);
			curr = curr->next;
		}
//		if (client_config.sname)
//			strncpy((char*)packet->sname, client_config.sname, sizeof(packet->sname) - 1);
//		if (client_config.boot_file)
//			strncpy((char*)packet->file, client_config.boot_file, sizeof(packet->file) - 1);
	}
}

/* RFC 2131
 * 4.4.4 Use of broadcast and unicast
 *
 * The DHCP client broadcasts DHCPDISCOVER, DHCPREQUEST and DHCPINFORM
 * messages, unless the client knows the address of a DHCP server.
 * The client unicasts DHCPRELEASE messages to the server. Because
 * the client is declining the use of the IP address supplied by the server,
 * the client broadcasts DHCPDECLINE messages.
 *
 * When the DHCP client knows the address of a DHCP server, in either
 * INIT or REBOOTING state, the client may use that address
 * in the DHCPDISCOVER or DHCPREQUEST rather than the IP broadcast address.
 * The client may also use unicast to send DHCPINFORM messages
 * to a known DHCP server. If the client receives no response to DHCP
 * messages sent to the IP address of a known DHCP server, the DHCP
 * client reverts to using the IP broadcast address.
 */

static int raw_bcast_from_client_config_ifindex(struct dhcp_packet *packet)
{
	return udhcp_send_raw_packet(packet,
		/*src*/ INADDR_ANY, CLIENT_PORT,
		/*dst*/ INADDR_BROADCAST, SERVER_PORT, MAC_BCAST_ADDR,
		client_config.ifindex);
}

/* Broadcast a DHCP discover packet to the network, with an optionally requested IP */
static int send_discover(uint32_t xid, uint32_t requested)
{
	struct dhcp_packet packet;
	static int msgs = 0;

	init_packet(&packet, DHCPDISCOVER);
	packet.xid = xid;
	if (requested)
		udhcp_add_simple_option(&packet, DHCP_REQUESTED_IP, requested);
	/* Explicitly saying that we want RFC-compliant packets helps
	 * some buggy DHCP servers to NOT send bigger packets */
	udhcp_add_simple_option(&packet, DHCP_MAX_SIZE, htons(576));
	add_client_options(&packet);

	if (msgs++ < 3)
	bb_info_msg("Sending discover...");
	return raw_bcast_from_client_config_ifindex(&packet);
}

/* Broadcast a DHCP request message */
/* RFC 2131 3.1 paragraph 3:
 * "The client _broadcasts_ a DHCPREQUEST message..."
 */
static int send_select(uint32_t xid, uint32_t server, uint32_t requested)
{
	struct dhcp_packet packet;
	struct in_addr addr;

	init_packet(&packet, DHCPREQUEST);
	packet.xid = xid;
	udhcp_add_simple_option(&packet, DHCP_REQUESTED_IP, requested);
	udhcp_add_simple_option(&packet, DHCP_SERVER_ID, server);
	add_client_options(&packet);

	addr.s_addr = requested;
	bb_info_msg("Sending select for %s...", inet_ntoa(addr));
	return raw_bcast_from_client_config_ifindex(&packet);
}

/* Unicast or broadcast a DHCP renew message */
static int send_renew(uint32_t xid, uint32_t server, uint32_t ciaddr)
{
	struct dhcp_packet packet;

	init_packet(&packet, DHCPREQUEST);
	packet.xid = xid;
	packet.ciaddr = ciaddr;
	add_client_options(&packet);

	bb_info_msg("Sending renew...");
	if (server)
		return udhcp_send_kernel_packet(&packet,
			ciaddr, CLIENT_PORT,
			server, SERVER_PORT);
	return raw_bcast_from_client_config_ifindex(&packet);
}

#if ENABLE_FEATURE_UDHCPC_ARPING
/* Broadcast a DHCP decline message */
static int send_decline(uint32_t xid, uint32_t server, uint32_t requested)
{
	struct dhcp_packet packet;

	init_packet(&packet, DHCPDECLINE);
	packet.xid = xid;
	udhcp_add_simple_option(&packet, DHCP_REQUESTED_IP, requested);
	udhcp_add_simple_option(&packet, DHCP_SERVER_ID, server);

	bb_info_msg("Sending decline...");
	return raw_bcast_from_client_config_ifindex(&packet);
}
#endif

/* Unicast a DHCP release message */
static int send_release(uint32_t server, uint32_t ciaddr)
{
	struct dhcp_packet packet;

	init_packet(&packet, DHCPRELEASE);
	packet.xid = random_xid();
	packet.ciaddr = ciaddr;

	udhcp_add_simple_option(&packet, DHCP_SERVER_ID, server);

	bb_info_msg("Sending release...");
	return udhcp_send_kernel_packet(&packet, ciaddr, CLIENT_PORT, server, SERVER_PORT);
}

/* Returns -1 on errors that are fatal for the socket, -2 for those that aren't */
static NOINLINE int udhcp_recv_raw_packet(struct dhcp_packet *dhcp_pkt, int fd)
{
	int bytes;
	struct ip_udp_dhcp_packet packet;
	uint16_t check;

	memset(&packet, 0, sizeof(packet));
	bytes = safe_read(fd, &packet, sizeof(packet));
	if (bytes < 0) {
		log1("Packet read error, ignoring");
		/* NB: possible down interface, etc. Caller should pause. */
		return bytes; /* returns -1 */
	}

	if (bytes < (int) (sizeof(packet.ip) + sizeof(packet.udp))) {
		log1("Packet is too short, ignoring");
		return -2;
	}

	if (bytes < ntohs(packet.ip.tot_len)) {
		/* packet is bigger than sizeof(packet), we did partial read */
		log1("Oversized packet, ignoring");
		return -2;
	}

	/* ignore any extra garbage bytes */
	bytes = ntohs(packet.ip.tot_len);

	/* make sure its the right packet for us, and that it passes sanity checks */
	if (packet.ip.protocol != IPPROTO_UDP || packet.ip.version != IPVERSION
	 || packet.ip.ihl != (sizeof(packet.ip) >> 2)
	 || packet.udp.dest != htons(CLIENT_PORT)
	/* || bytes > (int) sizeof(packet) - can't happen */
	 || ntohs(packet.udp.len) != (uint16_t)(bytes - sizeof(packet.ip))
	) {
		log1("Unrelated/bogus packet, ignoring");
		return -2;
	}

	/* verify IP checksum */
	check = packet.ip.check;
	packet.ip.check = 0;
	if (check != udhcp_checksum(&packet.ip, sizeof(packet.ip))) {
		log1("Bad IP header checksum, ignoring");
		return -2;
	}

	/* verify UDP checksum. IP header has to be modified for this */
	memset(&packet.ip, 0, offsetof(struct iphdr, protocol));
	/* ip.xx fields which are not memset: protocol, check, saddr, daddr */
	packet.ip.tot_len = packet.udp.len; /* yes, this is needed */
	check = packet.udp.check;
	packet.udp.check = 0;
	if (check && check != udhcp_checksum(&packet, bytes)) {
		log1("Packet with bad UDP checksum received, ignoring");
		return -2;
	}

	memcpy(dhcp_pkt, &packet.data, bytes - (sizeof(packet.ip) + sizeof(packet.udp)));

	if (dhcp_pkt->cookie != htonl(DHCP_MAGIC)) {
		bb_info_msg("Packet with bad magic, ignoring");
		return -2;
	}
	log1("Got valid DHCP packet");
	udhcp_dump_packet(dhcp_pkt);
	return bytes - (sizeof(packet.ip) + sizeof(packet.udp));
}


/*** Main ***/

static int sockfd = -1;

#define LISTEN_NONE   0
#define LISTEN_KERNEL 1
#define LISTEN_RAW    2
static smallint listen_mode;

/* initial state: (re)start DHCP negotiation */
#define INIT_SELECTING  0
/* discover was sent, DHCPOFFER reply received */
#define REQUESTING      1
/* select/renew was sent, DHCPACK reply received */
#define BOUND           2
/* half of lease passed, want to renew it by sending unicast renew requests */
#define RENEWING        3
/* renew requests were not answered, lease is almost over, send broadcast renew */
#define REBINDING       4
/* manually requested renew (SIGUSR1) */
#define RENEW_REQUESTED 5
/* release, possibly manually requested (SIGUSR2) */
#define RELEASED        6
static smallint state;

static int udhcp_raw_socket(int ifindex)
{
	int fd;
	struct sockaddr_ll sock;

	/*
	 * Comment:
	 *
	 *	I've selected not to see LL header, so BPF doesn't see it, too.
	 *	The filter may also pass non-IP and non-ARP packets, but we do
	 *	a more complete check when receiving the message in userspace.
	 *
	 * and filter shamelessly stolen from:
	 *
	 *	http://www.flamewarmaster.de/software/dhcpclient/
	 *
	 * There are a few other interesting ideas on that page (look under
	 * "Motivation").  Use of netlink events is most interesting.  Think
	 * of various network servers listening for events and reconfiguring.
	 * That would obsolete sending HUP signals and/or make use of restarts.
	 *
	 * Copyright: 2006, 2007 Stefan Rompf <sux@loplof.de>.
	 * License: GPL v2.
	 *
	 * TODO: make conditional?
	 */
#define SERVER_AND_CLIENT_PORTS  ((67 << 16) + 68)
	static const struct sock_filter filter_instr[] = {
		/* check for udp */
		BPF_STMT(BPF_LD|BPF_B|BPF_ABS, 9),
		BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K, IPPROTO_UDP, 2, 0),     /* L5, L1, is UDP? */
		/* ugly check for arp on ethernet-like and IPv4 */
		BPF_STMT(BPF_LD|BPF_W|BPF_ABS, 2),                      /* L1: */
		BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K, 0x08000604, 3, 4),      /* L3, L4 */
		/* skip IP header */
		BPF_STMT(BPF_LDX|BPF_B|BPF_MSH, 0),                     /* L5: */
		/* check udp source and destination ports */
		BPF_STMT(BPF_LD|BPF_W|BPF_IND, 0),
		BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K, SERVER_AND_CLIENT_PORTS, 0, 1),	/* L3, L4 */
		/* returns */
		BPF_STMT(BPF_RET|BPF_K, 0x0fffffff ),                   /* L3: pass */
		BPF_STMT(BPF_RET|BPF_K, 0),                             /* L4: reject */
	};
	static const struct sock_fprog filter_prog = {
		.len = sizeof(filter_instr) / sizeof(filter_instr[0]),
		/* casting const away: */
		.filter = (struct sock_filter *) filter_instr,
	};

	log1("Opening raw socket on ifindex %d", ifindex); //log2?

	fd = xsocket(PF_PACKET, SOCK_DGRAM, htons(ETH_P_IP));
	log1("Got raw socket fd %d", fd); //log2?

	if (SERVER_PORT == 67 && CLIENT_PORT == 68) {
		/* Use only if standard ports are in use */
		/* Ignoring error (kernel may lack support for this) */
		if (setsockopt(fd, SOL_SOCKET, SO_ATTACH_FILTER, &filter_prog,
				sizeof(filter_prog)) >= 0)
			log1("Attached filter to raw socket fd %d", fd); // log?
	}

	sock.sll_family = AF_PACKET;
	sock.sll_protocol = htons(ETH_P_IP);
	sock.sll_ifindex = ifindex;
	xbind(fd, (struct sockaddr *) &sock, sizeof(sock));
	log1("Created raw socket");

	return fd;
}

static void change_listen_mode(int new_mode)
{
	log1("Entering listen mode: %s",
		new_mode != LISTEN_NONE
			? (new_mode == LISTEN_KERNEL ? "kernel" : "raw")
			: "none"
	);

	listen_mode = new_mode;
	if (sockfd >= 0) {
		close(sockfd);
		sockfd = -1;
	}
	if (new_mode == LISTEN_KERNEL)
		sockfd = udhcp_listen_socket(/*INADDR_ANY,*/ CLIENT_PORT, client_config.interface);
	else if (new_mode != LISTEN_NONE)
		sockfd = udhcp_raw_socket(client_config.ifindex);
	/* else LISTEN_NONE: sockfd stays closed */
}

static void perform_renew(void)
{
	bb_info_msg("Performing a DHCP renew");
	switch (state) {
	case BOUND:
		change_listen_mode(LISTEN_KERNEL);
	case RENEWING:
	case REBINDING:
		state = RENEW_REQUESTED;
		break;
	case RENEW_REQUESTED: /* impatient are we? fine, square 1 */
	case REQUESTING:
	case RELEASED:
		change_listen_mode(LISTEN_RAW);
		state = INIT_SELECTING;
		break;
	case INIT_SELECTING:
		break;
	}
}

static void perform_release(uint32_t requested_ip, uint32_t server_addr)
{
	char buffer[sizeof("255.255.255.255")];
	struct in_addr temp_addr;

#ifdef FEATURE_OP60_OP125_SUPPORT    
        write_state_to_file(&client_config, RELEASED);
#endif
	/* send release packet */
	if (state == BOUND || state == RENEWING || state == REBINDING) {
		temp_addr.s_addr = server_addr;
		strcpy(buffer, inet_ntoa(temp_addr));
		temp_addr.s_addr = requested_ip;
		bb_info_msg("Unicasting a release of %s to %s",
				inet_ntoa(temp_addr), buffer);
		send_release(server_addr, requested_ip); /* unicast */
		udhcp_run_script(NULL, "deconfig");
	}
	bb_info_msg("Entering released state");

	change_listen_mode(LISTEN_NONE);
	state = RELEASED;
}

static uint8_t* alloc_dhcp_option(int code, const char *str, int extra)
{
	uint8_t *storage;
	int len = strnlen(str, 255);
	storage = xzalloc(len + extra + OPT_DATA);
	storage[OPT_CODE] = code;
	storage[OPT_LEN] = len + extra;
	memcpy(storage + extra + OPT_DATA, str, len);
	return storage;
}

#if BB_MMU
static void client_background(void)
{
	bb_daemonize(0);
	logmode &= ~LOGMODE_STDIO;
	/* rewrite pidfile, as our pid is different now */
	write_pidfile(client_config.pidfile);
}
#endif

int udhcpc_main(int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;
int udhcpc_main(int argc UNUSED_PARAM, char **argv)
{
	uint8_t *temp, *message;
	const char *str_c, *str_V, *str_h, *str_F, *str_r;
#ifdef FEATURE_OP60_OP125_SUPPORT
	 char *str_I;/*vendor_cls_id*/
#endif
	IF_FEATURE_UDHCP_PORT(char *str_P;)
	llist_t *list_O = NULL;
	llist_t *list_x = NULL;
	int tryagain_timeout = 20;
	int discover_timeout = 3;
	int discover_retries = 3;
	uint32_t server_addr = server_addr; /* for compiler */
	uint32_t requested_ip = 0;
	uint32_t xid = 0;
	uint32_t lease_seconds = 0; /* can be given as 32-bit quantity */
	int packet_num;
	int timeout; /* must be signed */
	unsigned already_waited_sec;
	unsigned opt;
	int max_fd;
	int retval;
	struct timeval tv;
	struct dhcp_packet packet;
	fd_set rfds;

#if ENABLE_LONG_OPTS
	static const char udhcpc_longopts[] ALIGN1 =
		"clientid\0"       Required_argument "c"
		"clientid-none\0"  No_argument       "C"
		"vendorclass\0"    Required_argument "V"
		"hostname\0"       Required_argument "H"
		"fqdn\0"           Required_argument "F"
		"interface\0"      Required_argument "i"
		"now\0"            No_argument       "n"
		"pidfile\0"        Required_argument "p"
		"quit\0"           No_argument       "q"
		"release\0"        No_argument       "R"
		"request\0"        Required_argument "r"
		"script\0"         Required_argument "s"
		"timeout\0"        Required_argument "T"
		"version\0"        No_argument       "v"
		"retries\0"        Required_argument "t"
		"tryagain\0"       Required_argument "A"
		"syslog\0"         No_argument       "S"
		"request-option\0" Required_argument "O"
		"no-default-options\0" No_argument   "o"
		"foreground\0"     No_argument       "f"
		"background\0"     No_argument       "b"
#ifdef FEATURE_OP60_OP125_SUPPORT
            "vendorid\0"       Required_argument "I"
#endif //HGU
		IF_FEATURE_UDHCPC_ARPING("arping\0"	No_argument       "a")
		IF_FEATURE_UDHCP_PORT("client-port\0"	Required_argument "P")
		;
#endif
	enum {
		OPT_c = 1 << 0,
		OPT_C = 1 << 1,
		OPT_V = 1 << 2,
		OPT_H = 1 << 3,
		OPT_h = 1 << 4,
		OPT_F = 1 << 5,
		OPT_i = 1 << 6,
		OPT_n = 1 << 7,
		OPT_p = 1 << 8,
		OPT_q = 1 << 9,
		OPT_R = 1 << 10,
		OPT_r = 1 << 11,
		OPT_s = 1 << 12,
		OPT_T = 1 << 13,
		OPT_t = 1 << 14,
		OPT_S = 1 << 15,
		OPT_A = 1 << 16,
		OPT_O = 1 << 17,
		OPT_o = 1 << 18,
		OPT_x = 1 << 19,
		OPT_f = 1 << 20,
#ifdef FEATURE_OP60_OP125_SUPPORT
		OPT_I = 1 << 21, //
#endif //HGU
        
/* The rest has variable bit positions, need to be clever */
#ifndef FEATURE_OP60_OP125_SUPPORT
		OPTBIT_f = 20,
#else
		OPTBIT_f = 21,
#endif /* FEATURE_OP60_OP125_SUPPORT */
		USE_FOR_MMU(             OPTBIT_b,)
		IF_FEATURE_UDHCPC_ARPING(OPTBIT_a,)
		IF_FEATURE_UDHCP_PORT(   OPTBIT_P,)
		USE_FOR_MMU(             OPT_b = 1 << OPTBIT_b,)
		IF_FEATURE_UDHCPC_ARPING(OPT_a = 1 << OPTBIT_a,)
		IF_FEATURE_UDHCP_PORT(   OPT_P = 1 << OPTBIT_P,)
	};

	/* Default options. */
	IF_FEATURE_UDHCP_PORT(SERVER_PORT = 67;)
	IF_FEATURE_UDHCP_PORT(CLIENT_PORT = 68;)
	client_config.interface = "eth0";
	client_config.script = CONFIG_UDHCPC_DEFAULT_SCRIPT;
	str_V = "udhcp "BB_VER;

	/* Parse command line */
	/* Cc: mutually exclusive; O,x: list; -T,-t,-A take numeric param */
	opt_complementary = "c--C:C--c:O::x::T+:t+:A+"
#if defined CONFIG_UDHCP_DEBUG && CONFIG_UDHCP_DEBUG >= 1
		":vv"
#endif
		;
	IF_LONG_OPTS(applet_long_options = udhcpc_longopts;)
#ifdef FEATURE_OP60_OP125_SUPPORT        
	opt = getopt32(argv, "c:CV:H:h:F:i:np:qRr:s:T:t:SA:O:ox:fI:"
#else
	opt = getopt32(argv, "c:CV:H:h:F:i:np:qRr:s:T:t:SA:O:ox:f"
#endif
		USE_FOR_MMU("b")
		IF_FEATURE_UDHCPC_ARPING("a")
		IF_FEATURE_UDHCP_PORT("P:")
		"v"
		, &str_c, &str_V, &str_h, &str_h, &str_F
		, &client_config.interface, &client_config.pidfile, &str_r /* i,p */
		, &client_config.script /* s */
		, &discover_timeout, &discover_retries, &tryagain_timeout /* T,t,A */
		, &list_O
		, &list_x
#ifdef FEATURE_OP60_OP125_SUPPORT
		,&str_I
#endif  //HGU
		IF_FEATURE_UDHCP_PORT(, &str_P)
#if defined CONFIG_UDHCP_DEBUG && CONFIG_UDHCP_DEBUG >= 1
		, &dhcp_verbose
#endif
		);
	if (opt & (OPT_h|OPT_H))
		client_config.hostname = alloc_dhcp_option(DHCP_HOST_NAME, str_h, 0);
	if (opt & OPT_F) {
		/* FQDN option format: [0x51][len][flags][0][0]<fqdn> */
		client_config.fqdn = alloc_dhcp_option(DHCP_FQDN, str_F, 3);
		/* Flag bits: 0000NEOS
		 * S: 1 = Client requests server to update A RR in DNS as well as PTR
		 * O: 1 = Server indicates to client that DNS has been updated regardless
		 * E: 1 = Name is in DNS format, i.e. <4>host<6>domain<3>com<0>,
		 *    not "host.domain.com". Format 0 is obsolete.
		 * N: 1 = Client requests server to not update DNS (S must be 0 then)
		 * Two [0] bytes which follow are deprecated and must be 0.
		 */
		client_config.fqdn[OPT_DATA + 0] = 0x1;
		/*client_config.fqdn[OPT_DATA + 1] = 0; - xzalloc did it */
		/*client_config.fqdn[OPT_DATA + 2] = 0; */
	}
	if (opt & OPT_r)
		requested_ip = inet_addr(str_r);
#if ENABLE_FEATURE_UDHCP_PORT
	if (opt & OPT_P) {
		CLIENT_PORT = xatou16(str_P);
		SERVER_PORT = CLIENT_PORT - 1;
	}
#endif
	if (opt & OPT_o)
		client_config.no_default_options = 1;
	while (list_O) {
		char *optstr = llist_pop(&list_O);
		unsigned n = udhcp_option_idx(optstr);
		n = dhcp_optflags[n].code;
		client_config.opt_mask[n >> 3] |= 1 << (n & 7);
	}
	while (list_x) {
		char *optstr = llist_pop(&list_x);
		char *colon = strchr(optstr, ':');
		if (colon)
			*colon = ' ';
		/* now it looks similar to udhcpd's config file line:
		 * "optname optval", using the common routine: */
		udhcp_str2optset(optstr, &client_config.options);
	}

	if (udhcp_read_interface(client_config.interface,
			&client_config.ifindex,
			NULL,
			client_config.client_mac)
	) {
		return 1;
	}

	if (opt & OPT_c) {
		client_config.clientid = alloc_dhcp_option(DHCP_CLIENT_ID, str_c, 0);
	} else if (!(opt & OPT_C)) {
		/* not set and not suppressed, set the default client ID */
		client_config.clientid = alloc_dhcp_option(DHCP_CLIENT_ID, "", 7);
		client_config.clientid[OPT_DATA] = 1; /* type: ethernet */
		memcpy(client_config.clientid + OPT_DATA+1, client_config.client_mac, 6);
	}
#ifdef FEATURE_OP60_OP125_SUPPORT
	if ((opt & OPT_V) && str_V[0] != '\0') {
                char *tmp_data = malloc(strlen(str_V)+1);
                if(tmp_data) {
                    uint8_t *opt60_data = malloc(strlen(str_V)+1);
                    if(opt60_data) {
                        memset(tmp_data, 0, strlen(str_V)+1);
                        strcpy(tmp_data, str_V);
                        
                        opt60_parse(tmp_data, (dh4op60_t *)opt60_data);
                        client_config.vendorclass = opt60_data;
                    }


                    free(tmp_data);
                }
       }
#else /* FEATURE_OP60_OP125_SUPPORT */
	if (str_V[0] != '\0')
		client_config.vendorclass = alloc_dhcp_option(DHCP_VENDOR, str_V, 0);
#endif /* FEATURE_OP60_OP125_SUPPORT */
#if !BB_MMU
	/* on NOMMU reexec (i.e., background) early */
	if (!(opt & OPT_f)) {
		bb_daemonize_or_rexec(0 /* flags */, argv);
		logmode = LOGMODE_NONE;
	}
#endif
	if (opt & OPT_S) {
		openlog(applet_name, LOG_PID, LOG_DAEMON);
		logmode |= LOGMODE_SYSLOG;
	}

#ifdef FEATURE_OP60_OP125_SUPPORT    

        if (opt & OPT_I) {
            char *tmp_data1 = NULL;
            int tmp_len = 0;
            tmp_len = strlen(str_I);
            tmp_data1 = malloc(tmp_len+1);
            if(tmp_data1) {
                uint8_t *opt125_data = malloc(tmp_len+1);
                if(opt125_data) {
                    memset(tmp_data1, 0, tmp_len+1);
                    strcpy(tmp_data1, str_I);
                    opt125_parse(tmp_data1, (dh4op125_t *)opt125_data);
                    client_config.dh4op125 = opt125_data;
                }
            
            
                free(tmp_data1);
            }
            //client_config.dh4op125 = alloc_dhcp_option(DHCP_VENDOR_INFO,str_I,0);   
            }   
	client_config.opt_mask[DHCP_VENDOR_INFO >> 3] |= 1 << (DHCP_VENDOR_INFO & 7);
#endif
	/* Make sure fd 0,1,2 are open */
	bb_sanitize_stdio();
	/* Equivalent of doing a fflush after every \n */
	setlinebuf(stdout);
	/* Create pidfile */
	write_pidfile(client_config.pidfile);
	/* Goes to stdout (unless NOMMU) and possibly syslog */
	bb_info_msg("%s (v"BB_VER") started", applet_name);
	/* Set up the signal pipe */
	udhcp_sp_setup();
	/* We want random_xid to be random... */
	srand(monotonic_us());

	state = INIT_SELECTING;
	udhcp_run_script(NULL, "deconfig");
	change_listen_mode(LISTEN_RAW);
	packet_num = 0;
	timeout = 0;
	already_waited_sec = 0;

#ifdef FEATURE_OP60_OP125_SUPPORT    
        mkdir_conf();
        write_state_to_file(&client_config, state);
#endif
	/* Main event loop. select() waits on signal pipe and possibly
	 * on sockfd.
	 * "continue" statements in code below jump to the top of the loop.
	 */
	for (;;) {
		/* silence "uninitialized!" warning */
		unsigned timestamp_before_wait = timestamp_before_wait;

		/* When running on a bridge, the ifindex may have changed (e.g. if
		 * member interfaces were added/removed or if the status of the
		 * bridge changed).
		 * Workaround: refresh it here before processing the next packet */
		udhcp_read_interface(client_config.interface, &client_config.ifindex, NULL, client_config.client_mac);

		//bb_error_msg("sockfd:%d, listen_mode:%d", sockfd, listen_mode);

		/* Was opening raw or udp socket here
		 * if (listen_mode != LISTEN_NONE && sockfd < 0),
		 * but on fast network renew responses return faster
		 * than we open sockets. Thus this code is moved
		 * to change_listen_mode(). Thus we open listen socket
		 * BEFORE we send renew request (see "case BOUND:"). */

		max_fd = udhcp_sp_fd_set(&rfds, sockfd);

		tv.tv_sec = timeout - already_waited_sec;
		tv.tv_usec = 0;
		retval = 0;
		/* If we already timed out, fall through with retval = 0, else... */
		if ((int)tv.tv_sec > 0) {
			timestamp_before_wait = (unsigned)monotonic_sec();
			log1("Waiting on select...");
			retval = select(max_fd + 1, &rfds, NULL, NULL, &tv);
			if (retval < 0) {
				/* EINTR? A signal was caught, don't panic */
				if (errno == EINTR) {
					already_waited_sec += (unsigned)monotonic_sec() - timestamp_before_wait;
					continue;
				}
				/* Else: an error occured, panic! */
				bb_perror_msg_and_die("select");
			}
		}

		/* If timeout dropped to zero, time to become active:
		 * resend discover/renew/whatever
		 */
		if (retval == 0) {
			/* We will restart the wait in any case */
			already_waited_sec = 0;

			switch (state) {
			case INIT_SELECTING:
				if (!discover_retries || packet_num < discover_retries) {
					if (packet_num == 0)
						xid = random_xid();
					/* broadcast */
					send_discover(xid, requested_ip);
					timeout = discover_timeout;
					packet_num++;
					continue;
				}
 leasefail:
				udhcp_run_script(NULL, "leasefail");
#if BB_MMU /* -b is not supported on NOMMU */
				if (opt & OPT_b) { /* background if no lease */
					bb_info_msg("No lease, forking to background");
					client_background();
					/* do not background again! */
					opt = ((opt & ~OPT_b) | OPT_f);
				} else
#endif
				if (opt & OPT_n) { /* abort if no lease */
					bb_info_msg("No lease, failing");
					retval = 1;
					goto ret;
				}
				/* wait before trying again */
				timeout = tryagain_timeout;
				packet_num = 0;
				continue;
			case REQUESTING:
				if (!discover_retries || packet_num < discover_retries) {
					/* send broadcast select packet */
					send_select(xid, server_addr, requested_ip);
					timeout = discover_timeout;
					packet_num++;
					continue;
				}
				/* Timed out, go back to init state.
				 * "discover...select...discover..." loops
				 * were seen in the wild. Treat them similarly
				 * to "no response to discover" case */
				change_listen_mode(LISTEN_RAW);
				state = INIT_SELECTING;
#ifdef FEATURE_OP60_OP125_SUPPORT    
                                write_state_to_file(&client_config, state);
#endif
				goto leasefail;
			case BOUND:
				/* 1/2 lease passed, enter renewing state */
				state = RENEWING;
#ifdef FEATURE_OP60_OP125_SUPPORT    
                                write_state_to_file(&client_config, state);
#endif
				change_listen_mode(LISTEN_KERNEL);
				log1("Entering renew state");
				/* fall right through */
			case RENEW_REQUESTED: /* manual (SIGUSR1) renew */
			case_RENEW_REQUESTED:
			case RENEWING:
				if (timeout > 60) {
					/* send an unicast renew request */
			/* Sometimes observed to fail (EADDRNOTAVAIL) to bind
			 * a new UDP socket for sending inside send_renew.
			 * I hazard to guess existing listening socket
			 * is somehow conflicting with it, but why is it
			 * not deterministic then?! Strange.
			 * Anyway, it does recover by eventually failing through
			 * into INIT_SELECTING state.
			 */
					send_renew(xid, server_addr, requested_ip);
					timeout >>= 1;
					continue;
				}
				/* Timed out, enter rebinding state */
				log1("Entering rebinding state");
				state = REBINDING;
#ifdef FEATURE_OP60_OP125_SUPPORT    
                                write_state_to_file(&client_config, state);
#endif
				/* fall right through */
			case REBINDING:
				/* Switch to bcast receive */
				change_listen_mode(LISTEN_RAW);
				/* Lease is *really* about to run out,
				 * try to find DHCP server using broadcast */
				if (timeout > 0) {
					/* send a broadcast renew request */
					send_renew(xid, 0 /*INADDR_ANY*/, requested_ip);
					timeout >>= 1;
					continue;
				}
				/* Timed out, enter init state */
				bb_info_msg("Lease lost, entering init state");
				udhcp_run_script(NULL, "deconfig");
				state = INIT_SELECTING;
#ifdef FEATURE_OP60_OP125_SUPPORT    
                                write_state_to_file(&client_config, state);
#endif
				/*timeout = 0; - already is */
				packet_num = 0;
				continue;
			/* case RELEASED: */
			}
			/* yah, I know, *you* say it would never happen */
			timeout = INT_MAX;
			continue; /* back to main loop */
		} /* if select timed out */

		/* select() didn't timeout, something happened */

		/* Is it a signal? */
		/* note: udhcp_sp_read checks FD_ISSET before reading */
		switch (udhcp_sp_read(&rfds)) {
		case SIGUSR1:
			perform_renew();
			if (state == RENEW_REQUESTED)
				goto case_RENEW_REQUESTED;
			/* Start things over */
			packet_num = 0;
			/* Kill any timeouts, user wants this to hurry along */
			timeout = 0;
			continue;
		case SIGUSR2:
			perform_release(requested_ip, server_addr);
			timeout = INT_MAX;
			continue;
		case SIGTERM:
			bb_info_msg("Received SIGTERM");
			if (opt & OPT_R) /* release on quit */
				perform_release(requested_ip, server_addr);
			goto ret0;
		}

		/* Is it a packet? */
		if (listen_mode == LISTEN_NONE || !FD_ISSET(sockfd, &rfds))
			continue; /* no */

		{
			int len;

			/* A packet is ready, read it */
			if (listen_mode == LISTEN_KERNEL)
				len = udhcp_recv_kernel_packet(&packet, sockfd);
			else
				len = udhcp_recv_raw_packet(&packet, sockfd);
			if (len == -1) {
				/* Error is severe, reopen socket */
				bb_info_msg("Read error: %s, reopening socket", strerror(errno));
				sleep(discover_timeout); /* 3 seconds by default */
				change_listen_mode(listen_mode); /* just close and reopen */
			}
			/* If this packet will turn out to be unrelated/bogus,
			 * we will go back and wait for next one.
			 * Be sure timeout is properly decreased. */
			already_waited_sec += (unsigned)monotonic_sec() - timestamp_before_wait;
			if (len < 0)
				continue;
		}

		if (packet.xid != xid) {
			log1("xid %x (our is %x), ignoring packet",
				(unsigned)packet.xid, (unsigned)xid);
			continue;
		}

		/* Ignore packets that aren't for us */
		if (packet.hlen != 6
		 || memcmp(packet.chaddr, client_config.client_mac, 6)
		) {
//FIXME: need to also check that last 10 bytes are zero
			log1("chaddr does not match, ignoring packet"); // log2?
			continue;
		}

		message = udhcp_get_option(&packet, DHCP_MESSAGE_TYPE);
		if (message == NULL) {
			bb_error_msg("no message type option, ignoring packet");
			continue;
		}

		switch (state) {
		case INIT_SELECTING:
			/* Must be a DHCPOFFER to one of our xid's */
			if (*message == DHCPOFFER) {
#ifdef FEATURE_OP60_OP125_SUPPORT
				if(dhcpc_op125_checking(client_config.dh4op125, &packet)) {
				    continue;
				}
#endif
                
		/* TODO: why we don't just fetch server's IP from IP header? */
				temp = udhcp_get_option(&packet, DHCP_SERVER_ID);
				if (!temp) {
					bb_error_msg("no server ID in message");
					continue;
					/* still selecting - this server looks bad */
				}
				/* it IS unaligned sometimes, don't "optimize" */
				move_from_unaligned32(server_addr, temp);
				xid = packet.xid;
				requested_ip = packet.yiaddr;

				/* enter requesting state */
				state = REQUESTING;
#ifdef FEATURE_OP60_OP125_SUPPORT    
                                write_state_to_file(&client_config, state);
#endif
				timeout = 0;
				packet_num = 0;
				already_waited_sec = 0;
			}
			continue;
		case REQUESTING:
		case RENEWING:
		case RENEW_REQUESTED:
		case REBINDING:
			if (*message == DHCPACK) {
#ifdef FEATURE_OP60_OP125_SUPPORT
                                if(dhcpc_op125_checking(client_config.dh4op125, &packet)) {
                                    continue;
                                }
#endif
                
				temp = udhcp_get_option(&packet, DHCP_LEASE_TIME);
				if (!temp) {
					bb_error_msg("no lease time with ACK, using 1 hour lease");
					lease_seconds = 60 * 60;
				} else {
					/* it IS unaligned sometimes, don't "optimize" */
					move_from_unaligned32(lease_seconds, temp);
					lease_seconds = ntohl(lease_seconds);
					lease_seconds &= 0x0fffffff; /* paranoia: must not be prone to overflows */
					if (lease_seconds < 10) /* and not too small */
						lease_seconds = 10;
				}
#if ENABLE_FEATURE_UDHCPC_ARPING
				if (opt & OPT_a) {
/* RFC 2131 3.1 paragraph 5:
 * "The client receives the DHCPACK message with configuration
 * parameters. The client SHOULD perform a final check on the
 * parameters (e.g., ARP for allocated network address), and notes
 * the duration of the lease specified in the DHCPACK message. At this
 * point, the client is configured. If the client detects that the
 * address is already in use (e.g., through the use of ARP),
 * the client MUST send a DHCPDECLINE message to the server and restarts
 * the configuration process..." */
					if (!arpping(packet.yiaddr,
							NULL,
							(uint32_t) 0,
							client_config.client_mac,
							client_config.interface)
					) {
						bb_info_msg("Offered address is in use "
							"(got ARP reply), declining");
						send_decline(xid, server_addr, packet.yiaddr);

						if (state != REQUESTING)
							udhcp_run_script(NULL, "deconfig");
						change_listen_mode(LISTEN_RAW);
						state = INIT_SELECTING;
#ifdef FEATURE_OP60_OP125_SUPPORT    
                                                write_state_to_file(&client_config, state);
#endif
						requested_ip = 0;
						timeout = tryagain_timeout;
						packet_num = 0;
						already_waited_sec = 0;
						continue; /* back to main loop */
					}
				}
#endif
				/* enter bound state */
				timeout = lease_seconds / 2;
				{
					struct in_addr temp_addr;
					temp_addr.s_addr = packet.yiaddr;
					bb_info_msg("Lease of %s obtained, lease time %u",
						inet_ntoa(temp_addr), (unsigned)lease_seconds);
				}
				requested_ip = packet.yiaddr;
#ifdef FEATURE_OP60_OP125_SUPPORT
                dhcpc_info_write(&packet);
#endif
				udhcp_run_script(&packet, state == REQUESTING ? "bound" : "renew");

				state = BOUND;
#ifdef FEATURE_OP60_OP125_SUPPORT    
                                write_state_to_file(&client_config, state);
#endif
				change_listen_mode(LISTEN_NONE);
				if (opt & OPT_q) { /* quit after lease */
					if (opt & OPT_R) /* release on quit */
						perform_release(requested_ip, server_addr);
					goto ret0;
				}
				/* future renew failures should not exit (JM) */
				opt &= ~OPT_n;
#if BB_MMU /* NOMMU case backgrounded earlier */
				if (!(opt & OPT_f)) {
					client_background();
					/* do not background again! */
					opt = ((opt & ~OPT_b) | OPT_f);
				}
#endif
				already_waited_sec = 0;
				continue; /* back to main loop */
			}
			if (*message == DHCPNAK) {
				/* return to init state */
				bb_info_msg("Received DHCP NAK");
				udhcp_run_script(&packet, "nak");
				if (state != REQUESTING)
					udhcp_run_script(NULL, "deconfig");
				change_listen_mode(LISTEN_RAW);
				sleep(3); /* avoid excessive network traffic */
				state = INIT_SELECTING;
#ifdef FEATURE_OP60_OP125_SUPPORT    
                                write_state_to_file(&client_config, state);
#endif
				requested_ip = 0;
				timeout = 0;
				packet_num = 0;
				already_waited_sec = 0;
			}
			continue;
		/* case BOUND: - ignore all packets */
		/* case RELEASED: - ignore all packets */
		}
		/* back to main loop */
	} /* for (;;) - main loop ends */

 ret0:
	retval = 0;
 ret:
	/*if (client_config.pidfile) - remove_pidfile has its own check */
		remove_pidfile(client_config.pidfile);
	return retval;
}
