#include <linux/module.h>
#include <linux/fs.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/dma-mapping.h>

#include "pciedev_buffer.h"


/**
 * @brief Free resources allocated by memory buffer. 
 * 
 * @param pciedev  PCI device that target buffer is mapped to
 * @param memblock Target buffer
 * @return void
 */
void pciedev_buffer_destroy(struct pciedev_dev *dev, pciedev_buffer *buffer) 
{
    unsigned long iter = 0;
    
    PDEBUG("pciedev_buffer_destroy(dev.name=%s, buffer=%p)", dev->name, buffer);

    if (!buffer)
    {
        printk(KERN_ERR "PCIEDEV(%s): Got request to release unallocated buffer!\n", dev->name);
        return;
    }
    
    if (buffer->kaddr) 
    {
        // unmap from pci_dev
        if (buffer->dma_handle != DMA_ERROR_CODE)
        {
            dma_unmap_single(&dev->pciedev_pci_dev->dev, buffer->dma_handle, buffer->size, DMA_FROM_DEVICE);
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

/**
 *  @brief Allocate memory buffer for DMA transfers.
 * 
 * Allocates and initializes pciedev_buffer structure with corresponding contiguous block of memory that can be 
 * used as target in DMA data tranfer from device. Buffer is mapped for DMA from given PCI device.
 * Buffer size should be power of 2 and at least 4kB. Upper limit is system dependant, but anything bigger than 4MB is 
 * likely going to fail with -ENOMEM.
 * 
 * @param dev       Target PCI device.
 * @param blkSize   Buffer size.
 *
 * @return          On success the allocated memory buffer is returned.
 * @retval -ENOMEM  Allocation failed
 * 
 */
pciedev_buffer *pciedev_buffer_create(struct pciedev_dev *dev, unsigned long bufSize)
{
    pciedev_buffer *buffer;
    unsigned long iter = 0;
    
    PDEBUG("pciedev_buffer_create(dev.name=%s, bufSize=0x%lx)", dev->name, bufSize);
 
    // allocate the buffer structure
    buffer = kzalloc(sizeof(pciedev_buffer), GFP_KERNEL);
    if (!buffer)
    {
        return ERR_PTR(-ENOMEM);
    }
    
    // init the buffer structure 
    buffer->order    = get_order(bufSize);
    buffer->size     = 1 << (buffer->order + PAGE_SHIFT);
    buffer->dma_free = 1;
    buffer->dma_done = 0;
    
    // allocate memory block
    buffer->kaddr = __get_free_pages(GFP_KERNEL|__GFP_DMA, buffer->order);
    if (!buffer->kaddr) 
    {
        printk(KERN_ERR "pciedev_buffer_create(): can't get free pages of order %u\n", buffer->order);
        
        pciedev_buffer_destroy(dev, buffer);
        return ERR_PTR(-ENOMEM);
    }
            
    // reserve allocated memory pages to prevent them from being swapped out
    for (iter = 0; iter < buffer->size; iter += PAGE_SIZE) 
    {
        SetPageReserved(virt_to_page(buffer->kaddr + iter));
    }
    
    // map to pci_dev
    buffer->dma_handle = dma_map_single(&dev->pciedev_pci_dev->dev, (void *)buffer->kaddr, buffer->size, DMA_FROM_DEVICE);
    if (dma_mapping_error(&dev->pciedev_pci_dev->dev, buffer->dma_handle)) 
    {
        printk(KERN_ERR "pciedev_buffer_create(): dma mapping error\n");
        pciedev_buffer_destroy(dev, buffer);
        return ERR_PTR(-ENOMEM);
    }    
    
    printk(KERN_INFO "PCIEDEV(%s): Allocated DMA buffer of size 0x%lx\n", dev->name, buffer->size);
    return buffer;
}


/** 
 * @brief Add memory buffer to list of memory buffers for given device.
 * 
 * This function is reentrant.
 * 
 * @param mdev   Target device
 * @return void
 */
void pciedev_buffer_add(module_dev* mdev, pciedev_buffer *buffer)
{
    PDEBUG("pciedev_bufferlist_add(mdev = 0x%lx)", mdev,  );
    
    spin_lock(&mdev->dma_bufferList_lock);
    list_add_tail(&buffer->list, &mdev->dma_bufferList);
    spin_unlock(&mdev->dma_bufferList_lock);        
    
    printk(KERN_INFO"PCIEDEV: DMA buffer added to buffer list (size=0x%lx)!\n", buffer->size); 
}

/** 
 * @brief Clears all buffers allocated for given device.
 * 
 * This function is reentrant.
 * 
 * @param mdev   Target device
 * @return void
 */
void pciedev_buffer_clearAll(module_dev* mdev)
{
    pciedev_buffer *buffer = 0;
    struct list_head *pos;
    struct list_head *tpos;
    
    PDEBUG("pciedev_buffer_clearAll(mdev = 0x%lx)");
    
    spin_lock(&mdev->dma_bufferList_lock);
    list_for_each_safe(pos, tpos, &mdev->dma_bufferList) 
    {
        buffer = list_entry(pos, struct pciedev_buffer, list);
        list_del(pos);
        pciedev_buffer_destroy(mdev->parent_dev, buffer);
    }
    spin_unlock(&mdev->dma_bufferList_lock);        
}

/**
 * @brief Gives free (unused) buffer from the list of allocated buffers.
 * 
 * This function is reentrant. The returned buffer is marked reserved so it won't appear free to any 
 * other thread. 
 * Note that this function may block! If there is no free buffer available it will wait for up to 1
 * second. If no buffer becomes available in this time, the call will fail.
 * 
 * @param mdev     Target device
 * @return         On success a free buffer is returned.
 * @retval -EINTR  Interrupted while waiting for available buffer 
 * @retval -BUSY   Timed out while waiting for available buffer 
 * 
 */
pciedev_buffer* pciedev_buffer_get_free(module_dev* mdev)
{
    pciedev_buffer *buffer = 0;
    ulong timeout = HZ/1; // one second timeout
    int code = 0;
    
    PDEBUG("pciedev_buffer_get_free(mdev=0x%lx )", mdev);
    
    spin_lock(&mdev->dma_bufferList_lock);  
    buffer = list_entry(mdev->bufferListNext, struct pciedev_buffer, list);
    while (!buffer->dma_free)
    {
        spin_unlock(&mdev->dma_bufferList_lock);        

        PDEBUG("pciedev_buffer_get_free() WAITING... ");
        code = wait_event_interruptible_timeout(mdev->buffer_waitQueue, buffer->dma_free , timeout);
        if (code == 0)
        {
            printk(KERN_ALERT "PCIEDEV: Failed to get free memory buffer - TIMEOUT!\n");
            return ERR_PTR(-EBUSY);
        }
        else if (code < 0 )
        {
            printk(KERN_ALERT "PCIEDEV: Failed to get free memory buffer - errno=%d\n", code);
            return ERR_PTR(-EINTR);
        }

        spin_lock(&mdev->dma_bufferList_lock);
        buffer = list_entry(mdev->bufferListNext, struct pciedev_buffer, list);
    }
    
    buffer->dma_free = 0;   // Mark buffer reserved
    buffer->dma_done = 0;   // Mark buffer empty
    
    // wrap circular buffers list
    if (list_is_last(mdev->bufferListNext, &mdev->dma_bufferList))
    {
        mdev->bufferListNext = mdev->dma_bufferList.next;
    }
    else
    {
        mdev->bufferListNext = mdev->bufferListNext->next;
    }
    spin_unlock(&mdev->dma_bufferList_lock);        
    
    return buffer;
}

/**
 * @brief Mark buffer free and wake up any waiters
 * 
 * This function is reentrant. 
 * 
 * @param mdev     Target device
 * @param buffer   Target buffer
 * @return void
 */
void pciedev_buffer_set_free(module_dev* mdev, pciedev_buffer* block)
{
    PDEBUG("pciedev_buffer_set_free(offset=0x%lx, size=0x%lx)\n", block->dma_offset, block->dma_size);
    
    spin_lock(&mdev->dma_bufferList_lock);
    block->dma_done = 0;
    block->dma_free = 1;
    spin_unlock(&mdev->dma_bufferList_lock);        
    
    // Wake up any process waiting for free buffers
    wake_up_interruptible(&(mdev->buffer_waitQueue)); 
}

EXPORT_SYMBOL(pciedev_buffer_create);
EXPORT_SYMBOL(pciedev_buffer_add);
EXPORT_SYMBOL(pciedev_buffer_destroy);
EXPORT_SYMBOL(pciedev_buffer_clearAll);
EXPORT_SYMBOL(pciedev_buffer_get_free);
EXPORT_SYMBOL(pciedev_buffer_set_free);
