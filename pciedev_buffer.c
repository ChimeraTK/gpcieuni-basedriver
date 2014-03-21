#include <linux/module.h>
#include <linux/fs.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/dma-mapping.h>

#include "pciedev_buffer.h"

int pciedev_block_add(module_dev* mdev)
{
    // TODO: handle errors
    pciedev_block*  block;
    block = kzalloc(sizeof(pciedev_block), GFP_KERNEL);   
    
    int err = pciedev_dma_alloc(mdev, block);
    if (err) {
        return err;
    }
    block->dma_free = 1;
    block->dma_done = 0;
    
    spin_lock(&mdev->dma_bufferList_lock);
    list_add_tail(&block->list, &mdev->dma_bufferList);
    spin_unlock(&mdev->dma_bufferList_lock);        
}

/*
 * Allocate memory for dma transfers.
 *
 * Try to allocate a physically contiguous block of memory 
 * and obtain a dma handle for that memory. Then fill the memory block struct
 * with necessary info.
 */
int pciedev_dma_alloc(pciedev_dev *pciedev, pciedev_block *memblock) 
{
    unsigned int order;
    int          iter;
    
    order = get_order(4*1024*1024);
    
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
    printk(KERN_INFO "PCIEDEV_ALLOC: allocated buffer:0x%lx with order:%d and size: 0x%lx\n", memblock->kaddr, order, memblock->size);
    
    // map to pci_dev
    memblock->dma_handle = dma_map_single(&pciedev->pciedev_pci_dev, (void *)memblock->kaddr, memblock->size, DMA_FROM_DEVICE);
    if (dma_mapping_error(&pciedev->pciedev_pci_dev, memblock->dma_handle)) 
    {
        printk(KERN_ERR "PCIEDEV_ALLOC: dma mapping error\n");
        for(iter = 0; iter < memblock->size; iter += PAGE_SIZE) 
        {
            ClearPageReserved(virt_to_page(memblock->kaddr + iter));
        }
        free_pages(memblock->kaddr, memblock->order);
        printk(KERN_INFO "PCIEDEV_ALLOC: freed buffer:0x%lx with order:%d and size: 0x%lx\n", memblock->kaddr, order, memblock->size);
        
        return -ENOMEM;
        
    }    
    
    printk(KERN_INFO "PCIEDEV_ALLOC: retrieved dma handle: 0x%llx for buffer: 0x%lx\n", memblock->dma_handle, memblock->kaddr);
    return 0;
}

/*
 * Release memory buffer for dma transfers.
 */
void pciedev_dma_free(pciedev_dev *pciedev, pciedev_block *memblock) 
{
    int    iter;
    
    if (!memblock->kaddr) {
        printk(KERN_WARNING "PCIEDEV_ALLOC: attempted to free NULL dma buffer!\n");
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
