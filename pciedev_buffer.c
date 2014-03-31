#include <linux/module.h>
#include <linux/fs.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/dma-mapping.h>

#include "pciedev_buffer.h"

int pciedev_block_add(module_dev* mdev, ulong size)
{
    PDEBUG("pciedev_block_add(module_dev = &X)", mdev);
    
    // TODO: handle errors
    pciedev_block*  block;
    block = kzalloc(sizeof(pciedev_block), GFP_KERNEL);   
    
    int err = pciedev_dma_alloc(mdev->parent_dev, size, block);
    if (err) {
        kfree(block);
        return err;
    }
    block->dma_free = 1;
    block->dma_done = 0;
    
    spin_lock(&mdev->dma_bufferList_lock);
    block->offset = mdev->dma_bufferListCount * block->size;
    list_add_tail(&block->list, &mdev->dma_bufferList);
    mdev->dma_bufferListCount++;
    spin_unlock(&mdev->dma_bufferList_lock);        
    
    printk(KERN_INFO"PCIEDEV: memory block added to ring buffer (drv_offset=0x%lx, size=0x%lx)!\n", block->offset, block->size); 
}

/*
 * Allocate memory for dma transfers.
 *
 * Try to allocate a physically contiguous block of memory 
 * and obtain a dma handle for that memory. Then fill the memory block struct
 * with necessary info.
 */
int pciedev_dma_alloc(pciedev_dev *pciedev, ulong blkSize, pciedev_block *memblock) 
{
    unsigned int order;
    int          iter;
    
    PDEBUG("pciedev_dma_alloc( pciedev=0x%lx, memblock=0x%lx )", pciedev, memblock);
    
    order = get_order(blkSize);
    
    //memblock->kaddr = __get_free_pages(GFP_KERNEL|GFP_DMA32|__GFP_COMP, order);
    memblock->kaddr = __get_free_pages(GFP_KERNEL|__GFP_DMA, order);
    if (!memblock->kaddr) 
    {
        printk(KERN_ERR "PCIEDEV_ALLOC: can't get free pages of order %u\n", order);
        return -ENOMEM;
    }
        
    memblock->order = order;
    memblock->size = 1 << (order + PAGE_SHIFT);
        
    // make sure buffer pages are never swapped out
    for (iter = 0; iter < memblock->size; iter += PAGE_SIZE) 
    {
        SetPageReserved(virt_to_page(memblock->kaddr + iter));
    }
    printk(KERN_INFO "PCIEDEV_dma_alloc(): allocated buffer:0x%lx with order:%d and size: 0x%lx\n", memblock->kaddr, order, memblock->size);
    
    // map to pci_dev
    memblock->dma_handle = dma_map_single(&pciedev->pciedev_pci_dev, (void *)memblock->kaddr, memblock->size, DMA_FROM_DEVICE);
    if (dma_mapping_error(&pciedev->pciedev_pci_dev, memblock->dma_handle)) 
    {
        printk(KERN_ERR "PCIEDEV_dma_alloc(): dma mapping error\n");
        for(iter = 0; iter < memblock->size; iter += PAGE_SIZE) 
        {
            ClearPageReserved(virt_to_page(memblock->kaddr + iter));
        }
        free_pages(memblock->kaddr, memblock->order);
        printk(KERN_INFO "PCIEDEV_dma_alloc(): freed buffer:0x%lx with order:%d and size: 0x%lx\n", memblock->kaddr, order, memblock->size);
        
        return -ENOMEM;
        
    }    
    
    printk(KERN_INFO "PCIEDEV_dma_alloc(): retrieved dma handle: 0x%llx for buffer: 0x%lx\n", memblock->dma_handle, memblock->kaddr);
    return 0;
}

/*
 * Release memory buffer for dma transfers.
 */
void pciedev_dma_free(pciedev_dev *pciedev, pciedev_block *memblock) 
{
    int    iter;
    
    PDEBUG("pciedev_release_drvdata( pciedev=%X, memblock=%X )", pciedev, memblock);
    
    if (!memblock->kaddr) {
        printk(KERN_WARNING "PCIEDEV_dma_free: attempted to free NULL buffer!\n");
        return;
    }
    
    printk(KERN_INFO "PCIEDEV_ALLOC: freeing allocated dma buffer:0x%lx with order:%d and size: 0x%lx\n", memblock->kaddr, memblock->order, memblock->size);
    for (iter = 0; iter < memblock->size; iter += PAGE_SIZE) 
    {
        ClearPageReserved(virt_to_page(memblock->kaddr + iter));
    }
    dma_unmap_single(&pciedev->pciedev_pci_dev, memblock->dma_handle, memblock->size, DMA_FROM_DEVICE);
    free_pages(memblock->kaddr, memblock->order);
}

EXPORT_SYMBOL(pciedev_dma_alloc);
