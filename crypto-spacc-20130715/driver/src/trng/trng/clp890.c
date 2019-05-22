#include "elpclp890_hw.h"
#include "elpclp890.h"

void elpclp890_setup(struct elpclp890_state *clp890, uint32_t *regbase)
{
   clp890->regbase = regbase;
}

int elpclp890_set_ps(struct elpclp890_state *clp890, void *ps, unsigned len)
{
   return 0;
}
