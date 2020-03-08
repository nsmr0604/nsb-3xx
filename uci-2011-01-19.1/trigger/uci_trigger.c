#include <sys/types.h>
#include <sys/time.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <lualib.h>
#include <lauxlib.h>
#include "uci.h"

// #define DEBUG

static int refcount = 0;
static lua_State *gL = NULL;
static bool prepared = false;

struct trigger_set_op {
	struct uci_package *p;
	struct uci_delta *h;
};

static int trigger_set(lua_State *L)
{
	struct trigger_set_op *so =
		(struct trigger_set_op *)lua_touserdata(L, 1);
	struct uci_package *p = so->p;
	struct uci_delta *h = so->h;
	struct uci_context *ctx = p->ctx;

	/* ignore non-standard savedirs/configdirs
	 * in order to not trigger events on uci state changes */
	if (strcmp(ctx->savedir, UCI_SAVEDIR) ||
		strcmp(ctx->confdir, UCI_CONFDIR))
		return 0;

	if (!prepared) {
		lua_getglobal(L, "require");
		lua_pushstring(L, "uci.trigger");
		lua_call(L, 1, 0);

		lua_getglobal(L, "uci");
		lua_getfield(L, -1, "trigger");
		lua_getfield(L, -1, "load_modules");
		lua_call(L, 0, 0);
		prepared = true;
	} else {
		lua_getglobal(L, "uci");
		lua_getfield(L, -1, "trigger");
	}

	lua_getfield(L, -1, "set");
	lua_createtable(L, 4, 0);

	lua_pushstring(L, p->e.name);
	lua_rawseti(L, -2, 1);
	if (h->section) {
		lua_pushstring(L, h->section);
		lua_rawseti(L, -2, 2);
	}
	if (h->e.name) {
		lua_pushstring(L, h->e.name);
		lua_rawseti(L, -2, 3);
	}
	if (h->value) {
		lua_pushstring(L, h->value);
		lua_rawseti(L, -2, 4);
	}
	lua_call(L, 1, 0);
	lua_pop(L, 2);
	return 0;
}

#ifdef DEBUG

static int report (lua_State *L, int status) {
	if (status && !lua_isnil(L, -1)) {
		const char *msg = lua_tostring(L, -1);
		if (msg == NULL) msg = "(error object is not a string)";
		fprintf(stderr, "ERROR: %s\n", msg);
		lua_pop(L, 1);
	}
	return status;
}

#else

static inline int report(lua_State *L, int status) {
	return status;
}

#endif

static void trigger_set_hook(const struct uci_hook_ops *ops, struct uci_package *p, struct uci_delta *h)
{
	struct trigger_set_op so;

	so.p = p;
	so.h = h;
	report(gL, lua_cpcall(gL, &trigger_set, &so));
}

static struct uci_hook_ops hook = {
	.set = trigger_set_hook,
};

static int trigger_attach(struct uci_context *ctx)
{
	if (!gL) {
		gL = luaL_newstate();
		if (!gL)
			return -1;
		luaL_openlibs(gL);

		refcount++;
	}
	uci_add_hook(ctx, &hook);
	return 0;
}

static void trigger_detach(struct uci_context *ctx)
{
	if (gL && (--refcount <= 0)) {
#if 0 /* it will cause segmentation fault, mark off temporarily */
		lua_close(gL);
#endif
		gL = NULL;
		refcount = 0;
		prepared = false;
	}
}

struct uci_plugin_ops uci_plugin = {
	.attach = trigger_attach,
	.detach = trigger_detach,
};