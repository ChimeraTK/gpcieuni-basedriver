

#include <linux/module.h>
#include <linux/fs.h>	
#include <linux/sched.h>
#include <linux/delay.h>

/*
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <asm/types.h>
*/


#include "pcieuni_ufn.h"


int    pcieuni_open_exp( struct inode *inode, struct file *filp )
{
    int    minor;
    struct pcieuni_dev *dev;
    
    minor = iminor(inode);
    dev = container_of(inode->i_cdev, struct pcieuni_dev, cdev);
    dev->dev_minor     = minor;
    filp->private_data  = dev; 
    
    //printk(KERN_ALERT "Open Procces is \"%s\" (pid %i) DEV is %d \n", current->comm, current->pid, minor);
    return 0;
}
EXPORT_SYMBOL(pcieuni_open_exp);

int    pcieuni_release_exp(struct inode *inode, struct file *filp)
{
    u16 cur_proc     = 0;
    //struct pcieuni_dev *dev   = filp->private_data;
    //minor     = dev->dev_minor;
    //d_num   = dev->dev_num;
    cur_proc = current->group_leader->pid;
    //printk(KERN_ALERT "Close Procces is \"%s\" (pid %i)\n", current->comm, current->pid);
    //printk(KERN_ALERT "Close INODE %d FILE %d \n", inode, filp);
    return 0;
}
EXPORT_SYMBOL(pcieuni_release_exp);


int pcieuni_set_drvdata(struct pcieuni_dev *dev, void *data)
{
    if(!dev)
        return 1;
    dev->dev_str = data;
    return 0;
}
EXPORT_SYMBOL(pcieuni_set_drvdata);

void *pcieuni_get_drvdata(struct pcieuni_dev *dev){
    if(dev && dev->dev_str)
        return dev->dev_str;
    return NULL;
}
EXPORT_SYMBOL(pcieuni_get_drvdata);

int       pcieuni_get_brdnum(struct pci_dev *dev)
{
    int                                 m_brdNum;
    pcieuni_dev                *pcieunidev;
    pcieunidev        = dev_get_drvdata(&(dev->dev));
    m_brdNum       = pcieunidev->brd_num;
    return m_brdNum;
}
EXPORT_SYMBOL(pcieuni_get_brdnum);

pcieuni_dev*   pcieuni_get_pciedata(struct pci_dev  *dev)
{
    pcieuni_dev                *pcieunidev;
    pcieunidev    = dev_get_drvdata(&(dev->dev));
    return pcieunidev;
}
EXPORT_SYMBOL(pcieuni_get_pciedata);

void*   pcieuni_get_baddress(int br_num, struct pcieuni_dev  *dev)
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
EXPORT_SYMBOL(pcieuni_get_baddress);

#if LINUX_VERSION_CODE < 0x20613 // irq_handler_t has changed in 2.6.19
int pcieuni_setup_interrupt(irqreturn_t (*pcieuni_interrupt)(int , void *, struct pt_regs *),struct pcieuni_dev  *pdev, char  *dev_name)
#else
int pcieuni_setup_interrupt(irqreturn_t (*pcieuni_interrupt)(int , void *), struct pcieuni_dev  *pdev, char  *dev_name)
#endif
{
    int result = 0;
    
    /*******SETUP INTERRUPTS******/
    pdev->irq_mode = 1;
    result = request_irq(pdev->pci_dev_irq, pcieuni_interrupt,
                        pdev->irq_flag, dev_name, pdev);
    printk(KERN_INFO "PCIEUNI_PROBE:  assigned IRQ %i RESULT %i\n",
               pdev->pci_dev_irq, result);
    if (result) {
         printk(KERN_INFO "PCIEUNI_PROBE: can't get assigned irq %i\n", pdev->pci_dev_irq);
         pdev->irq_mode = 0;
    }
    return result;
}
EXPORT_SYMBOL(pcieuni_setup_interrupt);

int      pcieuni_get_brdinfo(struct pcieuni_dev  *bdev)
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
            bdev->brd_info_list.PCIEUNI_BOARD_ID = tmp_data_32;

            address = baddress + WORD_BOARD_VERSION;
            tmp_data_32       = ioread32(address );
            bdev->brd_info_list.PCIEUNI_BOARD_VERSION = tmp_data_32;

            address = baddress + WORD_BOARD_DATE;
            tmp_data_32       = ioread32(address );
            bdev->brd_info_list.PCIEUNI_BOARD_DATE = tmp_data_32;

            address = baddress + WORD_BOARD_HW_VERSION;
            tmp_data_32       = ioread32(address );
            bdev->brd_info_list.PCIEUNI_HW_VERSION = tmp_data_32;

            bdev->brd_info_list.PCIEUNI_PROJ_NEXT = 0;
            address = baddress + WORD_BOARD_TO_PROJ;
            tmp_data_32       = ioread32(address );
            bdev->brd_info_list.PCIEUNI_PROJ_NEXT = tmp_data_32;
        }
    }
    
    strbrd = bdev->startup_brd;

    return strbrd;
}
EXPORT_SYMBOL(pcieuni_get_brdinfo);

int   pcieuni_fill_prj_info(struct pcieuni_dev  *bdev, void  *baddress)
{
    void *address;
    int    strbrd  = 0;
    u32  tmp_data_32;
     struct pcieuni_prj_info  *tmp_prj_info_list; 
    
    address           = baddress;
    tmp_data_32  = ioread32(address );
    if(tmp_data_32 == ASCII_PROJ_MAGIC_NUM ){
        bdev->startup_prj_num++;
        tmp_prj_info_list = kzalloc(sizeof(pcieuni_prj_info), GFP_KERNEL);
        
        address = baddress + WORD_PROJ_ID;
        tmp_data_32       = ioread32(address);
       tmp_prj_info_list->PCIEUNI_PROJ_ID = tmp_data_32;

        address = baddress + WORD_PROJ_VERSION;
        tmp_data_32       = ioread32(address );
       tmp_prj_info_list->PCIEUNI_PROJ_VERSION = tmp_data_32;

        address = baddress + WORD_PROJ_DATE;
        tmp_data_32       = ioread32(address );
       tmp_prj_info_list->PCIEUNI_PROJ_DATE = tmp_data_32;

        address = baddress + WORD_PROJ_RESERVED;
        tmp_data_32       = ioread32(address );
       tmp_prj_info_list->PCIEUNI_PROJ_RESERVED = tmp_data_32;

        bdev->brd_info_list.PCIEUNI_PROJ_NEXT = 0;
        address = baddress + WORD_PROJ_NEXT;
        tmp_data_32       = ioread32(address );
       tmp_prj_info_list->PCIEUNI_PROJ_NEXT = tmp_data_32;

        list_add(&(tmp_prj_info_list->prj_list), &(bdev->prj_info_list.prj_list));
        strbrd= tmp_data_32;
    }
    
    return strbrd;
}
EXPORT_SYMBOL(pcieuni_fill_prj_info);

int      pcieuni_get_prjinfo(struct pcieuni_dev  *bdev)
{
    void *baddress;
    void *address;
    int   strbrd             = 0;
    int  tmp_next_prj  = 0;
    int  tmp_next_prj1 = 0;
    
    bdev->startup_prj_num = 0;
    tmp_next_prj =bdev->brd_info_list.PCIEUNI_PROJ_NEXT;
    if(tmp_next_prj){
        baddress = bdev->memmory_base0;
        while(tmp_next_prj){
            address = baddress + tmp_next_prj;
            tmp_next_prj = pcieuni_fill_prj_info(bdev, address);
        }
    }else{
        if(bdev->memmory_base1){ 
            tmp_next_prj  = 1;
            tmp_next_prj1 = 0;
            baddress = bdev->memmory_base1;
            while(tmp_next_prj){
                tmp_next_prj  = tmp_next_prj1;
                address = baddress + tmp_next_prj;
                tmp_next_prj = pcieuni_fill_prj_info(bdev, address);
            }
        }
        if(bdev->memmory_base2){ 
            tmp_next_prj  = 1;
            tmp_next_prj1 = 0;
            baddress = bdev->memmory_base2;
            while(tmp_next_prj){
                tmp_next_prj  = tmp_next_prj1;
                address = baddress + tmp_next_prj;
                tmp_next_prj = pcieuni_fill_prj_info(bdev, address);
            }
        }
        if(bdev->memmory_base3){ 
            tmp_next_prj  = 1;
            tmp_next_prj1 = 0;
            baddress = bdev->memmory_base3;
            while(tmp_next_prj){
                tmp_next_prj  = tmp_next_prj1;
                address = baddress + tmp_next_prj;
                tmp_next_prj = pcieuni_fill_prj_info(bdev, address);
            }
        }
        if(bdev->memmory_base4){ 
            tmp_next_prj  = 1;
            tmp_next_prj1 = 0;
            baddress = bdev->memmory_base4;
            while(tmp_next_prj){
                tmp_next_prj  = tmp_next_prj1;
                address = baddress + tmp_next_prj;
                tmp_next_prj = pcieuni_fill_prj_info(bdev, address);
            }
        }
        if(bdev->memmory_base5){ 
            tmp_next_prj  = 1;
            tmp_next_prj1 = 0;
            baddress = bdev->memmory_base5;
            while(tmp_next_prj){
                tmp_next_prj  = tmp_next_prj1;
                address = baddress + tmp_next_prj;
                tmp_next_prj = pcieuni_fill_prj_info(bdev, address);
            }
        }
    }
    strbrd = bdev->startup_prj_num;
    return strbrd;
}
EXPORT_SYMBOL(pcieuni_get_prjinfo);

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
    void register_gpcieuni_proc(int num, char * dfn, struct pcieuni_dev     *p_upcie_dev, struct pcieuni_cdev     *p_upcie_cdev)
    {
        char prc_entr[32];
        sprintf(prc_entr, "%ss%i", dfn, num);
        p_upcie_cdev->pcieuni_procdir = create_proc_entry(prc_entr, S_IFREG | S_IRUGO, 0);
        p_upcie_cdev->pcieuni_procdir->read_proc = pcieuni_procinfo;
        p_upcie_cdev->pcieuni_procdir->data = p_upcie_dev;
    }

    void unregister_gpcieuni_proc(int num, char *dfn)
    {
        char prc_entr[32];
        sprintf(prc_entr, "%ss%i", dfn, num);
        remove_proc_entry(prc_entr,0);
    }

    int pcieuni_procinfo(char *buf, char **start, off_t fpos, int lenght, int *eof, void *data)
    {
        char *p;
        pcieuni_dev     *pcieuni_dev_m ;
        struct list_head *pos;
        struct pcieuni_prj_info  *tmp_prj_info_list;

        pcieuni_dev_m = (pcieuni_dev*)data;
        p = buf;
        p += sprintf(p,"GPCIEUNI Driver Version:\t%i.%i\n", pcieuni_dev_m->parent_dev->GPCIEUNI_VER_MAJ, 
                                                                                   pcieuni_dev_m->parent_dev->GPCIEUNI_VER_MIN);
        p += sprintf(p,"Driver Version:\t%i.%i\n", pcieuni_dev_m->parent_dev->PCIEUNI_DRV_VER_MAJ, 
                                                                                   pcieuni_dev_m->parent_dev->PCIEUNI_DRV_VER_MIN);
        p += sprintf(p,"Board NUM:\t%i\n", pcieuni_dev_m->brd_num);
        p += sprintf(p,"Slot    NUM:\t%i\n", pcieuni_dev_m->slot_num);
        p += sprintf(p,"Board ID:\t%X\n", pcieuni_dev_m->brd_info_list.PCIEUNI_BOARD_ID);
        p += sprintf(p,"Board Version;\t%X\n",pcieuni_dev_m->brd_info_list.PCIEUNI_BOARD_VERSION);
        p += sprintf(p,"Board Date:\t%X\n",pcieuni_dev_m->brd_info_list.PCIEUNI_BOARD_DATE);
        p += sprintf(p,"Board HW Ver:\t%X\n",pcieuni_dev_m->brd_info_list.PCIEUNI_HW_VERSION);
        p += sprintf(p,"Board Next Prj:\t%X\n",pcieuni_dev_m->brd_info_list.PCIEUNI_PROJ_NEXT);
        p += sprintf(p,"Board Reserved:\t%X\n",pcieuni_dev_m->brd_info_list.PCIEUNI_BOARD_RESERVED);
        p += sprintf(p,"Number of Proj:\t%i\n", pcieuni_dev_m->startup_prj_num);

        list_for_each(pos,  &pcieuni_dev_m->prj_info_list.prj_list ){
            tmp_prj_info_list = list_entry(pos, struct pcieuni_prj_info, prj_list);
            p += sprintf(p,"Project ID:\t%X\n", tmp_prj_info_list->PCIEUNI_PROJ_ID);
            p += sprintf(p,"Project Version:\t%X\n", tmp_prj_info_list->PCIEUNI_PROJ_VERSION);
            p += sprintf(p,"Project Date:\t%X\n", tmp_prj_info_list->PCIEUNI_PROJ_DATE);
            p += sprintf(p,"Project Reserver:\t%X\n", tmp_prj_info_list->PCIEUNI_PROJ_RESERVED);
            p += sprintf(p,"Project Next:\t%X\n", tmp_prj_info_list->PCIEUNI_PROJ_NEXT);
        }

        *eof = 1;
        return p - buf;
    }
#else
    void register_gpcieuni_proc(int num, char * dfn, struct pcieuni_dev     *p_upcie_dev, struct pcieuni_cdev     *p_upcie_cdev)
    {
        char prc_entr[32];
        sprintf(prc_entr, "%ss%i", dfn, num);
        p_upcie_cdev->pcieuni_procdir = proc_create_data(prc_entr, S_IFREG | S_IRUGO, 0, &gpcieuni_proc_fops, p_upcie_dev); 
    }

    void unregister_gpcieuni_proc(int num, char *dfn)
    {
        char prc_entr[32];
        sprintf(prc_entr, "%ss%i", dfn, num);
        remove_proc_entry(prc_entr,0);
    }

   ssize_t pcieuni_procinfo(struct file *filp,char *buf,size_t count,loff_t *offp)
{
    char *p;
    int cnt = 0;
    pcieuni_dev     *pcieuni_dev_m ;
    struct list_head *pos;
    struct pcieuni_prj_info  *tmp_prj_info_list;
    pcieuni_dev_m=PDE_DATA(file_inode(filp));
        
    printk(KERN_INFO "PCIEUNI_PROC_INFO CALLEDi\n");

    p = buf;
    p += sprintf(p,"GPCIEUNI Driver Version:\t%i.%i\n", pcieuni_dev_m->parent_dev->GPCIEUNI_VER_MAJ, 
                                                                               pcieuni_dev_m->parent_dev->GPCIEUNI_VER_MIN);
    p += sprintf(p,"Driver Version:\t%i.%i\n", pcieuni_dev_m->parent_dev->PCIEUNI_DRV_VER_MAJ, 
                                                                               pcieuni_dev_m->parent_dev->PCIEUNI_DRV_VER_MIN);
    p += sprintf(p,"Board NUM:\t%i\n", pcieuni_dev_m->brd_num);
    p += sprintf(p,"Slot    NUM:\t%i\n", pcieuni_dev_m->slot_num);
    p += sprintf(p,"Board ID:\t%X\n", pcieuni_dev_m->brd_info_list.PCIEUNI_BOARD_ID);
    p += sprintf(p,"Board Version;\t%X\n",pcieuni_dev_m->brd_info_list.PCIEUNI_BOARD_VERSION);
    p += sprintf(p,"Board Date:\t%X\n",pcieuni_dev_m->brd_info_list.PCIEUNI_BOARD_DATE);
    p += sprintf(p,"Board HW Ver:\t%X\n",pcieuni_dev_m->brd_info_list.PCIEUNI_HW_VERSION);
    p += sprintf(p,"Board Next Prj:\t%X\n",pcieuni_dev_m->brd_info_list.PCIEUNI_PROJ_NEXT);
    p += sprintf(p,"Board Reserved:\t%X\n",pcieuni_dev_m->brd_info_list.PCIEUNI_BOARD_RESERVED);
    p += sprintf(p,"Number of Proj:\t%i\n", pcieuni_dev_m->startup_prj_num);
    
    list_for_each(pos,  &pcieuni_dev_m->prj_info_list.prj_list ){
        tmp_prj_info_list = list_entry(pos, struct pcieuni_prj_info, prj_list);
        p += sprintf(p,"Project ID:\t%X\n", tmp_prj_info_list->PCIEUNI_PROJ_ID);
        p += sprintf(p,"Project Version:\t%X\n", tmp_prj_info_list->PCIEUNI_PROJ_VERSION);
        p += sprintf(p,"Project Date:\t%X\n", tmp_prj_info_list->PCIEUNI_PROJ_DATE);
        p += sprintf(p,"Project Reserver:\t%X\n", tmp_prj_info_list->PCIEUNI_PROJ_RESERVED);
        p += sprintf(p,"Project Next:\t%X\n", tmp_prj_info_list->PCIEUNI_PROJ_NEXT);
    }

    p += sprintf(p,"\0");
    cnt = strlen(p);
    printk(KERN_INFO "PCIEUNI_PROC_INFO: PROC LEN%i\n", cnt);
    copy_to_user(buf,p, (size_t)cnt);
    return cnt;
}
#endif
EXPORT_SYMBOL(pcieuni_procinfo);

/**
 * @brief Writes 32bit value to memory mapped device register
 * 
 * If parameter @param ensureFlush is set to true the function will try to make sure that write is flushed to device. 
 * Flushing is a problem because PCI bus writes are posted asynchronously (see 
 * <a href="https://www.kernel.org/doc/htmldocs/deviceiobook/accessing_the_device.html">deviceiobook in  Linux Kernel 
 * HTML Documentation</a>). In principle PCI bus should automatically flush everything before next ioread is serviced. 
 * There are sill a couple of layers before writes make it to the board, but usually we only care that order of writes
 * is correct - any writes comming after the read should come to the board after the previous writes. However there 
 * seem to be some problem with this assumption therefore we make an addtional 5 microseconds delay after a write that 
 * has to commmit.
 * 
 * @param dev         Target device
 * @param bar         Target BAR
 * @param offset      Offset of target register within the BAR
 * @param value       Value to write to target register
 * @param ensureFlush Ensure write operation is flushed to device before function returns.
 * 
 * @retval  0     Success
 * @retval  -EIO  Failure
 */
int pcieuni_register_write32(struct pcieuni_dev *dev, void* bar, u32 offset, u32 value, bool ensureFlush)
{
    void *address = (void*)(bar + offset);
    u32 readbackData;
    
    // Write to device register
    iowrite32(value, address);
    
    if (ensureFlush)
    {
        // force CPU write flush
        smp_wmb();
        
        // Read request is supposed to block until PCIe bus flushes the pending writes
        readbackData = ioread32(bar);
        smp_rmb();
        
        // Experimental: additional wait to let TLPs reach the board
        udelay(5);
    }
    
    return 0; 
}
EXPORT_SYMBOL(pcieuni_register_write32);
