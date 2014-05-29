

#include <linux/module.h>
#include <linux/fs.h>	
#include <linux/sched.h>
#include <linux/delay.h>

#include "pciedev_buffer.h"
#include "pciedev_ufn.h"


int    pciedev_open_exp( struct inode *inode, struct file *filp )
{
    int    minor;
    struct pciedev_dev *dev;
    
    minor = iminor(inode);
    dev = container_of(inode->i_cdev, struct pciedev_dev, cdev);
    dev->dev_minor     = minor;
    filp->private_data  = dev; 
    
    //printk(KERN_ALERT "Open Procces is \"%s\" (pid %i) DEV is %d \n", current->comm, current->pid, minor);
    return 0;
}
EXPORT_SYMBOL(pciedev_open_exp);

int    pciedev_release_exp(struct inode *inode, struct file *filp)
{
    int minor            = 0;
    int d_num           = 0;
    u16 cur_proc     = 0;
    struct pciedev_dev *dev   = filp->private_data;
    minor     = dev->dev_minor;
    d_num   = dev->dev_num;
    cur_proc = current->group_leader->pid;
    //printk(KERN_ALERT "Close Procces is \"%s\" (pid %i)\n", current->comm, current->pid);
    //printk(KERN_ALERT "Close MINOR %d DEV_NUM %d \n", minor, d_num);
    return 0;
}
EXPORT_SYMBOL(pciedev_release_exp);


int pciedev_set_drvdata(struct pciedev_dev *dev, void *data)
{
    if(!dev)
        return 1;
    dev->dev_str = data;
    return 0;
}
EXPORT_SYMBOL(pciedev_set_drvdata);

void *pciedev_get_drvdata(struct pciedev_dev *dev){
    if(dev && dev->dev_str)
        return dev->dev_str;
    return NULL;
}
EXPORT_SYMBOL(pciedev_get_drvdata);

/**
 * @brief Allocates and initializes driver specific data for given pci device
 * 
 * @param brd_num       device index in the list of probed devices
 * @param pcidev        target pci device structure
 * @param kbuf_blk_num  number of preallocated DMA buffers
 * @param kbuf_blk_size size of preallocated DMA buffers
 * 
 * @return  Allocated module_dev strucure
 * @retval  -ENOMEM     Failed - could not allocate memory
 */
module_dev* pciedev_create_drvdata(int brd_num, pciedev_dev* pcidev, ushort kbuf_blk_num, ulong kbuf_blk_size)
{
    module_dev* mdev;
    pciedev_buffer* dmaBuffer;
    ushort i;
    
    PDEBUG("pciedev_create_drvdata( brd_num = %i)", brd_num);
    
    mdev = kzalloc(sizeof(module_dev), GFP_KERNEL);
    if(!mdev) 
    {
        return ERR_PTR(-ENOMEM);
    }
    mdev->brd_num     = brd_num;
    mdev->parent_dev  = pcidev;

    init_waitqueue_head(&mdev->waitDMA);
    init_waitqueue_head(&mdev->buffer_waitQueue);
    INIT_LIST_HEAD(&mdev->dma_bufferList);
    spin_lock_init(&mdev->dma_bufferList_lock);
    sema_init(&mdev->dma_sem, 1);
    
    // allocate DMA buffers
    for (i = 0; i < kbuf_blk_num; i++)
    {
        dmaBuffer = pciedev_buffer_append(mdev, kbuf_blk_size); // TODO: error handling
        if (IS_ERR(dmaBuffer))
        {
            pciedev_release_drvdata(mdev);
            return ERR_CAST(dmaBuffer);
        }
    }
    mdev->bufferListNext = mdev->dma_bufferList.next;
    
    mdev->waitFlag        = 1;
    mdev->dma_buffer      = 0;
    
    return mdev;
}
EXPORT_SYMBOL(pciedev_create_drvdata);

void pciedev_release_drvdata(module_dev* mdev)
{
    PDEBUG("pciedev_release_drvdata(mdev = %p)", mdev);
    
    if (!IS_ERR_OR_NULL(mdev))
    {
        // TODO: 
        // set shutting down
        // wait until all buffers available (interruptible wait)
        
        // clear the buffers
        pciedev_buffer_clearAll(mdev);        
        
        // clear the drvdata structure
        kfree(mdev);
    }
}
EXPORT_SYMBOL(pciedev_release_drvdata);

int       pciedev_get_brdnum(struct pci_dev *dev)
{
    int                                 m_brdNum;
    pciedev_dev                *pciedevdev;
    pciedevdev        = dev_get_drvdata(&(dev->dev));
    m_brdNum       = pciedevdev->brd_num;
    return m_brdNum;
}
EXPORT_SYMBOL(pciedev_get_brdnum);

pciedev_dev*   pciedev_get_pciedata(struct pci_dev  *dev)
{
    pciedev_dev                *pciedevdev;
    pciedevdev    = dev_get_drvdata(&(dev->dev));
    return pciedevdev;
}
EXPORT_SYMBOL(pciedev_get_pciedata);

module_dev*   pciedev_get_moduledata(struct pciedev_dev *dev)
{
    struct module_dev *mdev = (struct module_dev*)(dev->dev_str);
    return mdev;
}
EXPORT_SYMBOL(pciedev_get_moduledata);

void*   pciedev_get_baddress(int br_num, struct pciedev_dev  *dev)
{
    void *tmp_address;
    
    tmp_address = 0;
    switch(br_num){
        case 0:
            tmp_address = dev->memmory_base0;
            break;
        case 1:
            tmp_address = dev->memmory_base0;
            break;
        case 2:
            tmp_address = dev->memmory_base0;
            break;
        case 3:
            tmp_address = dev->memmory_base0;
            break;
        case 4:
            tmp_address = dev->memmory_base0;
            break;
        case 5:
            tmp_address = dev->memmory_base0;
            break;
        default:
            break;
    }
    return tmp_address;
}
EXPORT_SYMBOL(pciedev_get_baddress);

#if LINUX_VERSION_CODE < 0x20613 // irq_handler_t has changed in 2.6.19
int pciedev_setup_interrupt(irqreturn_t (*pciedev_interrupt)(int , void *, struct pt_regs *),struct pciedev_dev  *pdev, char  *dev_name)
#else
int pciedev_setup_interrupt(irqreturn_t (*pciedev_interrupt)(int , void *), struct pciedev_dev  *pdev, char  *dev_name)
#endif
{
    int result = 0;
    
    /*******SETUP INTERRUPTS******/
    pdev->irq_mode = 1;
    result = request_irq(pdev->pci_dev_irq, pciedev_interrupt,
                        pdev->irq_flag, dev_name, pdev);
    printk(KERN_INFO "PCIEDEV_PROBE:  assigned IRQ %i RESULT %i\n",
               pdev->pci_dev_irq, result);
    if (result) {
         printk(KERN_INFO "PCIEDEV_PROBE: can't get assigned irq %i\n", pdev->pci_dev_irq);
         pdev->irq_mode = 0;
    }
    return result;
}
EXPORT_SYMBOL(pciedev_setup_interrupt);

int      pciedev_get_brdinfo(struct pciedev_dev  *bdev)
{
    void *baddress;
    void *address;
    int    strbrd = 0;
    u32  tmp_data_32;
    
    bdev->startup_brd = 0;
    if(bdev->memmory_base0){ 
        baddress = bdev->memmory_base0;
        address = baddress;
        tmp_data_32       = ioread32(address );
        if(tmp_data_32 == ASCII_BOARD_MAGIC_NUM || tmp_data_32 ==ASCII_BOARD_MAGIC_NUM_L){
            bdev->startup_brd = 1;
            address = baddress + WORD_BOARD_ID;
            tmp_data_32       = ioread32(address);
            bdev->brd_info_list.PCIEDEV_BOARD_ID = tmp_data_32;

            address = baddress + WORD_BOARD_VERSION;
            tmp_data_32       = ioread32(address );
            bdev->brd_info_list.PCIEDEV_BOARD_VERSION = tmp_data_32;

            address = baddress + WORD_BOARD_DATE;
            tmp_data_32       = ioread32(address );
            bdev->brd_info_list.PCIEDEV_BOARD_DATE = tmp_data_32;

            address = baddress + WORD_BOARD_HW_VERSION;
            tmp_data_32       = ioread32(address );
            bdev->brd_info_list.PCIEDEV_HW_VERSION = tmp_data_32;

            bdev->brd_info_list.PCIEDEV_PROJ_NEXT = 0;
            address = baddress + WORD_BOARD_TO_PROJ;
            tmp_data_32       = ioread32(address );
            bdev->brd_info_list.PCIEDEV_PROJ_NEXT = tmp_data_32;
        }
    }
    
    strbrd = bdev->startup_brd;

    return strbrd;
}
EXPORT_SYMBOL(pciedev_get_brdinfo);

int   pciedev_fill_prj_info(struct pciedev_dev  *bdev, void  *baddress)
{
    void *address;
    int    strbrd  = 0;
    u32  tmp_data_32;
     struct pciedev_prj_info  *tmp_prj_info_list; 
    
    address           = baddress;
    tmp_data_32  = ioread32(address );
    if(tmp_data_32 == ASCII_PROJ_MAGIC_NUM ){
        bdev->startup_prj_num++;
        tmp_prj_info_list = kzalloc(sizeof(pciedev_prj_info), GFP_KERNEL);
        
        address = baddress + WORD_PROJ_ID;
        tmp_data_32       = ioread32(address);
       tmp_prj_info_list->PCIEDEV_PROJ_ID = tmp_data_32;

        address = baddress + WORD_PROJ_VERSION;
        tmp_data_32       = ioread32(address );
       tmp_prj_info_list->PCIEDEV_PROJ_VERSION = tmp_data_32;

        address = baddress + WORD_PROJ_DATE;
        tmp_data_32       = ioread32(address );
       tmp_prj_info_list->PCIEDEV_PROJ_DATE = tmp_data_32;

        address = baddress + WORD_PROJ_RESERVED;
        tmp_data_32       = ioread32(address );
       tmp_prj_info_list->PCIEDEV_PROJ_RESERVED = tmp_data_32;

        bdev->brd_info_list.PCIEDEV_PROJ_NEXT = 0;
        address = baddress + WORD_PROJ_NEXT;
        tmp_data_32       = ioread32(address );
       tmp_prj_info_list->PCIEDEV_PROJ_NEXT = tmp_data_32;

        list_add(&(tmp_prj_info_list->prj_list), &(bdev->prj_info_list.prj_list));
        strbrd= tmp_data_32;
    }
    
    return strbrd;
}
EXPORT_SYMBOL(pciedev_fill_prj_info);

int      pciedev_get_prjinfo(struct pciedev_dev  *bdev)
{
    void *baddress;
    void *address;
    int   strbrd             = 0;
    int  tmp_next_prj  = 0;
    int  tmp_next_prj1 = 0;
    
    bdev->startup_prj_num = 0;
    tmp_next_prj =bdev->brd_info_list.PCIEDEV_PROJ_NEXT;
    if(tmp_next_prj){
        baddress = bdev->memmory_base0;
        while(tmp_next_prj){
            address = baddress + tmp_next_prj;
            tmp_next_prj = pciedev_fill_prj_info(bdev, address);
        }
    }else{
        if(bdev->memmory_base1){ 
            tmp_next_prj  = 1;
            tmp_next_prj1 = 0;
            baddress = bdev->memmory_base1;
            while(tmp_next_prj){
                tmp_next_prj  = tmp_next_prj1;
                address = baddress + tmp_next_prj;
                tmp_next_prj = pciedev_fill_prj_info(bdev, address);
            }
        }
        if(bdev->memmory_base2){ 
            tmp_next_prj  = 1;
            tmp_next_prj1 = 0;
            baddress = bdev->memmory_base2;
            while(tmp_next_prj){
                tmp_next_prj  = tmp_next_prj1;
                address = baddress + tmp_next_prj;
                tmp_next_prj = pciedev_fill_prj_info(bdev, address);
            }
        }
        if(bdev->memmory_base3){ 
            tmp_next_prj  = 1;
            tmp_next_prj1 = 0;
            baddress = bdev->memmory_base3;
            while(tmp_next_prj){
                tmp_next_prj  = tmp_next_prj1;
                address = baddress + tmp_next_prj;
                tmp_next_prj = pciedev_fill_prj_info(bdev, address);
            }
        }
        if(bdev->memmory_base4){ 
            tmp_next_prj  = 1;
            tmp_next_prj1 = 0;
            baddress = bdev->memmory_base4;
            while(tmp_next_prj){
                tmp_next_prj  = tmp_next_prj1;
                address = baddress + tmp_next_prj;
                tmp_next_prj = pciedev_fill_prj_info(bdev, address);
            }
        }
        if(bdev->memmory_base5){ 
            tmp_next_prj  = 1;
            tmp_next_prj1 = 0;
            baddress = bdev->memmory_base5;
            while(tmp_next_prj){
                tmp_next_prj  = tmp_next_prj1;
                address = baddress + tmp_next_prj;
                tmp_next_prj = pciedev_fill_prj_info(bdev, address);
            }
        }
    }
    strbrd = bdev->startup_prj_num;
    return strbrd;
}
EXPORT_SYMBOL(pciedev_get_prjinfo);

int pciedev_procinfo(char *buf, char **start, off_t fpos, int lenght, int *eof, void *data)
{
    char *p;
    pciedev_dev     *pciedev_dev_m ;
    struct list_head *pos;
    struct pciedev_prj_info  *tmp_prj_info_list;

    pciedev_dev_m = (pciedev_dev*)data;
    p = buf;
    p += sprintf(p,"UPCIEDEV Driver Version:\t%i.%i\n", pciedev_dev_m->parent_dev->UPCIEDEV_VER_MAJ, 
                                                                               pciedev_dev_m->parent_dev->UPCIEDEV_VER_MIN);
    p += sprintf(p,"Driver Version:\t%i.%i\n", pciedev_dev_m->parent_dev->PCIEDEV_DRV_VER_MAJ, 
                                                                               pciedev_dev_m->parent_dev->PCIEDEV_DRV_VER_MIN);
    p += sprintf(p,"Board NUM:\t%i\n", pciedev_dev_m->brd_num);
    p += sprintf(p,"Slot    NUM:\t%i\n", pciedev_dev_m->slot_num);
    p += sprintf(p,"Board ID:\t%X\n", pciedev_dev_m->brd_info_list.PCIEDEV_BOARD_ID);
    p += sprintf(p,"Board Version;\t%X\n",pciedev_dev_m->brd_info_list.PCIEDEV_BOARD_VERSION);
    p += sprintf(p,"Board Date:\t%X\n",pciedev_dev_m->brd_info_list.PCIEDEV_BOARD_DATE);
    p += sprintf(p,"Board HW Ver:\t%X\n",pciedev_dev_m->brd_info_list.PCIEDEV_HW_VERSION);
    p += sprintf(p,"Board Next Prj:\t%X\n",pciedev_dev_m->brd_info_list.PCIEDEV_PROJ_NEXT);
    p += sprintf(p,"Board Reserved:\t%X\n",pciedev_dev_m->brd_info_list.PCIEDEV_BOARD_RESERVED);
    p += sprintf(p,"Number of Proj:\t%i\n", pciedev_dev_m->startup_prj_num);
    
    list_for_each(pos,  &pciedev_dev_m->prj_info_list.prj_list ){
        tmp_prj_info_list = list_entry(pos, struct pciedev_prj_info, prj_list);
        p += sprintf(p,"Project ID:\t%X\n", tmp_prj_info_list->PCIEDEV_PROJ_ID);
        p += sprintf(p,"Project Version:\t%X\n", tmp_prj_info_list->PCIEDEV_PROJ_VERSION);
        p += sprintf(p,"Project Date:\t%X\n", tmp_prj_info_list->PCIEDEV_PROJ_DATE);
        p += sprintf(p,"Project Reserver:\t%X\n", tmp_prj_info_list->PCIEDEV_PROJ_RESERVED);
        p += sprintf(p,"Project Next:\t%X\n", tmp_prj_info_list->PCIEDEV_PROJ_NEXT);
    }
 /* 
    p += sprintf(p,"Proj ID:\t%i\n", PCIEDEV_PROJ_ID);
    p += sprintf(p,"Proj Version;\t%i\n",PCIEDEV_PROJ_VR);
    p += sprintf(p,"Proj Date:\t%i\n",PCIEDEV_PROJ_DT);
*/

    *eof = 1;
    return p - buf;
}
EXPORT_SYMBOL(pciedev_procinfo);

/**
 * @brief Writes 32bit value to memory mapped device register
 * 
 * If parameter @param ensureFlush is set to true the function will try to make sure that write is flushed to device. 
 * Flushing is a problem because PCI bus writes are posted asynchronously (see 
 * <a href="https://www.kernel.org/doc/htmldocs/deviceiobook/accessing_the_device.html">deviceiobook in  Linux Kernel HTML Documentation</a>). 
 * In principle PCI bus should automatically flush everything before next ioread is serviced. However, experience indicate that event this
 * is a problem with fast CPUs, therefore this function attempts up to 5 retries on ioread with 1 microsecond delays between them. This is       
 * still an experimental solution that needs to be verified.
 * 
 * @param address     Memory address of the target register
 * @param value       Value to write to target register
 * @param ensureFlush Ensure write operation is flushed to device before function returns.
 * 
 * @retval  0     Success
 * @retval  -EIO  Failure
 */
int pciedev_register_write32(u32 value, void* address, bool ensureFlush)
{
    u32 readbackData;
    int flushRetry = 5;
    
    // Write to device register
    iowrite32(value, address);
    
    if (ensureFlush)
    {
        // force CPU write flush
        smp_wmb();
        while (flushRetry > 0)
        {
            // Read from device should force PCI bus to flush all writes
            readbackData = ioread32(address);
            smp_rmb();
            PDEBUG("written=0x%x, readback=0x%x", value, readbackData);
            if (readbackData == value) break; // Assume write was flushed
            
            break; // TODO: readback does not work, so we have no feedback from PCI bus... We need a new plan.
            
            // Experimental: delay to give PCI bus some time and then read again 
            PDEBUG("Asynchronous write to device register too slow, delaying execution by one microsecond... ");
            udelay(1);
            flushRetry--;
        }
    }
    
    return (flushRetry > 0) ? 0 : -EIO; 
}
EXPORT_SYMBOL(pciedev_register_write32);