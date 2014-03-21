#ifndef PCIEDEV_BUFFER_H_
#define PCIEDEV_BUFFER_H_

#include "pciedev_ufn.h"

/**
 * Memory block struct used for dma transfer memory.
 */
typedef struct pciedev_block {
    struct list_head    list;           /**< List entry struct. */
    unsigned int        order;          /**< Memory block size in units of kernel page order. */
    unsigned long       size;           /**< Memory block size in bytes */
    unsigned long       kaddr;          /**< Kernel virtual address of memory block. */
    dma_addr_t          dma_handle;     /**< Dma handle for memory block. */
    
    unsigned int        dma_free;      // TODO: use flags   
    unsigned int        dma_done;      // TODO: use flags
    unsigned long       dma_offset;        
    unsigned long       dma_size;
};
typedef struct pciedev_block pciedev_block;


int pciedev_dma_alloc(pciedev_dev *pciedev, pciedev_block *memblock);
void pciedev_dma_free(pciedev_dev *pciedev, pciedev_block *memblock);

int pciedev_block_add(module_dev* mdev);


#endif /* PCIEDEV_BUFFER_H_ */
