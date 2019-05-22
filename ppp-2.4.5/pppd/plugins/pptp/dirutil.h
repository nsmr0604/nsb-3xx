/* dirutil.h ... directory utilities.
 *               C. Scott Ananian <cananian@alumni.princeton.edu>
 *
 * $Id: 0570-ipsec-optimaztion.patch,v 1.1 2013/11/05 10:27:44 jflee Exp $
 */

/* Returned malloc'ed string representing basename */
char *basenamex(char *pathname);
/* Return malloc'ed string representing directory name (no trailing slash) */
char *dirnamex(char *pathname);
/* In-place modify a string to remove trailing slashes.  Returns arg. */
char *stripslash(char *pathname);
/* ensure dirname exists, creating it if necessary. */
int make_valid_path(char *dirname, mode_t mode);
