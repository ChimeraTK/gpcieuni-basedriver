/**
 *  @file   pciedev_buffer.h
 *  @brief  Provides definitions for driver allocated list of DMA buffers                        
 */

#ifndef PCIEDEV_BUFFER_H_
#define PCIEDEV_BUFFER_H_

struct pciedev_dev;

/**
 * @brief Stores a linked list of DMA buffers.
 * 
 * This structure holds a linked list of allocated memory buffers that can be used for DMA. 
 * It also provides locking and waitqueue that ensure the list is thread safe. 
 */ 
struct pciedev_buffer_list 
{
    struct pciedev_dev  *parentDev;  /**< Link to parent pci device */
    struct list_head    head;        /**< Linked list head entry  */
    struct list_head    *next;       /**< Next available buffer  */
    spinlock_t          lock;        /**< Spinlock that protects linked list and buffer changes  */
    wait_queue_head_t   waitQueue;   /**< Buffer status changes are signaled here to wake up any waiters  */
    int                 shutDownFlag;/**< Set to 1 on device/driver shutdown to prevent further buffer allocations */
};
typedef struct pciedev_buffer_list pciedev_buffer_list;


/**
 * @brief A DMA buffer structure. 
 * 
 * This structure holds a contiguous memory buffer that can be used for DMA transfer. It also  
 * packs all the data nesessary to describe DMA operation and a buffer state that is used for
 * safe buffer manipulation.  
 */
struct pciedev_buffer {
    struct list_head    list;           /**< List entry struct - holds position in the parent list of DMA buffers */

    unsigned long       size;           /**< Memory buffer size in bytes */
    unsigned int        order;          /**< Memory buffer size in units of kernel page order. */

    unsigned long       kaddr;          /**< Kernel virtual address of memory buffer. */
    dma_addr_t          dma_handle;     /**< Dma handle for memory buffer. */

    unsigned long       dma_offset;     /**< DMA operation offset in device memory */   
    unsigned long       dma_size;       /**< DMA operation data size */   
    
    unsigned long       state;          /**< Buffer state - a combination of ::pciedev_buffer_state_flags */   
};
typedef struct pciedev_buffer pciedev_buffer;

/**
 * @brief Enum of DMA buffer state flags
 */
enum pciedev_buffer_state_flags {
    BUFFER_STATE_AVAILABLE = 1 << 0,    /**< Buffer is available for next operation */
    BUFFER_STATE_WAITING   = 1 << 1     /**< Buffer is waiting for DMA to complete */
};


/* pciedev_buffer_list and pciedev_buffer manipulation functions */

void pciedev_bufferList_init(pciedev_buffer_list *bufferList, struct pciedev_dev *parentDev);

void pciedev_bufferList_append(pciedev_buffer_list* list, pciedev_buffer* buffer);
void pciedev_bufferList_clear(pciedev_buffer_list* list);
pciedev_buffer* pciedev_bufferList_get_free(pciedev_buffer_list* list);
void pciedev_bufferList_set_free(pciedev_buffer_list* list, pciedev_buffer* block);

pciedev_buffer *pciedev_buffer_create(struct pciedev_dev *dev, unsigned long bufSize);
void pciedev_buffer_destroy(struct pciedev_dev *dev, pciedev_buffer *buffer);



#endif /* PCIEDEV_BUFFER_H_ */
