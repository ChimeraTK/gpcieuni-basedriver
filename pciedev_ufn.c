

#include <linux/module.h>
#include <linux/fs.h>	
#include <linux/sched.h>

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

int    pciedev_set_drvdata(struct pciedev_dev *dev, void *data)
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

void*   pciedev_get_baddress(int br_num, struct pciedev_dev  *dev)
{
    if ( (br_num < 0) || (br_num >= PCIEDEV_N_BARS) ){
      return NULL;
    }

    return dev->memmory_base[br_num];
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
    if(bdev->memmory_base[0]){ 
        baddress = bdev->memmory_base[0];
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
    void *address = NULL;
    unsigned int nextProjectOffset = 0;
    unsigned int bar = 0;
    
    /* Set the number of projects found to 0 */
    bdev->startup_prj_num = 0;

    /* Special treatment for BAR 0:
       The first project in bar 0 is not at 0, but in the address contained at PCIEDEV_PROJ_NEXT
       of the board header.*/
    nextProjectOffset = bdev->brd_info_list.PCIEDEV_PROJ_NEXT;

    /* Rejecting loop, first project is NOT at 0, and there might be none.*/
    while(nextProjectOffset){
      /* FIXME: This mixes pointers and integers. Increpenting a pointer with a uint might be OK, though. */
      address = bdev->memmory_base[0] + nextProjectOffset;
      nextProjectOffset = pciedev_fill_prj_info(bdev, address);
    }

    /* loop all bars for the next project */
    for (bar=0; bar < PCIEDEV_N_BARS; ++bar){

      /* only try to access the bar if it is implemented */
      if(bdev->memmory_base[bar]){ 

	/* Use a non-rejecting loop because the first project is at 0 (except for bar 0).
	   This is safe if we assume that the bar is not empty. */
	do{ 
	  address = bdev->memmory_base[bar] + nextProjectOffset;
	  nextProjectOffset = pciedev_fill_prj_info(bdev, nextProjectOffset);
	  /* if the next project is 0 the loops exits, leaving the nextProjectOffset correclty
	     initialised with 0 for the next BAR */

	}while(nextProjectOffset);

      }/* if bar is implemented */
    }/* for (bar) */

    /* return the number of projects found. */
    return bdev->startup_prj_num;
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
