/**
 *  @file   pcieuni_buffer.c
 *  @brief  Provides implementation of functions related to driver allocated list of DMA buffers                         
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/dma-mapping.h>

#include "pcieuni_buffer.h"
#include "pcieuni_ufn.h"

/**
 * @brief Initializes empty list of DMA buffers
 * 
 * @param bufferList Target list of DMA buffers
 * @param parentDev  Parent PCI device that buffers map to
 * 
 * @return void
 */
void pcieuni_bufferList_init(pcieuni_buffer_list *bufferList, pcieuni_dev *parentDev)
{
    bufferList->parentDev = parentDev;
    spin_lock_init(&bufferList->lock);
    init_waitqueue_head(&bufferList->waitQueue);
    INIT_LIST_HEAD(&bufferList->head);
    bufferList->next = 0;
    bufferList->shutDownFlag = 0;
}
EXPORT_SYMBOL(pcieuni_bufferList_init); 

/** 
 * @brief Creates DMA buffer and appends it to the target list of DMA buffers.
 * @note This function is thread-safe
 * 
 * @param list      Target list of DMA buffers.
 * @param blkSize   DMA buffer to append
 * 
 * @return void
 */
void pcieuni_bufferList_append(pcieuni_buffer_list* list, pcieuni_buffer* buffer)
{
    spin_lock(&list->lock);

    list_add_tail(&buffer->list, &list->head);
    if (list_is_singular(&list->head))
    {
        list->next = list->head.next;
    }

    spin_unlock(&list->lock);    
}
EXPORT_SYMBOL(pcieuni_bufferList_append);   

/** 
 * @brief Removes and releases all the DMA buffers from the list.
 * 
 * This function attempts to release all the resources held by DMA bufffers in a safe manner. It uses the pcieuni_buffer_list::shutDownFlag
 * flag to disable other threads from using the buffers. If any bufffer appears to be in use this function will wait for up to 1 second for
 * operations on buffer to complete and then move on. If buffer is stuck in a busy mode it's memory won't be released (so memory will leak
 * in this case). 
 * @note This function is thread-safe.
 * @note This function may block.
 * 
 * @param list   Target list of DMA buffers
 * 
 * @return void
 */
void pcieuni_bufferList_clear(pcieuni_buffer_list* list)
{
    ulong timeout = HZ/1; // one second timeout
    pcieuni_buffer *buffer = 0;
    struct list_head *pos;
    struct list_head *tpos;
    int code;
    
    spin_lock(&list->lock);
    list->shutDownFlag = 1;
    
    list_for_each_safe(pos, tpos, &list->head) 
    {
        buffer = list_entry(pos, struct pcieuni_buffer, list);
        code = 1;
        
        while (!test_bit(BUFFER_STATE_AVAILABLE, &buffer->state))
        {
            spin_unlock(&list->lock);
            
            PDEBUG(list->parentDev->name, "pcieuni_bufferList_clear(): Waiting for pending operations on buffer to complete...");
            code = wait_event_timeout(list->waitQueue, test_bit(BUFFER_STATE_AVAILABLE, &buffer->state) , timeout);
            if (code == 0)
            {
                printk(KERN_ALERT "PCIEUNI(%s): Timeout waiting for pending operations to complete before buffer is deleted. Memory won't be released!", 
                       list->parentDev->name);
            }
            else if (code < 0 )
            {
                printk(KERN_ALERT "PCIEUNI(%s): Interrupted while waiting for pending operations to complete before buffer is deleted. Memory won't be released!", 
                       list->parentDev->name);
            }
            
            spin_lock(&list->lock);
        }
        
        list_del(pos);
        
        if (code > 0)
        {
            pcieuni_buffer_destroy(list->parentDev, buffer);
        }
        // else deleting buffer could be dangerous. Better have memory leak than kernel panic.
    }
    spin_unlock(&list->lock);
}
EXPORT_SYMBOL(pcieuni_bufferList_clear);   

/**
 *  @brief Allocates memory buffer for DMA transfers.
 * 
 * Allocates and initializes pcieuni_buffer structure with corresponding contiguous block of memory that can be 
 * used for DMA data tranfer from device. 
 * @note Buffer size should be power of 2 and at least 4kB. Upper limit is system dependant, but anything bigger 
 * than 4MB is likely going to fail with -ENOMEM.
 * 
 * @param dev       Target PCI device.
 * @param blkSize   Buffer size.
 *
 * @return          On success the allocated DMA buffer is returned.
 * @retval -ENOMEM  Allocation failed
 * 
 */
pcieuni_buffer *pcieuni_buffer_create(struct pcieuni_dev *dev, unsigned long bufSize)
{
    pcieuni_buffer *buffer;
    unsigned long iter = 0;
    
    PDEBUG(dev->name, "pcieuni_buffer_create(bufSize=0x%lx)", bufSize);
    
    // allocate the buffer structure
    buffer = kzalloc(sizeof(pcieuni_buffer), GFP_KERNEL);
    if (!buffer)
    {
        return ERR_PTR(-ENOMEM);
    }
    
    // init the buffer structure 
    buffer->order    = get_order(bufSize);
    buffer->size     = 1 << (buffer->order + PAGE_SHIFT);
    set_bit(BUFFER_STATE_AVAILABLE, &buffer->state);
    clear_bit(BUFFER_STATE_WAITING, &buffer->state);
    
#ifdef PCIEUNI_TEST_BUFFER_ALLOCATION_FAILURE
    TEST_RANDOM_EXIT(2, "PCIEUNI: Simulating failed buffer allocation!", ERR_PTR(-ENOMEM))
#endif    
    
    // allocate memory block
    buffer->kaddr = __get_free_pages(GFP_KERNEL|__GFP_DMA, buffer->order);
    if (!buffer->kaddr) 
    {
        printk(KERN_ERR "pcieuni_buffer_create(): can't get free pages of order %u\n", buffer->order);
        
        pcieuni_buffer_destroy(dev, buffer);
        return ERR_PTR(-ENOMEM);
    }
    
    // reserve allocated memory pages to prevent them from being swapped out
    for (iter = 0; iter < buffer->size; iter += PAGE_SIZE) 
    {
        SetPageReserved(virt_to_page(buffer->kaddr + iter));
    }
    
    // map to pci_dev
    buffer->dma_handle = dma_map_single(&dev->pcieuni_pci_dev->dev, (void *)buffer->kaddr, buffer->size, DMA_FROM_DEVICE);
    if (dma_mapping_error(&dev->pcieuni_pci_dev->dev, buffer->dma_handle)) 
    {
        printk(KERN_ERR "pcieuni_buffer_create(): dma mapping error\n");
        pcieuni_buffer_destroy(dev, buffer);
        return ERR_PTR(-ENOMEM);
    }    
    
    printk(KERN_INFO "PCIEUNI(%s): Allocated DMA buffer of size 0x%lx\n", dev->name, buffer->size);
    return buffer;
}
EXPORT_SYMBOL(pcieuni_buffer_create);   

/**
 * @brief Frees all resources allocated by DMA buffer. 
 * 
 * @param pcieuni  PCI device that buffer is mapped to
 * @param memblock Target buffer
 * 
 * @return void
 */
void pcieuni_buffer_destroy(struct pcieuni_dev *dev, pcieuni_buffer *buffer) 
{
    unsigned long iter = 0;
    
    PDEBUG(dev->name, "pcieuni_buffer_destroy(buffer=%p)", buffer);
    
    if (!buffer)
    {
        printk(KERN_ERR "PCIEUNI(%s): Got request to release unallocated buffer!\n", dev->name);
        return;
    }
    
    if (buffer->kaddr) 
    {
        // unmap from pci_dev
        if (buffer->dma_handle != DMA_ERROR_CODE)
        {
            dma_unmap_single(&dev->pcieuni_pci_dev->dev, buffer->dma_handle, buffer->size, DMA_FROM_DEVICE);
        }
        
        // release allocated memory pages
        for (iter = 0; iter < buffer->size; iter += PAGE_SIZE) 
        {
            ClearPageReserved(virt_to_page(buffer->kaddr + iter));
        }
        free_pages(buffer->kaddr, buffer->order);
    }
    
    kfree(buffer);
}
EXPORT_SYMBOL(pcieuni_buffer_destroy);   

/**
 * @brief Gives free (available) DMA buffer from the list of DMA buffers.
 * 
 * If there is no free buffer available this fuction will wait for up to 1 second before it times out.
 * If buffer is found it is reserved by clearing the BUFFER_STATE_AVAILABLE flag.
 * @note This function is thread-safe.
 * @note This function may block.
 * 
 * @param list     Target list of DMA buffers
 * 
 * @return          On success a free buffer is returned.
 * @retval -EINTR   Interrupted while waiting for available buffer 
 * @retval -BUSY    Timed out while waiting for available buffer 
 * @retval -ENOMEM  No buffers are allocated   
 */
pcieuni_buffer* pcieuni_bufferList_get_free(pcieuni_buffer_list* list)
{
    pcieuni_buffer *buffer = 0;
    ulong timeout = HZ/1; // one second timeout
    int code = 0;
    
    spin_lock(&list->lock);
    if (list->shutDownFlag) 
    {
        spin_unlock(&list->lock);
        return ERR_PTR(-ENOMEM);
    }
        
    buffer = list_entry(list->next, struct pcieuni_buffer, list);
    while (!test_bit(BUFFER_STATE_AVAILABLE, &buffer->state))
    {
        spin_unlock(&list->lock);

        PDEBUG(list->parentDev->name, "pcieuni_bufferList_get_free(): Waiting... ");
        code = wait_event_interruptible_timeout(list->waitQueue, test_bit(BUFFER_STATE_AVAILABLE, &buffer->state) , timeout);
        if (code == 0)
        {
            return ERR_PTR(-EBUSY);
        }
        else if (code < 0 )
        {
            return ERR_PTR(-EINTR);
        }

        spin_lock(&list->lock);
        if (list->shutDownFlag) return ERR_PTR(-EINTR);
        
        buffer = list_entry(list->next, struct pcieuni_buffer, list);
    }
    
    clear_bit(BUFFER_STATE_AVAILABLE, &buffer->state); // Buffer is now reserved
    set_bit(BUFFER_STATE_WAITING, &buffer->state);     // Buffer is now waiting for DMA data
    
    // wrap circular buffers list
    if (list_is_last(list->next, &list->head))
    {
        list->next = list->head.next;
    }
    else
    {
        list->next = list->next->next;
    }
    spin_unlock(&list->lock);
    
    return buffer;
}
EXPORT_SYMBOL(pcieuni_bufferList_get_free); 

/**
 * @brief Marks DMA buffer avaialble by settig the BUFFER_STATE_AVAILABLE flag and wakes up any waiters.
 * @note This function is thread-safe.
 * 
 * @param mdev     Target device
 * @param buffer   Target buffer
 * 
 * @return void
 */
void pcieuni_bufferList_set_free(pcieuni_buffer_list* list, pcieuni_buffer* buffer)
{
    spin_lock(&list->lock);
    
    clear_bit(BUFFER_STATE_WAITING, &buffer->state);
    set_bit(BUFFER_STATE_AVAILABLE, &buffer->state);

    spin_unlock(&list->lock);        
    
    // Wake up any process waiting for free buffers
    wake_up_interruptible(&(list->waitQueue)); 
}
EXPORT_SYMBOL(pcieuni_bufferList_set_free); 
