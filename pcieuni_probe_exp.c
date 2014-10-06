#include <linux/module.h>
#include <linux/fs.h>	
#include <linux/proc_fs.h>

#include "pcieuni_ufn.h"

int    pcieuni_probe_exp(struct pci_dev *dev, const struct pci_device_id *id, 
            struct file_operations *pcieuni_fops, pcieuni_cdev **pcieuni_cdev_pp, char *dev_name, int * brd_num)
{
    
    dev_t devt              = 0;
    int    devno             = 0;
    int    i                      = 0;
    int    m_brdNum   = 0;
    pcieuni_dev     *m_pcieuni_dev_p;
    pcieuni_cdev   *pcieuni_cdev_p;
 
    int     err       = 0;
    int     result   = 0;
    int     tmp_info = 0;
    char  **endptr = 0;
    
    u16 vendor_id;
    u16 device_id;
    u8  revision;
    u8  irq_line;
    u8  irq_pin;
    u32 res_start;
    u32 res_end;
    u32 res_flag;
    int pcie_cap;
    u32 tmp_slot_cap     = 0;
    int tmp_slot_num     = 0;
    int tmp_dev_num      = 0;
    int tmp_bus_func     = 0;
    
     int cur_mask = 0;
    u8  dev_payload;
    u32 tmp_payload_size = 0;

    u16 subvendor_id;
    u16 subdevice_id;
    u16 class_code;
    u32 tmp_devfn;
    u32 busNumber;
    u32 devNumber;
    u32 funcNumber;
    
    char prc_entr[64];
    	
    printk(KERN_ALERT "############PCIEUNI_PROBE THIS IS U_FUNCTION NAME %s\n", dev_name);
    
   if(!( *pcieuni_cdev_pp)){
        printk(KERN_ALERT "PCIEUNI_PROBE: CREATING CDEV_M\n");
        pcieuni_cdev_p= kzalloc(sizeof(pcieuni_cdev), GFP_KERNEL);
        if(!pcieuni_cdev_p){
            printk(KERN_ALERT "AFTER_INIT CREATE CDEV STRUCT NO MEM\n");
            return -ENOMEM;
        }
         *pcieuni_cdev_pp = pcieuni_cdev_p;
        pcieuni_cdev_p->PCIEUNI_MAJOR             = 47;
        pcieuni_cdev_p->PCIEUNI_MINOR             = 0;
        pcieuni_cdev_p->pcieuniModuleNum         = 0;
        pcieuni_cdev_p->PCIEUNI_DRV_VER_MAJ = 1;
        pcieuni_cdev_p->PCIEUNI_DRV_VER_MIN = 1;
        pcieuni_cdev_p->GPCIEUNI_VER_MAJ        = 1;
        pcieuni_cdev_p->GPCIEUNI_VER_MIN        = 1;
        
        result = alloc_chrdev_region(&devt, pcieuni_cdev_p->PCIEUNI_MINOR, PCIEUNI_NR_DEVS, dev_name);
        pcieuni_cdev_p->PCIEUNI_MAJOR = MAJOR(devt);
        printk(KERN_ALERT "PCIEUNI_PROBE: DRV_MAJOR is %d\n", pcieuni_cdev_p->PCIEUNI_MAJOR);
        /* Populate sysfs entries */
        pcieuni_cdev_p->pcieuni_class = class_create(pcieuni_fops->owner, dev_name);
        /*Get module driver version information*/
        printk(KERN_ALERT "&&&&&PCIEUNI_PROBE CALLED; GPCIEUNI MODULE VERSION %s \n", THIS_MODULE->version);
        pcieuni_cdev_p->GPCIEUNI_VER_MAJ = simple_strtol(THIS_MODULE->version, endptr, 10);
        pcieuni_cdev_p->GPCIEUNI_VER_MIN = simple_strtol(THIS_MODULE->version + 2, endptr, 10);
        printk(KERN_ALERT "&&&&&PCIEUNI_PROBE CALLED; GPCIEUNI MODULE VERSION %i.%i\n", 
                pcieuni_cdev_p->GPCIEUNI_VER_MAJ, pcieuni_cdev_p->GPCIEUNI_VER_MIN);
        
        printk(KERN_ALERT "&&&&&PCIEUNI_PROBE CALLED; THIS MODULE VERSION %s \n", pcieuni_fops->owner->version);
        pcieuni_cdev_p->PCIEUNI_DRV_VER_MAJ = simple_strtol(pcieuni_fops->owner->version, endptr, 10);
        pcieuni_cdev_p->PCIEUNI_DRV_VER_MIN = simple_strtol(pcieuni_fops->owner->version + 2, endptr, 10);
        printk(KERN_ALERT "&&&&&PCIEUNI_PROBE CALLED; THIS MODULE VERSION %i.%i\n", 
                pcieuni_cdev_p->PCIEUNI_DRV_VER_MAJ, pcieuni_cdev_p->PCIEUNI_DRV_VER_MIN);
    }else{
        printk(KERN_ALERT "PCIEUNI_PROBE: CDEV_M EXIST\n");
        pcieuni_cdev_p     = *pcieuni_cdev_pp;
    }
    
    /*************************CDEV INIT******************************************************/
    printk(KERN_WARNING "PCIEUNIINIT_MODULE CALLED\n");
    
    /*setup device*/
    for(m_brdNum = 0;m_brdNum < PCIEUNI_NR_DEVS;m_brdNum++){
        if(!pcieuni_cdev_p->pcieuni_dev_m[m_brdNum])
        break;
    }
    if(m_brdNum == PCIEUNI_NR_DEVS){
        printk(KERN_ALERT "AFTER_INIT NO MORE DEVICES is %d\n", m_brdNum);
        return -1;
    }
    m_pcieuni_dev_p = kzalloc(sizeof(pcieuni_dev), GFP_KERNEL);
    if(!m_pcieuni_dev_p){
        printk(KERN_ALERT "AFTER_INIT CREATE DEV STRUCT NO MEM\n");
        return -ENOMEM;
    }
    printk(KERN_ALERT "PCIEUNI_PROBE_DEV_INIT: BOARD NUM %i\n", m_brdNum);
    * brd_num = m_brdNum;
    pcieuni_cdev_p->pcieuni_dev_m[m_brdNum]= m_pcieuni_dev_p;
    m_pcieuni_dev_p->parent_dev = pcieuni_cdev_p;
    devno = MKDEV(pcieuni_cdev_p->PCIEUNI_MAJOR, pcieuni_cdev_p->PCIEUNI_MINOR + m_brdNum);
    cdev_init(&(m_pcieuni_dev_p->cdev), pcieuni_fops);
    m_pcieuni_dev_p->cdev.owner = THIS_MODULE;
    m_pcieuni_dev_p->cdev.ops = pcieuni_fops;
    result = cdev_add(&(m_pcieuni_dev_p->cdev), devno, 1);
    if (result){
      printk(KERN_NOTICE "Error %d adding devno%d num%d\n", result, devno, i);
      return 1;
    }
    //INIT_LIST_HEAD(&(m_pcieuni_dev_p->brd_info_list));
    INIT_LIST_HEAD(&(m_pcieuni_dev_p->prj_info_list.prj_list));
    mutex_init(&(m_pcieuni_dev_p->dev_mut));
    m_pcieuni_dev_p->dev_sts                  = 0;
    m_pcieuni_dev_p->irq_mode               = 0;
    m_pcieuni_dev_p->msi                        = 0;
    m_pcieuni_dev_p->dev_dma_64mask  = 0;
    m_pcieuni_dev_p->pcieuni_all_mems = 0;
    m_pcieuni_dev_p->brd_num                = m_brdNum;
    m_pcieuni_dev_p->dev_minor             = pcieuni_cdev_p->PCIEUNI_MINOR + m_brdNum;
    /*************************END CDEV INIT*************************************************/
    
    if ((err= pci_enable_device(dev)))
            return err;
    err = pci_request_regions(dev, dev_name);
    if (err ){
        pci_disable_device(dev);
        pci_set_drvdata(dev, NULL);
        return err;
    }
    pci_set_master(dev);
    
    tmp_devfn  = (u32)dev->devfn;
    busNumber  = (u32)dev->bus->number;
    devNumber  = (u32)PCI_SLOT(tmp_devfn);
    funcNumber = (u32)PCI_FUNC(tmp_devfn);
    tmp_bus_func = ((busNumber & 0xFF)<<8) + (devNumber & 0xFF);
    printk(KERN_ALERT "PCIEUNI_PROBE:DEVFN %X, BUS_NUM %X, DEV_NUM %X, FUNC_NUM %X, BUS_FUNC %x\n",
                                      tmp_devfn, busNumber, devNumber, funcNumber, tmp_bus_func);

    tmp_devfn  = (u32)dev->bus->self->devfn;
    busNumber  = (u32)dev->bus->self->bus->number;
    devNumber  = (u32)PCI_SLOT(tmp_devfn);
    funcNumber = (u32)PCI_FUNC(tmp_devfn);
    printk(KERN_ALERT "PCIEUNI_PROBE:DEVFN %X, BUS_NUM %X, DEV_NUM %X, FUNC_NUM %X\n",
                                      tmp_devfn, busNumber, devNumber, funcNumber);
    
    pcie_cap = pci_find_capability (dev->bus->self, PCI_CAP_ID_EXP);
    printk(KERN_INFO "PCIEUNI_PROBE: PCIE SWITCH CAP address %X\n",pcie_cap);
    
    pci_read_config_dword(dev->bus->self, (pcie_cap +PCI_EXP_SLTCAP), &tmp_slot_cap);
    tmp_slot_num = (tmp_slot_cap >> 19);
    tmp_dev_num  = tmp_slot_num;
    printk(KERN_ALERT "PCIEUNI_PROBE:SLOT NUM %d DEV NUM%d SLOT_CAP %X\n",tmp_slot_num,tmp_dev_num,tmp_slot_cap);
    
    m_pcieuni_dev_p->slot_num  = tmp_slot_num;
    m_pcieuni_dev_p->dev_num   = tmp_dev_num;
    m_pcieuni_dev_p->bus_func  = tmp_bus_func;
    m_pcieuni_dev_p->pcieuni_pci_dev = dev;

    dev_set_drvdata(&(dev->dev), m_pcieuni_dev_p);
    
    pcie_cap = pci_find_capability (dev, PCI_CAP_ID_EXP);
    printk(KERN_ALERT "DAMC_PROBE: PCIE CAP address %X\n",pcie_cap);
    pci_read_config_byte(dev, (pcie_cap +PCI_EXP_DEVCAP), &dev_payload);
    dev_payload &=0x0003;
    printk(KERN_ALERT "DAMC_PROBE: DEVICE CAP  %X\n",dev_payload);

    switch(dev_payload){
        case 0:
                   tmp_payload_size = 128;
                   break;
        case 1:
                   tmp_payload_size = 256;
                   break;
        case 2:
                   tmp_payload_size = 512;
                   break;
        case 3:
                   tmp_payload_size = 1024;	
                   break;
        case 4:
                   tmp_payload_size = 2048;	
                   break;
        case 5:
                   tmp_payload_size = 4096;	
                   break;
    }
    /*tmp_payload_size = 128; */
    printk(KERN_ALERT "DAMC: DEVICE PAYLOAD  %d\n",tmp_payload_size);
    

    if (!(cur_mask = pci_set_dma_mask(dev, DMA_BIT_MASK(64))) &&
        !(cur_mask = pci_set_consistent_dma_mask(dev, DMA_BIT_MASK(64)))) {
            m_pcieuni_dev_p->dev_dma_64mask = 1;
            printk(KERN_ALERT "CURRENT 64MASK %i\n", cur_mask);
    } else {
            if ((err = pci_set_dma_mask(dev, DMA_BIT_MASK(32))) &&
                (err = pci_set_consistent_dma_mask(dev, DMA_BIT_MASK(32)))) {
                    printk(KERN_ALERT "No usable DMA configuration\n");
            }else{
            m_pcieuni_dev_p->dev_dma_64mask = 0;
            printk(KERN_ALERT "CURRENT 32MASK %i\n", cur_mask);
            }
    }

    
    
    m_pcieuni_dev_p->pcieuni_all_mems = 0;
    
    
    pci_read_config_word(dev, PCI_VENDOR_ID,   &vendor_id);
    pci_read_config_word(dev, PCI_DEVICE_ID,   &device_id);
    pci_read_config_word(dev, PCI_SUBSYSTEM_VENDOR_ID,   &subvendor_id);
    pci_read_config_word(dev, PCI_SUBSYSTEM_ID,   &subdevice_id);
    pci_read_config_word(dev, PCI_CLASS_DEVICE,   &class_code);
    pci_read_config_byte(dev, PCI_REVISION_ID, &revision);
    pci_read_config_byte(dev, PCI_INTERRUPT_LINE, &irq_line);
    pci_read_config_byte(dev, PCI_INTERRUPT_PIN, &irq_pin);
    
    printk(KERN_INFO "PCIEUNI_PROBE: VENDOR_ID  %i\n",vendor_id);
    printk(KERN_INFO "PCIEUNI_PROBE: PCI_DEVICE_ID  %i\n",device_id);
    printk(KERN_INFO "PCIEUNI_PROBE: PCI_SUBSYSTEM_VENDOR_ID  %i\n",subvendor_id);
    printk(KERN_INFO "PCIEUNI_PROBE: PCI_SUBSYSTEM_ID  %i\n",subdevice_id);
    printk(KERN_INFO "PCIEUNI_PROBE: PCI_CLASS_DEVICE  %i\n",class_code);

    m_pcieuni_dev_p->vendor_id      = vendor_id;
    m_pcieuni_dev_p->device_id      = device_id;
    m_pcieuni_dev_p->subvendor_id   = subvendor_id;
    m_pcieuni_dev_p->subdevice_id   = subdevice_id;
    m_pcieuni_dev_p->class_code     = class_code;
    m_pcieuni_dev_p->revision       = revision;
    m_pcieuni_dev_p->irq_line       = irq_line;
    m_pcieuni_dev_p->irq_pin        = irq_pin;
    
    /*******SETUP BARs******/
    res_start  = pci_resource_start(dev, 0);
    res_end    = pci_resource_end(dev, 0);
    res_flag   = pci_resource_flags(dev, 0);
    m_pcieuni_dev_p->mem_base0       = res_start;
    m_pcieuni_dev_p->mem_base0_end   = res_end;
    m_pcieuni_dev_p->mem_base0_flag  = res_flag;
    if(res_start){
        m_pcieuni_dev_p->memmory_base0 = pci_iomap(dev, 0, (res_end - res_start));
        printk(KERN_INFO "PCIEUNI_PROBE: mem_region 0 address %X  SIZE %X FLAG %X\n",
                       res_start, (res_end - res_start),
                                                               m_pcieuni_dev_p->mem_base0_flag);
        m_pcieuni_dev_p->rw_off0 = (res_end - res_start);
        m_pcieuni_dev_p->pcieuni_all_mems = 1;
    }
    else{
      m_pcieuni_dev_p->memmory_base0 = 0;
      m_pcieuni_dev_p->rw_off0       = 0;
      printk(KERN_INFO "PCIEUNI: NO BASE0 address\n");
    }

    res_start   = pci_resource_start(dev, 1);
    res_end     = pci_resource_end(dev, 1);
    res_flag    = pci_resource_flags(dev, 1);
    m_pcieuni_dev_p->mem_base1       = res_start;
    m_pcieuni_dev_p->mem_base1_end   = res_end;
    m_pcieuni_dev_p->mem_base1_flag  = res_flag;
    if(res_start){
       m_pcieuni_dev_p->memmory_base1 = pci_iomap(dev, 1, (res_end - res_start));
       printk(KERN_INFO "PCIEUNI: mem_region 1 address %X \n", res_start);
       m_pcieuni_dev_p->rw_off1= (res_end - res_start);
       m_pcieuni_dev_p->pcieuni_all_mems +=2;
    }
    else{
      m_pcieuni_dev_p->memmory_base1 = 0;
      m_pcieuni_dev_p->rw_off1       = 0;
      printk(KERN_INFO "PCIEUNI: NO BASE1 address\n");
    }

    res_start  = pci_resource_start(dev, 2);
    res_end    = pci_resource_end(dev, 2);
    res_flag   = pci_resource_flags(dev, 2);
    m_pcieuni_dev_p->mem_base2       = res_start;
    m_pcieuni_dev_p->mem_base2_end   = res_end;
    m_pcieuni_dev_p->mem_base2_flag  = res_flag;
    if(res_start){
       m_pcieuni_dev_p->memmory_base2 = pci_iomap(dev, 2, (res_end - res_start));
       printk(KERN_INFO "PCIEUNI: mem_region 2 address %X \n", res_start);
        m_pcieuni_dev_p->rw_off2= (res_end - res_start);
        m_pcieuni_dev_p->pcieuni_all_mems += 4;
    }
    else{
      m_pcieuni_dev_p->memmory_base2 = 0;
      printk(KERN_INFO "PCIEUNI: NO BASE2 address\n");
    }

    res_start  = pci_resource_start(dev, 3);
    res_end    = pci_resource_end(dev, 3);
    res_flag   = pci_resource_flags(dev, 3);
    m_pcieuni_dev_p->mem_base3       = res_start;
    m_pcieuni_dev_p->mem_base3_end   = res_end;
    m_pcieuni_dev_p->mem_base3_flag  = res_flag;
    if(res_start){
        m_pcieuni_dev_p->memmory_base3 = pci_iomap(dev, 3, (res_end - res_start));
        printk(KERN_INFO "PCIEUNI: mem_region 3 address %X end %X SIZE %X FLAG %X\n",
                       res_start, res_end, (res_end - res_start),
                                                               m_pcieuni_dev_p->mem_base3_flag);
        m_pcieuni_dev_p->rw_off3 = (res_end - res_start);
        m_pcieuni_dev_p->pcieuni_all_mems += 8;
    }
    else{
      m_pcieuni_dev_p->memmory_base3 = 0;
      printk(KERN_INFO "PCIEUNI: NO BASE3 address\n");
    }

    res_start   = pci_resource_start(dev, 4);
    res_end     = pci_resource_end(dev, 4);
    res_flag    = pci_resource_flags(dev, 4);
    m_pcieuni_dev_p->mem_base4       = res_start;
    m_pcieuni_dev_p->mem_base4_end   = res_end;
    m_pcieuni_dev_p->mem_base4_flag  = res_flag;
    if(res_start){
       m_pcieuni_dev_p->memmory_base4 = pci_iomap(dev, 4, (res_end - res_start));
       printk(KERN_INFO "PCIEUNI: mem_region 4 address %X \n",  res_start);
       m_pcieuni_dev_p->rw_off4= (res_end - res_start);
       m_pcieuni_dev_p->pcieuni_all_mems +=16;
    }
    else{
      m_pcieuni_dev_p->memmory_base4 = 0;
      printk(KERN_INFO "PCIEUNI: NO BASE4 address\n");
    }

    res_start  = pci_resource_start(dev, 5);
    res_end    = pci_resource_end(dev, 5);
    res_flag   = pci_resource_flags(dev, 5);
    m_pcieuni_dev_p->mem_base5       = res_start;
    m_pcieuni_dev_p->mem_base5_end   = res_end;
    m_pcieuni_dev_p->mem_base5_flag  = res_flag;
    if(res_start){
       m_pcieuni_dev_p->memmory_base5 = pci_iomap(dev, 5, (res_end - res_start));
       printk(KERN_INFO "PCIEUNI: mem_region 5 address %X \n",   res_start);
        m_pcieuni_dev_p->rw_off5= (res_end - res_start);
        m_pcieuni_dev_p->pcieuni_all_mems += 32;
    }
    else{
      m_pcieuni_dev_p->memmory_base5 = 0;
      printk(KERN_INFO "PCIEUNI: NO BASE5 address\n");
    }
    if(!m_pcieuni_dev_p->pcieuni_all_mems){
        printk(KERN_ALERT "PROBE ERROR NO BASE_MEMs\n");
    }

    /******GET BRD INFO******/
    tmp_info = pcieuni_get_brdinfo(m_pcieuni_dev_p);
    printk(KERN_ALERT "$$$$$$$$$$$$$PROBE  IS STARTUP BOARD %i\n", tmp_info);
    if(tmp_info){
        tmp_info = pcieuni_get_prjinfo(m_pcieuni_dev_p);
        printk(KERN_ALERT "$$$$$$$$$$$$$PROBE  NUMBER OF PRJs %i\n", tmp_info);
    }
    /*******PREPARE INTERRUPTS******/
    
    m_pcieuni_dev_p->irq_flag = IRQF_SHARED | IRQF_DISABLED;
    #ifdef CONFIG_PCI_MSI
    if (pci_enable_msi(dev) == 0) {
            m_pcieuni_dev_p->msi = 1;
            m_pcieuni_dev_p->irq_flag &= ~IRQF_SHARED;
            printk(KERN_ALERT "MSI ENABLED\n");
    } else {
            m_pcieuni_dev_p->msi = 0;
            printk(KERN_ALERT "MSI NOT SUPPORTED\n");
    }
    #endif
    m_pcieuni_dev_p->pci_dev_irq = dev->irq;
    m_pcieuni_dev_p->irq_mode = 0;
    
    /* Send uvents to udev, so it'll create /dev nodes */
    if(m_pcieuni_dev_p->dev_sts){
        m_pcieuni_dev_p->dev_sts   = 0;
        device_destroy(pcieuni_cdev_p->pcieuni_class,  MKDEV(pcieuni_cdev_p->PCIEUNI_MAJOR, 
                                                  pcieuni_cdev_p->PCIEUNI_MINOR + m_brdNum));
    }
    m_pcieuni_dev_p->dev_sts   = 1;
    sprintf(m_pcieuni_dev_p->name, "%ss%d", dev_name, tmp_slot_num);
    sprintf(prc_entr, "%ss%d", dev_name, tmp_slot_num);
    printk(KERN_INFO "PCIEUNI_PROBE:  CREAT DEVICE MAJOR %i MINOR %i F_NAME %s\n",
           pcieuni_cdev_p->PCIEUNI_MAJOR, (pcieuni_cdev_p->PCIEUNI_MINOR + m_brdNum), m_pcieuni_dev_p->name);
    device_create(pcieuni_cdev_p->pcieuni_class, NULL, 
                  MKDEV(pcieuni_cdev_p->PCIEUNI_MAJOR, pcieuni_cdev_p->PCIEUNI_MINOR + m_brdNum),
                  &m_pcieuni_dev_p->pcieuni_pci_dev->dev, m_pcieuni_dev_p->name);

    pcieuni_cdev_p->pcieuni_procdir                     = create_proc_entry(prc_entr, S_IFREG | S_IRUGO, 0);
    pcieuni_cdev_p->pcieuni_procdir->read_proc  = pcieuni_procinfo;
    pcieuni_cdev_p->pcieuni_procdir->data           = m_pcieuni_dev_p;
    
    pcieuni_cdev_p->pcieuniModuleNum ++;
    return 0;
}
EXPORT_SYMBOL(pcieuni_probe_exp);