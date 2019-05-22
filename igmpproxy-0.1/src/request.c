/*
**  igmpproxy - IGMP proxy based multicast router 
**  Copyright (C) 2005 Johnny Egeland <johnny@rlo.org>
**
**  This program is free software; you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation; either version 2 of the License, or
**  (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program; if not, write to the Free Software
**  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
**
**----------------------------------------------------------------------------
**
**  This software is derived work from the following software. The original
**  source code has been modified from it's original state by the author
**  of igmpproxy.
**
**  smcroute 0.92 - Copyright (C) 2001 Carsten Schill <carsten@cschill.de>
**  - Licensed under the GNU General Public License, version 2
**  
**  mrouted 3.9-beta3 - COPYRIGHT 1989 by The Board of Trustees of 
**  Leland Stanford Junior University.
**  - Original license can be found in the Stanford.txt file.
**
*/
/**
*   request.c 
*
*   Functions for recieveing and processing IGMP requests.
*
*/

#ifdef CONFIG_CS75XX_DATA_PLANE_MULTICAST
#include <mach/cs_mcast.h>
#endif //CONFIG_CS75XX_DATA_PLANE_MULTICAST
#include "igmpproxy.h"
#ifdef CONFIG_LYNXE_KERNEL_HOOK
#include <net/if.h>
#include "khook.h"
#include "route.h"
#include "mcast.h"
#endif

// Prototypes...
void sendGroupSpecificMemberQuery(void *argument);  
    
typedef struct {
    uint32_t      group;
    uint32_t      vifAddr;
    short       started;
} GroupVifDesc;

#ifdef CONFIG_CS75XX_DATA_PLANE_MULTICAST
/* TODO:
    Please modify the follow code. 
    if the G2 physical interface name is not "eth0/eth1/eth2",
    or bridge interface name is not "br-lan"
 */
char *cs_network_interface_name[] = { "eth0", "eth1", "eth2", "br-lan", 0 };
typedef enum {
	CS_INTERFACE_ETH0,
	CS_INTERFACE_ETH1,
	CS_INTERFACE_ETH2,
	CS_INTERFACE_BR_LAN,
	CS_INTERFACE_NONE = 0xFF,
} cs_network_interface_e;

unsigned char csGetPortId(char *pdevName, cs_port_id_t *port_id)
{
    if (!strcmp (pdevName, cs_network_interface_name[CS_INTERFACE_ETH0])) {
        /* Fill G2 GMAC0 port id */
        *port_id = 0;
    } else if (!strcmp (pdevName, cs_network_interface_name[CS_INTERFACE_ETH1])) {
        /* Fill G2 GMAC1 port id */
        *port_id = 1;
    } else if (!strcmp (pdevName, cs_network_interface_name[CS_INTERFACE_ETH2])) {
        /* Fill G2 GMAC2 port id */
        *port_id = 2;
    } else if (!strcmp (pdevName, cs_network_interface_name[CS_INTERFACE_BR_LAN])) {
        /* Don't create hash here if device is bridge */
        return 1;
    } else {
        /* Fill others device port id to CPU */
        *port_id = 3;
    }

    return 0;
}
#endif //CONFIG_CS75XX_DATA_PLANE_MULTICAST


/**
*   Handles incoming membership reports, and
*   appends them to the routing table.
*/
void acceptGroupReport(uint32_t src, uint32_t group, uint8_t type) {
    struct IfDesc  *sourceVif;

    // Sanitycheck the group adress...
    if(!IN_MULTICAST( ntohl(group) )) {
        my_log(LOG_WARNING, 0, "The group address %s is not a valid Multicast group.",
            inetFmt(group, s1));
        return;
    }

    // Find the interface on which the report was recieved.
    sourceVif = getIfByAddress( src );
    if(sourceVif == NULL) {
        my_log(LOG_WARNING, 0, "No interfaces found for source %s",
            inetFmt(src,s1));
        return;
    }

    if(sourceVif->InAdr.s_addr == src) {
        my_log(LOG_NOTICE, 0, "The IGMP message was from myself. Ignoring.");
        return;
    }

    // We have a IF so check that it's an downstream IF.
    if(sourceVif->state == IF_STATE_DOWNSTREAM) {

        my_log(LOG_DEBUG, 0, "Should insert group %s (from: %s) to route table. Vif Ix : %d",
            inetFmt(group,s1), inetFmt(src,s2), sourceVif->index);

#ifdef CONFIG_CS75XX_DATA_PLANE_MULTICAST
	    cs_port_id_t port_id;
	    unsigned char status;
	    status = csGetPortId(sourceVif->Name, &port_id);
        if ((status != 0) || (port_id > 2)) {
#endif //CONFIG_CS75XX_DATA_PLANE_MULTICAST
        // The membership report was OK... Insert it into the route table..
        insertRoute(group, sourceVif->index);

#ifdef CONFIG_CS75XX_DATA_PLANE_MULTICAST
        }
#endif //CONFIG_CS75XX_DATA_PLANE_MULTICAST

    } else {
        // Log the state of the interface the report was recieved on.
        my_log(LOG_INFO, 0, "Mebership report was recieved on %s. Ignoring.",
            sourceVif->state==IF_STATE_UPSTREAM?"the upstream interface":"a disabled interface");
    }

}

/**
*   Recieves and handles a group leave message.
*/
void acceptLeaveMessage(uint32_t src, uint32_t group) {
    struct IfDesc   *sourceVif;
    
    my_log(LOG_DEBUG, 0,
	    "Got leave message from %s to %s. Starting last member detection.",
	    inetFmt(src, s1), inetFmt(group, s2));

    // Sanitycheck the group adress...
    if(!IN_MULTICAST( ntohl(group) )) {
        my_log(LOG_WARNING, 0, "The group address %s is not a valid Multicast group.",
            inetFmt(group, s1));
        return;
    }

    // Find the interface on which the report was recieved.
    sourceVif = getIfByAddress( src );
    if(sourceVif == NULL) {
        my_log(LOG_WARNING, 0, "No interfaces found for source %s",
            inetFmt(src,s1));
        return;
    }

    // We have a IF so check that it's an downstream IF.
    if(sourceVif->state == IF_STATE_DOWNSTREAM) {

        GroupVifDesc   *gvDesc;
        gvDesc = (GroupVifDesc*) malloc(sizeof(GroupVifDesc));

        // Tell the route table that we are checking for remaining members...
        setRouteLastMemberMode(group);

        // Call the group spesific membership querier...
        gvDesc->group = group;
        gvDesc->vifAddr = sourceVif->InAdr.s_addr;
        gvDesc->started = 0;

        sendGroupSpecificMemberQuery(gvDesc);

    } else {
        // just ignore the leave request...
        my_log(LOG_DEBUG, 0, "The found if for %s was not downstream. Ignoring leave request.", inetFmt(src, s1));
    }
}

/**
*   Sends a group specific member report query until the 
*   group times out...
*/
void sendGroupSpecificMemberQuery(void *argument) {
    struct  Config  *conf = getCommonConfig();

    // Cast argument to correct type...
    GroupVifDesc   *gvDesc = (GroupVifDesc*) argument;

    if(gvDesc->started) {
        // If aging returns false, we don't do any further action...
        if(!lastMemberGroupAge(gvDesc->group)) {
            return;
        }
    } else {
        gvDesc->started = 1;
    }

    // Send a group specific membership query...
    sendIgmp(gvDesc->vifAddr, gvDesc->group, 
             IGMP_MEMBERSHIP_QUERY,
             conf->lastMemberQueryInterval * IGMP_TIMER_SCALE, 
             gvDesc->group, 0);

    my_log(LOG_DEBUG, 0, "Sent membership query from %s to %s. Delay: %d",
        inetFmt(gvDesc->vifAddr,s1), inetFmt(gvDesc->group,s2),
        conf->lastMemberQueryInterval);

    // Set timeout for next round...
    timer_setTimer(conf->lastMemberQueryInterval, sendGroupSpecificMemberQuery, gvDesc);

}


/**
*   Sends a general membership query on downstream VIFs
*/
void sendGeneralMembershipQuery() {
    struct  Config  *conf = getCommonConfig();
    struct  IfDesc  *Dp;
    int             Ix;

    // Loop through all downstream vifs...
    for ( Ix = 0; (Dp = getIfByIx(Ix)); Ix++ ) {
        if ( Dp->InAdr.s_addr && ! (Dp->Flags & IFF_LOOPBACK) ) {
            if(Dp->state == IF_STATE_DOWNSTREAM) {
                // Send the membership query...
                sendIgmp(Dp->InAdr.s_addr, allhosts_group, 
                         IGMP_MEMBERSHIP_QUERY,
                         conf->queryResponseInterval * IGMP_TIMER_SCALE, 0, 0);
                
                my_log(LOG_DEBUG, 0,
			"Sent membership query from %s to %s. Delay: %d",
			inetFmt(Dp->InAdr.s_addr,s1),
			inetFmt(allhosts_group,s2),
			conf->queryResponseInterval);
            }
        }
    }

    // Install timer for aging active routes.
    timer_setTimer(conf->queryResponseInterval, ageActiveRoutes, NULL);

    // Install timer for next general query...
    if(conf->startupQueryCount>0) {
        // Use quick timer...
        timer_setTimer(conf->startupQueryInterval, sendGeneralMembershipQuery, NULL);
        // Decrease startup counter...
        conf->startupQueryCount--;
    } 
    else {
        // Use slow timer...
        timer_setTimer(conf->queryInterval, sendGeneralMembershipQuery, NULL);
    }


}
 
#ifdef CONFIG_CORTINA_DATA_PLANE_MULTICAST
/* src: member IP, group: MC group IP */
void csAcceptGroupReport(uint32_t src, uint32_t group)
{
    struct IfDesc *sourceVif;
    struct IfDesc *Dp;
#ifdef CONFIG_CS75XX_DATA_PLANE_MULTICAST
    unsigned int Ix;
	cs_mcast_member_t cs_entry;
	cs_status_t retStatus;
	cs_port_id_t port_id;
#endif /* CONFIG_CS75XX_DATA_PLANE_MULTICAST */  
#ifdef CONFIG_LYNXE_KERNEL_HOOK
    cs_ip_afi_t afi;
    cs_int16_t ifindex;
    cs_uint32_t intf_id;
    cs_status_t retStatus;
    cs_l3_intf_t intf;
    cs_l3_mcast_entry_t cs_entry;
#endif /* CONFIG_LYNXE_KERNEL_HOOK */  

    // Sanitycheck the group adress...
    if (!IN_MULTICAST( ntohl(group) )) {
        my_log(LOG_WARNING, 0, "The group address %s is not a valid Multicast group.",
            inetFmt(group, s1));
        return;
    }

    // Find the interface on which the report was recieved.
    sourceVif = getIfByAddress( src );
    if (sourceVif == NULL) {
        my_log(LOG_WARNING, 0, "No interfaces found for source %s",
            inetFmt(src,s1));
        return;
    }

    if (sourceVif->InAdr.s_addr == src) {
        my_log(LOG_NOTICE, 0, "The IGMP message was from myself. Ignoring.");
        return;
    }

    my_log(LOG_DEBUG, 0, "%s::Should insert group %s (from: %s) to route table. Vif Ix : %d",
        __func__, inetFmt(group,s1), inetFmt(src,s2), sourceVif->index);

#ifdef CONFIG_CS75XX_DATA_PLANE_MULTICAST
	retStatus = cs_l2_mcast_wan_port_id_get(0, &port_id);
	if ((retStatus != CS_E_OK) || (port_id == CS_DEFAULT_WAN_PORT_ID)) {
        /* Don't create hash here if device is bridge */
        return;
	}

    if (csGetPortId(sourceVif->Name, &port_id) != 0)
    {
        /* Don't create hash here if device is bridge */
        return;
    }
    
	memset(&cs_entry, 0, sizeof(cs_entry));
	cs_entry.afi = CS_IPV4;
	cs_entry.sub_port = port_id;
	cs_entry.grp_addr.afi = CS_IPV4;
	cs_entry.grp_addr.ip_addr.ipv4_addr = group;
	cs_entry.grp_addr.addr_len = sizeof(cs_entry.grp_addr.ip_addr.ipv4_addr);
	cs_entry.mode = CS_MCAST_EXCLUDE;
	cs_entry.src_num = 1;

	retStatus = cs_l2_mcast_member_add(0, cs_entry.sub_port, &cs_entry);
#endif /* CONFIG_CS75XX_DATA_PLANE_MULTICAST */  
#ifdef CONFIG_LYNXE_KERNEL_HOOK

    ifindex = if_nametoindex(sourceVif->Name);
    if (ifindex)
    {
        afi = CS_IPV4;
        retStatus = cs_kh_l3_get_intf_id_by_ifindex(0, afi, ifindex, &intf_id);
        if (retStatus == CS_E_OK) {
            intf.intf_id = intf_id;
            intf.ip_addr.afi = afi;
            my_log(LOG_INFO, 0, "%s cs_l3_intf_get intf_id %d, afi %d",
               sourceVif->Name, intf.intf_id, intf.ip_addr.afi);
            retStatus = cs_l3_intf_get(0, &intf);
            my_log(LOG_INFO, 0, "cs_l3_intf_get retStatus %d", retStatus);
            if (retStatus == CS_E_OK) {
                memset(&cs_entry, 0, sizeof(cs_entry));
                cs_entry.mcast_vlan = 0;
                cs_entry.grp_ip_addr.afi = CS_IPV4;
                cs_entry.grp_ip_addr.ip_addr.ipv4_addr = ntohl(group);
                cs_entry.filter_mode = CS_MCAST_EXCLUDE;
                cs_entry.src_num = 0;
                retStatus = cs_l3_mcast_member_add(0, intf_id, &cs_entry);
            }
        }
    }
#endif /* CONFIG_LYNXE_KERNEL_HOOK */  
    return;
} /* csAcceptGroupReport() */

void csAcceptLeaveMessage(uint32_t src, uint32_t group)
{
    struct IfDesc *sourceVif;
    struct IfDesc *Dp;
#ifdef CONFIG_CS75XX_DATA_PLANE_MULTICAST
    unsigned int Ix;
	cs_mcast_member_t cs_entry;
	cs_status_t retStatus;
 	cs_port_id_t port_id;
#endif /* CONFIG_CS75XX_DATA_PLANE_MULTICAST */  

#ifdef CONFIG_LYNXE_KERNEL_HOOK
    cs_ip_afi_t afi;
    cs_int16_t ifindex;
    cs_uint32_t intf_id;
    cs_status_t retStatus;
    cs_l3_intf_t intf;
    cs_l3_mcast_entry_t cs_entry;
#endif /* CONFIG_LYNXE_KERNEL_HOOK */  

    // Sanitycheck the group adress...
    if (!IN_MULTICAST( ntohl(group) )) {
        my_log(LOG_WARNING, 0, "The group address %s is not a valid Multicast group.",
            inetFmt(group, s1));
        return;
    }

    // Find the interface on which the report was recieved.
    sourceVif = getIfByAddress( src );
    if (sourceVif == NULL) {
        my_log(LOG_WARNING, 0, "No interfaces found for source %s",
            inetFmt(src,s1));
        return;
    }

#ifdef CONFIG_CS75XX_DATA_PLANE_MULTICAST
	retStatus = cs_l2_mcast_wan_port_id_get(0, &port_id);
	if ((retStatus != CS_E_OK) || (port_id == CS_DEFAULT_WAN_PORT_ID)) {
        /* Don't create hash here if device is bridge */
        return;
	}

    if (csGetPortId(sourceVif->Name, &port_id) != 0)
    {
        /* Don't create hash here if device is bridge */
        return;
    }

	memset(&cs_entry, 0, sizeof(cs_entry));
	cs_entry.afi = CS_IPV4;
	cs_entry.sub_port = port_id;
	cs_entry.grp_addr.afi = CS_IPV4;
	cs_entry.grp_addr.ip_addr.ipv4_addr = group;
	cs_entry.grp_addr.addr_len = sizeof(cs_entry.grp_addr.ip_addr.ipv4_addr);
	cs_entry.mode = CS_MCAST_EXCLUDE;
	cs_entry.src_num = 1;

	retStatus = cs_l2_mcast_member_delete(0, cs_entry.sub_port, &cs_entry);
#endif //CONFIG_CS75XX_DATA_PLANE_MULTICAST


#ifdef CONFIG_LYNXE_KERNEL_HOOK
    ifindex = if_nametoindex(sourceVif->Name);
    if (ifindex)
    {
        afi = CS_IPV4;
        retStatus = cs_kh_l3_get_intf_id_by_ifindex(0, afi, ifindex, &intf_id);
        if (retStatus == CS_E_OK) {
            intf.intf_id = intf_id;
            intf.ip_addr.afi = afi;
            retStatus = cs_l3_intf_get(0, &intf);
            if (retStatus == CS_E_OK) {
                memset(&cs_entry, 0, sizeof(cs_entry));
                cs_entry.mcast_vlan = 0;
                cs_entry.grp_ip_addr.afi = CS_IPV4;
                cs_entry.grp_ip_addr.ip_addr.ipv4_addr = ntohl(group);
                cs_entry.filter_mode = CS_MCAST_EXCLUDE;
                cs_entry.src_num = 0;
                retStatus = cs_l3_mcast_member_delete(0, intf_id, &cs_entry);
            }
        }
    }
#endif /* CONFIG_LYNXE_KERNEL_HOOK */
    return;
} /* csAcceptLeaveMessage() */
#endif /* CONFIG_CORTINA_DATA_PLANE_MULTICAST */

