#include "cs75xx.h"

#ifdef CONFIG_CS752X_PROC
#include <mach/platform.h>
#endif

extern unsigned int cs_acp_enable;

dma_addr_t cs_dma_map_single(struct device *dev, void *ptr,
                        size_t size, enum dma_data_direction dir)
{
#ifdef CONFIG_CS752X_PROC
	if (cs_acp_enable & CS75XX_ACP_ENABLE_CRYPT) {
		return (virt_to_phys(ptr) | GOLDENGATE_ACP_BASE);
	} else
#endif
		return dma_map_single(dev, ptr, size, dir);
}

void cs_dma_unmap_single(struct device *dev, dma_addr_t addr,
                        size_t size, enum dma_data_direction dir)
{
#ifdef CONFIG_CS752X_PROC
	if (cs_acp_enable & CS75XX_ACP_ENABLE_CRYPT) {
		/* don't need to unmap */
	} else
#endif
		return dma_unmap_single(dev, addr, size, dir);
}

int cs_dma_map_sg(struct device *dev, struct scatterlist *sg, int nents,
		enum dma_data_direction dir)
{
	int i;
	struct scatterlist *s;

#ifdef CONFIG_CS752X_PROC
	if (cs_acp_enable & CS75XX_ACP_ENABLE_CRYPT) {
		for_each_sg(sg, s, nents, i) {
			s->dma_address = cs_dma_map_single(dev, sg_virt(s), s->length, dir);
		}
		return nents;
	} else
#endif
		return dma_map_sg(dev, sg, nents, dir);
}

void cs_dma_unmap_sg(struct device *dev, struct scatterlist *sg, int nents,
		enum dma_data_direction dir)
{
#ifdef CONFIG_CS752X_PROC
	if (cs_acp_enable & CS75XX_ACP_ENABLE_CRYPT) {
		/* don't need to unmap */
	} else
#endif
		dma_unmap_sg(dev, sg, nents, dir);
}

void cs_dma_sync_single_for_cpu(struct device *dev, dma_addr_t addr,
				size_t size, enum dma_data_direction dir)
{
#ifdef CONFIG_CS752X_PROC
	if (cs_acp_enable & CS75XX_ACP_ENABLE_CRYPT) {
		/* don't need to sync */
	} else
#endif
		dma_sync_single_for_cpu(dev, addr, size, dir);

}

void cs_dma_sync_single_for_device(struct device *dev, dma_addr_t addr,
				size_t size, enum dma_data_direction dir)
{
#ifdef CONFIG_CS752X_PROC
	if (cs_acp_enable & CS75XX_ACP_ENABLE_CRYPT) {
		/* don't need to sync */
	} else
#endif
		dma_sync_single_for_device(dev, addr, size, dir);

}

void cs_dma_sync_sg_for_cpu(struct device *dev, struct scatterlist *sg,
			int nents, enum dma_data_direction dir)
{
#ifdef CONFIG_CS752X_PROC
	if (cs_acp_enable & CS75XX_ACP_ENABLE_CRYPT) {
		/* don't need to sync */
	} else
#endif
		dma_sync_sg_for_cpu(dev, sg, nents, dir);
}

void cs_dma_sync_sg_for_device(struct device *dev, struct scatterlist *sg,
			int nents, enum dma_data_direction dir)
{
#ifdef CONFIG_CS752X_PROC
	if (cs_acp_enable & CS75XX_ACP_ENABLE_CRYPT) {
		/* don't need to sync */
	} else
#endif
		dma_sync_sg_for_device(dev, sg, nents, dir);
}

