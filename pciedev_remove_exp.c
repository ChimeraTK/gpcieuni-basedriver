#include <linux/module.h>
#include <linux/fs.h>	
#include <linux/proc_fs.h>

#include "pciedev_ufn.h"

int pciedev_remove_exp(struct pci_dev *dev, pciedev_cdev  **pciedev_cdev_p, char *dev_name, int * brd_num)
{
     pciedev_dev                *pciedevdev;
     pciedev_cdev              *pciedev_cdev_m;
     int                    tmp_dev_num  = 0;
     int                    tmp_slot_num  = 0;
     int                    brdNum            = 0;
     int                    brdCnt              = 0;
     int                    m_brdNum      = 0;
     char                f_name[64];
     char                prc_entr[64];
     dev_t              devno ;
     
     struct list_head *pos;
     struct list_head *npos;
     struct pciedev_prj_info  *tmp_prj_info_list;
     
     printk(KERN_ALERT "PCIEDEV_REMOVE_EXP CALLED\n");
    
    pciedev_cdev_m = *pciedev_cdev_p;
    devno = MKDEV(pciedev_cdev_m->PCIEDEV_MAJOR, pciedev_cdev_m->PCIEDEV_MINOR);
    pciedevdev = dev_get_drvdata(&(dev->dev));
    tmp_dev_num  = pciedevdev->dev_num;
    tmp_slot_num  = pciedevdev->slot_num;
    m_brdNum       = pciedevdev->brd_num;
    * brd_num        = tmp_slot_num;
    sprintf(f_name, "%ss%d", dev_name, tmp_slot_num);
    sprintf(prc_entr, "%ss%d", dev_name, tmp_slot_num);
    printk(KERN_ALERT "PCIEDEV_REMOVE: SLOT %d DEV %d BOARD %i\n", tmp_slot_num, tmp_dev_num, m_brdNum);
    
    /* now let's be good and free the proj_info_list items. since we will be removing items
     * off the list using list_del() we need to use a safer version of the list_for_each() 
     * macro aptly named list_for_each_safe(). Note that you MUST use this macro if the loop 
     * involves deletions of items (or moving items from one list to another).
     */
    list_for_each_safe(pos,  npos, &pciedevdev->prj_info_list.prj_list ){
        tmp_prj_info_list = list_entry(pos, struct pciedev_prj_info, prj_list);
        list_del(pos);
        kfree(tmp_prj_info_list);
    }
    
    printk(KERN_ALERT "REMOVING IRQ_MODE %d\n", pciedevdev->irq_mode);
    if(pciedevdev->irq_mode){
       printk(KERN_ALERT "FREE IRQ\n");
       free_irq(pciedevdev->pci_dev_irq, pciedevdev);
       printk(KERN_ALERT "REMOVING IRQ\n");
       if(pciedevdev->msi){
           printk(KERN_ALERT "DISABLE MSI\n");
           pci_disable_msi((pciedevdev->pciedev_pci_dev));
       }
    }else{
        if(pciedevdev->msi){
           printk(KERN_ALERT "DISABLE MSI\n");
           pci_disable_msi((pciedevdev->pciedev_pci_dev));
       }
    }
     
    printk(KERN_ALERT "REMOVE: UNMAPPING MEMORYs\n");
    mutex_lock_interruptible(&pciedevdev->dev_mut);
                    
    if(pciedevdev->memmory_base0){
       pci_iounmap(dev, pciedevdev->memmory_base0);
       pciedevdev->memmory_base0  = 0;
       pciedevdev->mem_base0      = 0;
       pciedevdev->mem_base0_end  = 0;
       pciedevdev->mem_base0_flag = 0;
       pciedevdev->rw_off0         = 0;
    }
    if(pciedevdev->memmory_base1){
       pci_iounmap(dev, pciedevdev->memmory_base1);
       pciedevdev->memmory_base1  = 0;
       pciedevdev->mem_base1      = 0;
       pciedevdev->mem_base1_end  = 0;
       pciedevdev->mem_base1_flag = 0;
       pciedevdev->rw_off1        = 0;
    }
    if(pciedevdev->memmory_base2){
       pci_iounmap(dev, pciedevdev->memmory_base2);
       pciedevdev->memmory_base2  = 0;
       pciedevdev->mem_base2      = 0;
       pciedevdev->mem_base2_end  = 0;
       pciedevdev->mem_base2_flag = 0;
       pciedevdev->rw_off2        = 0;
    }
    if(pciedevdev->memmory_base3){
       pci_iounmap(dev, pciedevdev->memmory_base3);
       pciedevdev->memmory_base3  = 0;
       pciedevdev->mem_base3      = 0;
       pciedevdev->mem_base3_end  = 0;
       pciedevdev->mem_base3_flag = 0;
       pciedevdev->rw_off3         = 0;
    }
    if(pciedevdev->memmory_base4){
       pci_iounmap(dev, pciedevdev->memmory_base4);
       pciedevdev->memmory_base4  = 0;
       pciedevdev->mem_base4      = 0;
       pciedevdev->mem_base4_end  = 0;
       pciedevdev->mem_base4_flag = 0;
       pciedevdev->rw_off4        = 0;
    }
    if(pciedevdev->memmory_base5){
       pci_iounmap(dev, pciedevdev->memmory_base5);
       pciedevdev->memmory_base5  = 0;
       pciedevdev->mem_base5      = 0;
       pciedevdev->mem_base5_end  = 0;
       pciedevdev->mem_base5_flag = 0;
       pciedevdev->rw_off5        = 0;
    }
    pci_release_regions((pciedevdev->pciedev_pci_dev));
    mutex_unlock(&pciedevdev->dev_mut);
    printk(KERN_INFO "PCIEDEV_REMOVE:  DESTROY DEVICE MAJOR %i MINOR %i\n",
               pciedev_cdev_m->PCIEDEV_MAJOR, (pciedev_cdev_m->PCIEDEV_MINOR + pciedevdev->brd_num));
    device_destroy(pciedev_cdev_m->pciedev_class,  MKDEV(pciedev_cdev_m->PCIEDEV_MAJOR, 
                                                  pciedev_cdev_m->PCIEDEV_MINOR + pciedevdev->brd_num));
    remove_proc_entry(prc_entr,0);
    
    pciedevdev->dev_sts   = 0;
    pciedev_cdev_m->pciedevModuleNum --;
    pci_disable_device(dev);
    mutex_destroy(&pciedevdev->dev_mut);
    cdev_del(&pciedevdev->cdev);
    brdNum = pciedevdev->brd_num;
    kfree(pciedev_cdev_m->pciedev_dev_m[brdNum]);
    pciedev_cdev_m->pciedev_dev_m[brdNum] = 0;
    
    /*Check if no more modules free main chrdev structure */
    brdCnt             = 0;
    if(pciedev_cdev_m){
        for(brdNum = 0;brdNum < PCIEDEV_NR_DEVS;brdNum++){
            if(pciedev_cdev_m->pciedev_dev_m[brdNum]){
                printk(KERN_INFO "PCIEDEV_REMOVE:  EXIST BOARD %i\n", brdNum);
                brdCnt++;
            }
        }
        if(!brdCnt){
            printk(KERN_INFO "PCIEDEV_REMOVE:  NO MORE BOARDS\n");
            unregister_chrdev_region(devno, PCIEDEV_NR_DEVS);
            class_destroy(pciedev_cdev_m->pciedev_class);
            //kfree(pciedev_cdev_m);
            kfree(*pciedev_cdev_p);
            pciedev_cdev_m  = 0;
            *pciedev_cdev_p = 0;
        }
    }
    return 0;
}
EXPORT_SYMBOL(pciedev_remove_exp);
