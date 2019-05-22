/* pptp_callmgr.h ... Call manager for PPTP connections.
 *                    Handles TCP port 1723 protocol.
 *                    C. Scott Ananian <cananian@alumni.princeton.edu>
 *
 * $Id: 0570-ipsec-optimaztion.patch,v 1.1 2013/11/05 10:27:44 jflee Exp $
 */

#define PPTP_SOCKET_PREFIX "/var/run/pptp/"

int callmgr_main(struct in_addr inetaddr,
		char phonenr[],
		int window,
		int pcallid);

void callmgr_name_unixsock(struct sockaddr_un *where,
			   struct in_addr inetaddr,
			   struct in_addr localbind);
