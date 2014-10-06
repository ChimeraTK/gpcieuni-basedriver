/**
 *  @file   pcieuni_buffer.h
 *  @brief  Provides definitions for driver allocated list of DMA buffers                        
 */

#ifndef PCIEUNI_BUFFER_H_
#define PCIEUNI_BUFFER_H_

struct pcieuni_dev;

/**
 * @brief Stores a linked list of DMA buffers.
 * 
 * This structure holds a linked list of allocated memory buffers that can be used for DMA. 
 * It also provides locking and waitqueue that ensure the list is thread safe. 
 */ 
struct pcieuni_buffer_list 
{
    struct pcieuni_dev  *parentDev;  /**< Link to parent pci device */
    struct list_head    head;        /**< Linked list head entry  */
    struct list_head    *next;       /**< Next available buffer  */
    spinlock_t          lock;        /**< Spinlock that protects linked list and buffer changes  */
    wait_queue_head_t   waitQueue;   /**< Buffer status changes are signaled here to wake up any waiters  */
    int                 shutDownFlag;/**< Set to 1 on device/driver shutdown to prevent further buffer allocations */
};
typedef struct pcieuni_buffer_list pcieuni_buffer_list;


/**
 * @brief A DMA buffer structure. 
 * 
 * This structure holds a contiguous memory buffer that can be used for DMA transfer. It also  
 * packs all the data nesessary to describe DMA operation and a buffer state that is used for
 * safe buffer manipulation.  
 */
struct pcieuni_buffer {
    struct list_head    list;           /**< List entry struct - holds position in the parent list of DMA buffers */

    unsigned long       size;           /**< Memory buffer size in bytes */
    unsigned int        order;          /**< Memory buffer size in units of kernel page order. */

    unsigned long       kaddr;          /**< Kernel virtual address of memory buffer. */
    dma_addr_t          dma_handle;     /**< Dma handle for memory buffer. */

    unsigned long       dma_offset;     /**< DMA operation offset in device memory */   
    unsigned long       dma_size;       /**< DMA operation data size */   
    
    unsigned long       state;          /**< Buffer state - a combination of ::pcieuni_buffer_state_flags */   
};
typedef struct pcieuni_buffer pcieuni_buffer;

/**
 * @brief Enum of DMA buffer state flags
 */
enum pcieuni_buffer_state_flags {
    BUFFER_STATE_AVAILABLE = 1 << 0,    /**< Buffer is available for next operation */
    BUFFER_STATE_WAITING   = 1 << 1     /**< Buffer is waiting for DMA to complete */
};


/* pcieuni_buffer_list and pcieuni_buffer manipulation functions */

void pcieuni_bufferList_init(pcieuni_buffer_list *bufferList, struct pcieuni_dev *parentDev);

void pcieuni_bufferList_append(pcieuni_buffer_list* list, pcieuni_buffer* buffer);
void pcieuni_bufferList_clear(pcieuni_buffer_list* list);
pcieuni_buffer* pcieuni_bufferList_get_free(pcieuni_buffer_list* list);
void pcieuni_bufferList_set_free(pcieuni_buffer_list* list, pcieuni_buffer* block);

pcieuni_buffer *pcieuni_buffer_create(struct pcieuni_dev *dev, unsigned long bufSize);
void pcieuni_buffer_destroy(struct pcieuni_dev *dev, pcieuni_buffer *buffer);



#endif /* PCIEUNI_BUFFER_H_ */
