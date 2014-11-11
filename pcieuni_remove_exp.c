#include <linux/module.h>
#include <linux/fs.h>	
#include <linux/proc_fs.h>

#include "pcieuni_ufn.h"

int pcieuni_remove_exp(struct pci_dev *dev, pcieuni_cdev  **pcieuni_cdev_p, char *dev_name, int * brd_num)
{
     pcieuni_dev                *pcieunidev;
     pcieuni_cdev              *pcieuni_cdev_m;
     int                    tmp_dev_num  = 0;
     int                    tmp_slot_num  = 0;
     int                    brdNum            = 0;
     int                    brdCnt              = 0;
     int                    m_brdNum      = 0;
     char                prc_entr[64];
     dev_t              devno ;
     
     struct list_head *pos;
     struct list_head *npos;
     struct pcieuni_prj_info  *tmp_prj_info_list;
     
     printk(KERN_ALERT "PCIEUNI_REMOVE_EXP CALLED\n");
    
    pcieuni_cdev_m = *pcieuni_cdev_p;
    devno = MKDEV(pcieuni_cdev_m->PCIEUNI_MAJOR, pcieuni_cdev_m->PCIEUNI_MINOR);
    pcieunidev = dev_get_drvdata(&(dev->dev));
    tmp_dev_num  = pcieunidev->dev_num;
    tmp_slot_num  = pcieunidev->slot_num;
    m_brdNum       = pcieunidev->brd_num;
    * brd_num        = tmp_slot_num;
    sprintf(prc_entr, "%ss%d", dev_name, tmp_slot_num);
    printk(KERN_ALERT "PCIEUNI_REMOVE: SLOT %d DEV %d BOARD %i\n", tmp_slot_num, tmp_dev_num, m_brdNum);
    
    
    /* now let's be good and free the proj_info_list items. since we will be removing items
     * off the list using list_del() we need to use a safer version of the list_for_each() 
     * macro aptly named list_for_each_safe(). Note that you MUST use this macro if the loop 
     * involves deletions of items (or moving items from one list to another).
     */
    list_for_each_safe(pos,  npos, &pcieunidev->prj_info_list.prj_list ){
        tmp_prj_info_list = list_entry(pos, struct pcieuni_prj_info, prj_list);
        list_del(pos);
        kfree(tmp_prj_info_list);
    }
    
    printk(KERN_ALERT "REMOVING IRQ_MODE %d\n", pcieunidev->irq_mode);
    if(pcieunidev->irq_mode){
       printk(KERN_ALERT "FREE IRQ\n");
       free_irq(pcieunidev->pci_dev_irq, pcieunidev);
       printk(KERN_ALERT "REMOVING IRQ\n");
       if(pcieunidev->msi){
           printk(KERN_ALERT "DISABLE MSI\n");
           pci_disable_msi((pcieunidev->pcieuni_pci_dev));
       }
    }else{
        if(pcieunidev->msi){
           printk(KERN_ALERT "DISABLE MSI\n");
           pci_disable_msi((pcieunidev->pcieuni_pci_dev));
       }
    }
     
    printk(KERN_ALERT "REMOVE: UNMAPPING MEMORYs\n");
    mutex_lock_interruptible(&pcieunidev->dev_mut);
                    
    if(pcieunidev->memmory_base0){
       pci_iounmap(dev, pcieunidev->memmory_base0);
       pcieunidev->memmory_base0  = 0;
       pcieunidev->mem_base0      = 0;
       pcieunidev->mem_base0_end  = 0;
       pcieunidev->mem_base0_flag = 0;
       pcieunidev->rw_off0         = 0;
    }
    if(pcieunidev->memmory_base1){
       pci_iounmap(dev, pcieunidev->memmory_base1);
       pcieunidev->memmory_base1  = 0;
       pcieunidev->mem_base1      = 0;
       pcieunidev->mem_base1_end  = 0;
       pcieunidev->mem_base1_flag = 0;
       pcieunidev->rw_off1        = 0;
    }
    if(pcieunidev->memmory_base2){
       pci_iounmap(dev, pcieunidev->memmory_base2);
       pcieunidev->memmory_base2  = 0;
       pcieunidev->mem_base2      = 0;
       pcieunidev->mem_base2_end  = 0;
       pcieunidev->mem_base2_flag = 0;
       pcieunidev->rw_off2        = 0;
    }
    if(pcieunidev->memmory_base3){
       pci_iounmap(dev, pcieunidev->memmory_base3);
       pcieunidev->memmory_base3  = 0;
       pcieunidev->mem_base3      = 0;
       pcieunidev->mem_base3_end  = 0;
       pcieunidev->mem_base3_flag = 0;
       pcieunidev->rw_off3         = 0;
    }
    if(pcieunidev->memmory_base4){
       pci_iounmap(dev, pcieunidev->memmory_base4);
       pcieunidev->memmory_base4  = 0;
       pcieunidev->mem_base4      = 0;
       pcieunidev->mem_base4_end  = 0;
       pcieunidev->mem_base4_flag = 0;
       pcieunidev->rw_off4        = 0;
    }
    if(pcieunidev->memmory_base5){
       pci_iounmap(dev, pcieunidev->memmory_base5);
       pcieunidev->memmory_base5  = 0;
       pcieunidev->mem_base5      = 0;
       pcieunidev->mem_base5_end  = 0;
       pcieunidev->mem_base5_flag = 0;
       pcieunidev->rw_off5        = 0;
    }
    pci_release_regions((pcieunidev->pcieuni_pci_dev));
    mutex_unlock(&pcieunidev->dev_mut);
    printk(KERN_INFO "PCIEUNI_REMOVE:  DESTROY DEVICE MAJOR %i MINOR %i\n",
               pcieuni_cdev_m->PCIEUNI_MAJOR, (pcieuni_cdev_m->PCIEUNI_MINOR + pcieunidev->brd_num));
    device_destroy(pcieuni_cdev_m->pcieuni_class,  MKDEV(pcieuni_cdev_m->PCIEUNI_MAJOR, 
                                                  pcieuni_cdev_m->PCIEUNI_MINOR + pcieunidev->brd_num));
    
    unregister_gpcieuni_proc(tmp_slot_num, dev_name);
   // remove_proc_entry(prc_entr,0);
    
    pcieunidev->dev_sts   = 0;
    pcieuni_cdev_m->pcieuniModuleNum --;
    pci_disable_device(dev);
    mutex_destroy(&pcieunidev->dev_mut);
    cdev_del(&pcieunidev->cdev);
    brdNum = pcieunidev->brd_num;
    kfree(pcieuni_cdev_m->pcieuni_dev_m[brdNum]);
    pcieuni_cdev_m->pcieuni_dev_m[brdNum] = 0;
    
    /*Check if no more modules free main chrdev structure */
    brdCnt             = 0;
    if(pcieuni_cdev_m){
        for(brdNum = 0;brdNum < PCIEUNI_NR_DEVS;brdNum++){
            if(pcieuni_cdev_m->pcieuni_dev_m[brdNum]){
                printk(KERN_INFO "PCIEUNI_REMOVE:  EXIST BOARD %i\n", brdNum);
                brdCnt++;
            }
        }
        if(!brdCnt){
            printk(KERN_INFO "PCIEUNI_REMOVE:  NO MORE BOARDS\n");
            unregister_chrdev_region(devno, PCIEUNI_NR_DEVS);
            class_destroy(pcieuni_cdev_m->pcieuni_class);
            //kfree(pcieuni_cdev_m);
            kfree(*pcieuni_cdev_p);
            pcieuni_cdev_m  = 0;
            *pcieuni_cdev_p = 0;
        }
    }
    return 0;
}
EXPORT_SYMBOL(pcieuni_remove_exp);
