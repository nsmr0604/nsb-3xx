#ifndef PKA_COMMON_H_
#define PKA_COMMON_H_

void pka_tool_init(const char *name, int argc, char **argv);
const char *pka_tool_name(void);
void pka_tool_version(void);

int pka_tool_vmsg(const char *fmt, va_list ap);
int pka_tool_msg(const char *fmt, ...);
int pka_tool_verr(int err, const char *fmt, va_list ap);
int pka_tool_err(int err, const char *fmt, ...);

#endif
