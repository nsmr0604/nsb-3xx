/* This file was automatically generated */
#include <mrd/mrd.h>
extern "C" mrd_module *mrd_module_init_mld(void *, mrd *);
extern "C" mrd_module *mrd_module_init_pim(void *, mrd *);
extern "C" mrd_module *mrd_module_init_console(void *, mrd *);
void mrd::add_static_modules() {
	m_static_modules["mld"] = &mrd_module_init_mld;
	m_static_modules["pim"] = &mrd_module_init_pim;
	m_static_modules["console"] = &mrd_module_init_console;
}
