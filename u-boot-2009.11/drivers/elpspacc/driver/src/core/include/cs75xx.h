#ifndef __CS75XX_H__
#define __CS75XX_H__

#include <linux/dma-mapping.h>

dma_addr_t cs_dma_map_single(struct device *dev, void *ptr,
			size_t size, enum dma_data_direction dir);
void cs_dma_unmap_single(struct device *dev, dma_addr_t addr,
			size_t size, enum dma_data_direction dir);

int cs_dma_map_sg(struct device *dev, struct scatterlist *sg, int nents,
		enum dma_data_direction dir);
void cs_dma_unmap_sg(struct device *dev, struct scatterlist *sg, int nents,
		enum dma_data_direction dir);

void cs_dma_sync_single_for_cpu(struct device *dev, dma_addr_t addr,
				size_t size, enum dma_data_direction dir);
void cs_dma_sync_single_for_device(struct device *dev, dma_addr_t addr,
				size_t size, enum dma_data_direction dir);

void cs_dma_sync_sg_for_cpu(struct device *dev, struct scatterlist *sg,
			int nents, enum dma_data_direction dir);
void cs_dma_sync_sg_for_device(struct device *dev, struct scatterlist *sg,
			int nents, enum dma_data_direction dir);


#define DMA_MAP_SINGLE(d,a,s,r)			cs_dma_map_single(d,a,s,r)
#define DMA_UNMAP_SINGLE(d,a,s,r)		cs_dma_unmap_single(d,a,s,r)

#define DMA_MAP_SG(d,s,n,r)			cs_dma_map_sg(d,s,n,r)
#define DMA_UNMAP_SG(d,s,n,r)			cs_dma_unmap_sg(d,s,n,r)

#define DMA_SYNC_SINGLE_FOR_CPU(d,a,s,r)	cs_dma_sync_single_for_cpu(d,a,s,r)
#define DMA_SYNC_SINGLE_FOR_DEVICE(d,a,s,r)	cs_dma_sync_single_for_device(d,a,s,r)

#define DMA_SYNC_SG_FOR_CPU(d,s,n,r)		cs_dma_sync_sg_for_cpu(d,s,n,r)
#define DMA_SYNC_SG_FOR_DEVICE(d,s,n,r)		cs_dma_sync_sg_for_device(d,s,n,r)


#endif /* __CS75XX_H__ */

