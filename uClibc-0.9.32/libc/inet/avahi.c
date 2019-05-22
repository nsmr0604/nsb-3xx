/*
 *
 * (C) 2011 Weinmann GmbH, Hamburg, Germany
 *
 * Author: Nikolaus Voss <n.voss at weinmann.de <http://lists.busybox.net/mailman/listinfo/uclibc>>
 *
 * Derived from avahi.c / nss-mdns by Lennart Poettering.
 * Copyright 2004-2007 Lennart Poettering <mzaffzqaf (at) 0pointer (dot) de>
 *
 * Licensed under the LGPL v2.1, see the file COPYING.LIB in this tarball.
 *
 */

#include <arpa/inet.h>
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

int __avahi_resolve_name(int af, const char* name, void* data) attribute_hidden;

int __avahi_resolve_address(int af, const void *data, char* name,
			    size_t name_len) attribute_hidden;

#define WHITESPACE " \t"

static int set_cloexec(int fd)
{
	int n;

	assert(fd >= 0);

	if ((n = fcntl(fd, F_GETFD)) < 0)
		return -1;

	if (n & FD_CLOEXEC)
		return 0;

	return fcntl(fd, F_SETFD, n|FD_CLOEXEC);
}

static FILE *open_socket(void)
{
	int fd = -1;
	struct sockaddr_un sa;
	FILE *f = NULL;

	if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
		goto fail;

	set_cloexec(fd);

	memset(&sa, 0, sizeof(sa));
	sa.sun_family = AF_UNIX;
	strncpy(sa.sun_path, __UCLIBC_AVAHI_SOCKET_PATH__,
		sizeof(sa.sun_path)-1);
	sa.sun_path[sizeof(sa.sun_path)-1] = 0;

	if (connect(fd, (struct sockaddr*) &sa, sizeof(sa)) < 0)
		goto fail;

	if (!(f = fdopen(fd, "r+")))
		goto fail;

	return f;

fail:
	if (fd >= 0)
		close(fd);

	return NULL;
}

int attribute_hidden __avahi_resolve_name(int af, const char* name, void* data)
{
	FILE *f = NULL;
	char *p;
	int ret = -1;
	char ln[256];

	if (af != AF_INET && af != AF_INET6)
		goto finish;

	if (!(f = open_socket()))
		goto finish;

	fprintf(f, "RESOLVE-HOSTNAME%s %s\n", af == AF_INET ? "-IPV4" : "-IPV6", name);
	fflush(f);

	if (!(fgets(ln, sizeof(ln), f)))
		goto finish;

	if (ln[0] != '+') {
		ret = 1;
		goto finish;
	}

	p = ln+1;
	p += strspn(p, WHITESPACE);

	/* Skip interface */
	p += strcspn(p, WHITESPACE);
	p += strspn(p, WHITESPACE);

	/* Skip protocol */
	p += strcspn(p, WHITESPACE);
	p += strspn(p, WHITESPACE);

	/* Skip host name */
	p += strcspn(p, WHITESPACE);
	p += strspn(p, WHITESPACE);

	/* Cut off end of line */
	*(p + strcspn(p, "\n\r\t ")) = 0;

	if (inet_pton(af, p, data) <= 0)
		goto finish;

	ret = 0;

finish:

	if (f)
		fclose(f);

	return ret;
}

int attribute_hidden __avahi_resolve_address(int af, const void *data,
					     char* name, size_t name_len)
{
	FILE *f = NULL;
	char *p;
	int ret = -1;
	char a[256], ln[256];

	if (af != AF_INET && af != AF_INET6)
		goto finish;

	if (!(f = open_socket()))
		goto finish;

	fprintf(f, "RESOLVE-ADDRESS %s\n", inet_ntop(af, data, a, sizeof(a)));

	if (!(fgets(ln, sizeof(ln), f)))
		goto finish;

	if (ln[0] != '+') {
		ret = 1;
		goto finish;
	}

	p = ln+1;
	p += strspn(p, WHITESPACE);

	/* Skip interface */
	p += strcspn(p, WHITESPACE);
	p += strspn(p, WHITESPACE);

	/* Skip protocol */
	p += strcspn(p, WHITESPACE);
	p += strspn(p, WHITESPACE);

	/* Cut off end of line */
	*(p + strcspn(p, "\n\r\t ")) = 0;

	strncpy(name, p, name_len-1);
	name[name_len-1] = 0;

	ret = 0;

finish:

	if (f)
		fclose(f);

	return ret;
}
