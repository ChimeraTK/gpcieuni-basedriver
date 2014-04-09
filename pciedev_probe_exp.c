#include <linux/module.h>
#include <linux/fs.h>	
#include <linux/proc_fs.h>

#include "pciedev_ufn.h"

int    pciedev_probe_exp(struct pci_dev *dev, const struct pci_device_id *id, 
            struct file_operations *pciedev_fops, pciedev_cdev **pciedev_cdev_pp, char *dev_name, int * brd_num)
{
    
    dev_t devt              = 0;
    int    devno             = 0;
    int    i                      = 0;
    int    m_brdNum   = 0;
    pciedev_dev     *m_pciedev_dev_p;
    pciedev_cdev   *pciedev_cdev_p;
 
    int     err       = 0;
    int     result   = 0;
    int     tmp_info = 0;
    char  **endptr;
    
    u16 vendor_id;
    u16 device_id;
    u8  revision;
    u8  irq_line;
    u8  irq_pin;
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
    
    char f_name[64];
    char prc_entr[64];
    	
    printk(KERN_ALERT "############PCIEDEV_PROBE THIS IS U_FUNCTION NAME %s\n", dev_name);
    
   if(!( *pciedev_cdev_pp)){
        printk(KERN_ALERT "PCIEDEV_PROBE: CREATING CDEV_M\n");
        pciedev_cdev_p= kzalloc(sizeof(pciedev_cdev), GFP_KERNEL);
        if(!pciedev_cdev_p){
            printk(KERN_ALERT "AFTER_INIT CREATE CDEV STRUCT NO MEM\n");
            return -ENOMEM;
        }
         *pciedev_cdev_pp = pciedev_cdev_p;
        pciedev_cdev_p->PCIEDEV_MAJOR             = 47;
        pciedev_cdev_p->PCIEDEV_MINOR             = 0;
        pciedev_cdev_p->pciedevModuleNum         = 0;
        pciedev_cdev_p->PCIEDEV_DRV_VER_MAJ = 1;
        pciedev_cdev_p->PCIEDEV_DRV_VER_MIN = 1;
        pciedev_cdev_p->UPCIEDEV_VER_MAJ        = 1;
        pciedev_cdev_p->UPCIEDEV_VER_MIN        = 1;
        
        result = alloc_chrdev_region(&devt, pciedev_cdev_p->PCIEDEV_MINOR, PCIEDEV_NR_DEVS, dev_name);
        pciedev_cdev_p->PCIEDEV_MAJOR = MAJOR(devt);
        printk(KERN_ALERT "PCIEDEV_PROBE: DRV_MAJOR is %d\n", pciedev_cdev_p->PCIEDEV_MAJOR);
        /* Populate sysfs entries */
        pciedev_cdev_p->pciedev_class = class_create(pciedev_fops->owner, dev_name);
        /*Get module driver version information*/
        printk(KERN_ALERT "&&&&&PCIEDEV_PROBE CALLED; UPCIEDEV MODULE VERSION %s \n", THIS_MODULE->version);
        pciedev_cdev_p->UPCIEDEV_VER_MAJ = simple_strtol(THIS_MODULE->version, endptr, 10);
        pciedev_cdev_p->UPCIEDEV_VER_MIN = simple_strtol(THIS_MODULE->version + 2, endptr, 10);
        printk(KERN_ALERT "&&&&&PCIEDEV_PROBE CALLED; UPCIEDEV MODULE VERSION %i.%i\n", 
                pciedev_cdev_p->UPCIEDEV_VER_MAJ, pciedev_cdev_p->UPCIEDEV_VER_MIN);
        
        printk(KERN_ALERT "&&&&&PCIEDEV_PROBE CALLED; THIS MODULE VERSION %s \n", pciedev_fops->owner->version);
        pciedev_cdev_p->PCIEDEV_DRV_VER_MAJ = simple_strtol(pciedev_fops->owner->version, endptr, 10);
        pciedev_cdev_p->PCIEDEV_DRV_VER_MIN = simple_strtol(pciedev_fops->owner->version + 2, endptr, 10);
        printk(KERN_ALERT "&&&&&PCIEDEV_PROBE CALLED; THIS MODULE VERSION %i.%i\n", 
                pciedev_cdev_p->PCIEDEV_DRV_VER_MAJ, pciedev_cdev_p->PCIEDEV_DRV_VER_MIN);
    }else{
        printk(KERN_ALERT "PCIEDEV_PROBE: CDEV_M EXIST\n");
        pciedev_cdev_p     = *pciedev_cdev_pp;
    }
    
    /*************************CDEV INIT******************************************************/
    printk(KERN_WARNING "PCIEDEVINIT_MODULE CALLED\n");
    
    /*setup device*/
    for(m_brdNum = 0;m_brdNum < PCIEDEV_NR_DEVS;m_brdNum++){
        if(!pciedev_cdev_p->pciedev_dev_m[m_brdNum])
        break;
    }
    if(m_brdNum == PCIEDEV_NR_DEVS){
        printk(KERN_ALERT "AFTER_INIT NO MORE DEVICES is %d\n", m_brdNum);
        return -1;
    }
    m_pciedev_dev_p = kzalloc(sizeof(pciedev_dev), GFP_KERNEL);
    if(!m_pciedev_dev_p){
        printk(KERN_ALERT "AFTER_INIT CREATE DEV STRUCT NO MEM\n");
        return -ENOMEM;
    }
    printk(KERN_ALERT "PCIEDEV_PROBE_DEV_INIT: BOARD NUM %i\n", m_brdNum);
    * brd_num = m_brdNum;
    pciedev_cdev_p->pciedev_dev_m[m_brdNum]= m_pciedev_dev_p;
    m_pciedev_dev_p->parent_dev = pciedev_cdev_p;
    devno = MKDEV(pciedev_cdev_p->PCIEDEV_MAJOR, pciedev_cdev_p->PCIEDEV_MINOR + m_brdNum);
    cdev_init(&(m_pciedev_dev_p->cdev), pciedev_fops);
    m_pciedev_dev_p->cdev.owner = THIS_MODULE;
    m_pciedev_dev_p->cdev.ops = pciedev_fops;
    result = cdev_add(&(m_pciedev_dev_p->cdev), devno, 1);
    if (result){
      printk(KERN_NOTICE "Error %d adding devno%d num%d\n", result, devno, i);
      return 1;
    }
    //INIT_LIST_HEAD(&(m_pciedev_dev_p->brd_info_list));
    INIT_LIST_HEAD(&(m_pciedev_dev_p->prj_info_list.prj_list));
    mutex_init(&(m_pciedev_dev_p->dev_mut));
    m_pciedev_dev_p->dev_sts                  = 0;
    m_pciedev_dev_p->irq_mode               = 0;
    m_pciedev_dev_p->msi                        = 0;
    m_pciedev_dev_p->dev_dma_64mask  = 0;
    m_pciedev_dev_p->pciedev_all_mems = 0;
    m_pciedev_dev_p->brd_num                = m_brdNum;
    m_pciedev_dev_p->dev_minor             = pciedev_cdev_p->PCIEDEV_MINOR + m_brdNum;
    
    
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
    printk(KERN_ALERT "PCIEDEV_PROBE:DEVFN %X, BUS_NUM %X, DEV_NUM %X, FUNC_NUM %X, BUS_FUNC %x\n",
                                      tmp_devfn, busNumber, devNumber, funcNumber, tmp_bus_func);

    tmp_devfn  = (u32)dev->bus->self->devfn;
    busNumber  = (u32)dev->bus->self->bus->number;
    devNumber  = (u32)PCI_SLOT(tmp_devfn);
    funcNumber = (u32)PCI_FUNC(tmp_devfn);
    printk(KERN_ALERT "PCIEDEV_PROBE:DEVFN %X, BUS_NUM %X, DEV_NUM %X, FUNC_NUM %X\n",
                                      tmp_devfn, busNumber, devNumber, funcNumber);
    
    pcie_cap = pci_find_capability (dev->bus->self, PCI_CAP_ID_EXP);
    printk(KERN_INFO "PCIEDEV_PROBE: PCIE SWITCH CAP address %X\n",pcie_cap);
    
    pci_read_config_dword(dev->bus->self, (pcie_cap +PCI_EXP_SLTCAP), &tmp_slot_cap);
    tmp_slot_num = (tmp_slot_cap >> 19);
    tmp_dev_num  = tmp_slot_num;
    printk(KERN_ALERT "PCIEDEV_PROBE:SLOT NUM %d DEV NUM%d SLOT_CAP %X\n",tmp_slot_num,tmp_dev_num,tmp_slot_cap);
    
    m_pciedev_dev_p->slot_num  = tmp_slot_num;
    m_pciedev_dev_p->dev_num   = tmp_dev_num;
    m_pciedev_dev_p->bus_func  = tmp_bus_func;
    m_pciedev_dev_p->pciedev_pci_dev = dev;

    dev_set_drvdata(&(dev->dev), m_pciedev_dev_p);
    
    pcie_cap = pci_find_capability (dev, PCI_CAP_ID_EXP);
    printk(KERN_ALERT "DAMC_PROBE: PCIE CAP address %X\n",pcie_cap);
    pci_read_config_byte(dev, (pcie_cap +PCI_EXP_DEVCAP), &dev_payload);
    dev_payload &=0x0003;
    printk(KERN_ALERT "DAMC_PROBE: DEVICE CAP  %X\n",dev_payload);

    /* FIXME: How about
       tmp_payload_size = 128 << dev_payload;
    */
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
            m_pciedev_dev_p->dev_dma_64mask = 1;
            printk(KERN_ALERT "CURRENT 64MASK %i\n", cur_mask);
    } else {
            if ((err = pci_set_dma_mask(dev, DMA_BIT_MASK(32))) &&
                (err = pci_set_consistent_dma_mask(dev, DMA_BIT_MASK(32)))) {
                    printk(KERN_ALERT "No usable DMA configuration\n");
            }else{
            m_pciedev_dev_p->dev_dma_64mask = 0;
            printk(KERN_ALERT "CURRENT 32MASK %i\n", cur_mask);
            }
    }

    
    
    m_pciedev_dev_p->pciedev_all_mems = 0;
    
    
    pci_read_config_word(dev, PCI_VENDOR_ID,   &vendor_id);
    pci_read_config_word(dev, PCI_DEVICE_ID,   &device_id);
    pci_read_config_word(dev, PCI_SUBSYSTEM_VENDOR_ID,   &subvendor_id);
    pci_read_config_word(dev, PCI_SUBSYSTEM_ID,   &subdevice_id);
    pci_read_config_word(dev, PCI_CLASS_DEVICE,   &class_code);
    pci_read_config_byte(dev, PCI_REVISION_ID, &revision);
    pci_read_config_byte(dev, PCI_INTERRUPT_LINE, &irq_line);
    pci_read_config_byte(dev, PCI_INTERRUPT_PIN, &irq_pin);
    
    printk(KERN_INFO "PCIEDEV_PROBE: VENDOR_ID  %i\n",vendor_id);
    printk(KERN_INFO "PCIEDEV_PROBE: PCI_DEVICE_ID  %i\n",device_id);
    printk(KERN_INFO "PCIEDEV_PROBE: PCI_SUBSYSTEM_VENDOR_ID  %i\n",subvendor_id);
    printk(KERN_INFO "PCIEDEV_PROBE: PCI_SUBSYSTEM_ID  %i\n",subdevice_id);
    printk(KERN_INFO "PCIEDEV_PROBE: PCI_CLASS_DEVICE  %i\n",class_code);

    m_pciedev_dev_p->vendor_id      = vendor_id;
    m_pciedev_dev_p->device_id      = device_id;
    m_pciedev_dev_p->subvendor_id   = subvendor_id;
    m_pciedev_dev_p->subdevice_id   = subdevice_id;
    m_pciedev_dev_p->class_code     = class_code;
    m_pciedev_dev_p->revision       = revision;
    m_pciedev_dev_p->irq_line       = irq_line;
    m_pciedev_dev_p->irq_pin        = irq_pin;
    
    /*******SETUP BARs******/
    { /* Trick: open a new scope so we don't pollute the rest of the code with a loop counter,
         plus the definition is closer to the place where it is used.*/
      unsigned int bar;

      for (bar = 0; bar < PCIEDEV_N_BARS; ++bar){

	if(pci_resource_start(dev, bar)){
	  m_pciedev_dev_p->memory_base[bar] = pci_iomap(dev, bar, pci_resource_len(dev, bar));
	  printk(KERN_INFO "PCIEDEV_PROBE: mem_region %u address %LX  SIZE %LX FLAGS %lX\n",
		 bar, pci_resource_start(dev, bar), pci_resource_len(dev, bar),
		 pci_resource_flags(dev, bar));

	  m_pciedev_dev_p->bar_length[bar] =  pci_resource_len(dev, bar);

	  /* FIXME: What is all_mems, and why to we add something like this?
	     Could it be a bit-field and what really was ment is
	     pciedev_all_mems |= (0x1 << bar);
	  */
	  switch(bar){
	  case 0: m_pciedev_dev_p->pciedev_all_mems = 1;
	    break;
	  case 1: m_pciedev_dev_p->pciedev_all_mems += 2;
	    break;
	  case 2: m_pciedev_dev_p->pciedev_all_mems += 4;
	    break;
	  case 3: m_pciedev_dev_p->pciedev_all_mems += 8;
	    break;
	  case 4: m_pciedev_dev_p->pciedev_all_mems += 16;
	    break;
	  case 5: m_pciedev_dev_p->pciedev_all_mems += 32;
	    break;
	    /* no reasonable default statement possible, and all cases covered */
	  }/* switch (bar) */

	}// if(res_start)
	else{
	  m_pciedev_dev_p->memory_base[bar] = 0;
	  m_pciedev_dev_p->bar_length[bar]       = 0;
	  printk(KERN_INFO "PCIEDEV: NO BASE%u address\n", bar);
	}

      }/* for (bar) */
    }/* scope of the bar counter */

    /******GET BOARD INFO******/
    tmp_info = pciedev_get_brdinfo(m_pciedev_dev_p);
    printk(KERN_ALERT "$$$$$$$$$$$$$PROBE  IS STARTUP BOARD %i\n", tmp_info);
    if(tmp_info){
        tmp_info = pciedev_get_prjinfo(m_pciedev_dev_p);
        printk(KERN_ALERT "$$$$$$$$$$$$$PROBE  NUMBER OF PRJs %i\n", tmp_info);
    }
    /*******PREPARE INTERRUPTS******/
    
    m_pciedev_dev_p->irq_flag = IRQF_SHARED | IRQF_DISABLED;
    #ifdef CONFIG_PCI_MSI
    if (pci_enable_msi(dev) == 0) {
            m_pciedev_dev_p->msi = 1;
            m_pciedev_dev_p->irq_flag &= ~IRQF_SHARED;
            printk(KERN_ALERT "MSI ENABLED\n");
    } else {
            m_pciedev_dev_p->msi = 0;
            printk(KERN_ALERT "MSI NOT SUPPORTED\n");
    }
    #endif
    m_pciedev_dev_p->pci_dev_irq = dev->irq;
    m_pciedev_dev_p->irq_mode = 0;
    
    /* Send uvents to udev, so it'll create /dev nodes */
    if(m_pciedev_dev_p->dev_sts){
        m_pciedev_dev_p->dev_sts   = 0;
        device_destroy(pciedev_cdev_p->pciedev_class,  MKDEV(pciedev_cdev_p->PCIEDEV_MAJOR, 
                                                  pciedev_cdev_p->PCIEDEV_MINOR + m_brdNum));
    }
    m_pciedev_dev_p->dev_sts   = 1;
    sprintf(f_name, "%ss%d", dev_name, tmp_slot_num);
    sprintf(prc_entr, "%ss%d", dev_name, tmp_slot_num);
    printk(KERN_INFO "PCIEDEV_PROBE:  CREAT DEVICE MAJOR %i MINOR %i F_NAME %s\n",
               pciedev_cdev_p->PCIEDEV_MAJOR, (pciedev_cdev_p->PCIEDEV_MINOR + m_brdNum), f_name);
    device_create(pciedev_cdev_p->pciedev_class, NULL, MKDEV(pciedev_cdev_p->PCIEDEV_MAJOR, 
                                                      (pciedev_cdev_p->PCIEDEV_MINOR + m_brdNum)),
                   &m_pciedev_dev_p->pciedev_pci_dev->dev, f_name);

    pciedev_cdev_p->pciedev_procdir                     = create_proc_entry(prc_entr, S_IFREG | S_IRUGO, 0);
    pciedev_cdev_p->pciedev_procdir->read_proc  = pciedev_procinfo;
    pciedev_cdev_p->pciedev_procdir->data           = m_pciedev_dev_p;
    
    pciedev_cdev_p->pciedevModuleNum ++;
    return 0;
}
EXPORT_SYMBOL(pciedev_probe_exp);
