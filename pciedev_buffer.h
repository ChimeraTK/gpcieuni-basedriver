#ifndef PCIEDEV_BUFFER_H_
#define PCIEDEV_BUFFER_H_

#include "pciedev_ufn.h"

/**
 * Memory block struct used for dma transfer memory.
 */
typedef struct pciedev_buffer {
    struct list_head    list;           /**< List entry struct. */
    unsigned long       offset;         /**< Block offset in driver memory. */ 
    unsigned long       size;           /**< Memory block size in bytes */
    unsigned int        order;          /**< Memory block size in units of kernel page order. */
    unsigned long       kaddr;          /**< Kernel virtual address of memory block. */
    dma_addr_t          dma_handle;     /**< Dma handle for memory block. */
    
    unsigned int        dma_free;      // TODO: use flags, change to reserved
    unsigned int        dma_done;      // TODO: use flags, rename to empty
    unsigned long       dma_offset;        
    unsigned long       dma_size;
};
typedef struct pciedev_buffer pciedev_buffer;

pciedev_buffer *pciedev_buffer_create(struct pciedev_dev *dev, unsigned long bufSize);
void pciedev_buffer_destroy(struct pciedev_dev *dev, pciedev_buffer *buffer);

void pciedev_buffer_add(module_dev* mdev, pciedev_buffer *buffer);
void pciedev_buffer_clearAll(module_dev* mdev);

pciedev_buffer* pciedev_buffer_get_free(module_dev* mdev);
void pciedev_buffer_set_free(module_dev* mdev, pciedev_buffer* block);

#endif /* PCIEDEV_BUFFER_H_ */
