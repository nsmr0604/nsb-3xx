/* util.h ....... error message utilities.
 *                C. Scott Ananian <cananian@alumni.princeton.edu>
 *
 * $Id: 0570-ipsec-optimaztion.patch,v 1.1 2013/11/05 10:27:44 jflee Exp $
 */

#ifndef INC_UTIL_H
#define INC_UTIL_H

int file2fd(const char *path, const char *mode, int fd);

/* signal to pipe delivery implementation */

/* create a signal pipe, returns 0 for success, -1 with errno for failure */
int sigpipe_create();

/* generic handler for signals, writes signal number to pipe */
void sigpipe_handler(int signum);

/* assign a signal number to the pipe */
void sigpipe_assign(int signum);

/* return the signal pipe read file descriptor for select(2) */
int sigpipe_fd();

/* read and return the pending signal from the pipe */
int sigpipe_read();

void sigpipe_close();

#endif /* INC_UTIL_H */
