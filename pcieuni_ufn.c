#include "pcieuni_ufn.h"

#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/stat.h>
#include <linux/uaccess.h>

int pcieuni_init_module_exp(pcieuni_cdev** pcieuni_cdev_pp, struct file_operations* pcieuni_fops, char* dev_name) {
  int i = 0;
  int k = 0;
  int result = 0;
  int devno = 0;
  dev_t devt = 0;
  char** endptr = 0;
  pcieuni_cdev* pcieuni_cdev_p;

  pcieuni_cdev_p = kzalloc(sizeof(pcieuni_cdev), GFP_KERNEL);
  if(!pcieuni_cdev_p) {
    printk(KERN_ALERT "gpcieuni(%s): no memory to create CDEV structure\n", dev_name);
    return -ENOMEM;
  }

  *pcieuni_cdev_pp = pcieuni_cdev_p;
  pcieuni_cdev_p->PCIEUNI_MAJOR = 47;
  pcieuni_cdev_p->PCIEUNI_MINOR = 0;
  pcieuni_cdev_p->pcieuniModuleNum = 0;
  pcieuni_cdev_p->PCIEUNI_DRV_VER_MAJ = 1;
  pcieuni_cdev_p->PCIEUNI_DRV_VER_MIN = 1;
  pcieuni_cdev_p->GPCIEUNI_VER_MAJ = 1;
  pcieuni_cdev_p->GPCIEUNI_VER_MIN = 1;

  result = alloc_chrdev_region(&devt, pcieuni_cdev_p->PCIEUNI_MINOR, (PCIEUNI_NR_DEVS + 1), dev_name);
  pcieuni_cdev_p->PCIEUNI_MAJOR = MAJOR(devt);

  /* Populate sysfs entries */
/* see linux kernel commit 1aaba11da9aa7d7d6b52a74d45b31cac118295a1 */
#if LINUX_VERSION_CODE > KERNEL_VERSION(6, 3, 0)
  pcieuni_cdev_p->pcieuni_class = class_create(dev_name);
#else
  pcieuni_cdev_p->pcieuni_class = class_create(pcieuni_fops->owner, dev_name);
#endif

  /*Get module driver version information*/
  pcieuni_cdev_p->GPCIEUNI_VER_MAJ = simple_strtol(THIS_MODULE->version, endptr, 10);
  pcieuni_cdev_p->GPCIEUNI_VER_MIN = simple_strtol(THIS_MODULE->version + 2, endptr, 10);

  pcieuni_cdev_p->PCIEUNI_DRV_VER_MAJ = simple_strtol(pcieuni_fops->owner->version, endptr, 10);
  pcieuni_cdev_p->PCIEUNI_DRV_VER_MIN = simple_strtol(pcieuni_fops->owner->version + 2, endptr, 10);

  printk(KERN_INFO "gpcieuni base version %i.%i, driver version %i.%i\n", pcieuni_cdev_p->GPCIEUNI_VER_MAJ,
      pcieuni_cdev_p->GPCIEUNI_VER_MIN, pcieuni_cdev_p->PCIEUNI_DRV_VER_MAJ, pcieuni_cdev_p->PCIEUNI_DRV_VER_MIN);

  for(i = 0; i <= PCIEUNI_NR_DEVS; i++) {
    pcieuni_cdev_p->pcieuni_dev_m[i] = kzalloc(sizeof(pcieuni_dev), GFP_KERNEL);
    if(!pcieuni_cdev_p->pcieuni_dev_m[i]) {
      printk(KERN_ERR "gpcieuni(%s): no memory to create device structure\n", dev_name);

      for(k = 0; k < i; k++) {
        if(pcieuni_cdev_p->pcieuni_dev_m[k]) {
          kfree(pcieuni_cdev_p->pcieuni_dev_m[k]);
        }
      }

      if(pcieuni_cdev_p) {
        kfree(pcieuni_cdev_p);
      }

      return -ENOMEM;
    }

    pcieuni_cdev_p->pcieuni_dev_m[i]->parent_dev = pcieuni_cdev_p;
    devno = MKDEV(pcieuni_cdev_p->PCIEUNI_MAJOR, pcieuni_cdev_p->PCIEUNI_MINOR + i);

    pcieuni_cdev_p->pcieuni_dev_m[i]->dev_num = devno;
    pcieuni_cdev_p->pcieuni_dev_m[i]->dev_minor = (pcieuni_cdev_p->PCIEUNI_MINOR + i);

    cdev_init(&(pcieuni_cdev_p->pcieuni_dev_m[i]->cdev), pcieuni_fops);
    pcieuni_cdev_p->pcieuni_dev_m[i]->cdev.owner = THIS_MODULE;
    pcieuni_cdev_p->pcieuni_dev_m[i]->cdev.ops = pcieuni_fops;

    /* The device is now live and the device file should be available */
    result = cdev_add(&(pcieuni_cdev_p->pcieuni_dev_m[i]->cdev), devno, 1);
    if(result) {
      printk(KERN_WARNING "gpcieuni(%s): error %d adding devno%d num%d\n", dev_name, result, devno, i);
      return 1;
    }

    INIT_LIST_HEAD(&(pcieuni_cdev_p->pcieuni_dev_m[i]->prj_info_list.prj_list));
    INIT_LIST_HEAD(&(pcieuni_cdev_p->pcieuni_dev_m[i]->dev_file_list.node_file_list));
    mutex_init(&(pcieuni_cdev_p->pcieuni_dev_m[i]->dev_mut));
    pcieuni_cdev_p->pcieuni_dev_m[i]->dev_sts = 0;
    pcieuni_cdev_p->pcieuni_dev_m[i]->dev_file_ref = 0;
    pcieuni_cdev_p->pcieuni_dev_m[i]->irq_mode = 0;
    pcieuni_cdev_p->pcieuni_dev_m[i]->msi = 0;
    pcieuni_cdev_p->pcieuni_dev_m[i]->dev_dma_64mask = 0;
    pcieuni_cdev_p->pcieuni_dev_m[i]->pcieuni_all_mems = 0;
    pcieuni_cdev_p->pcieuni_dev_m[i]->brd_num = i;
    pcieuni_cdev_p->pcieuni_dev_m[i]->binded = 0;
    pcieuni_cdev_p->pcieuni_dev_m[i]->dev_file_list.file_cnt = 0;
    pcieuni_cdev_p->pcieuni_dev_m[i]->null_dev = 0;

    if(i == PCIEUNI_NR_DEVS) {
      pcieuni_cdev_p->pcieuni_dev_m[i]->binded = 1;
      pcieuni_cdev_p->pcieuni_dev_m[i]->null_dev = 1;
    }
  }

  return result; /* succeed */
}
EXPORT_SYMBOL(pcieuni_init_module_exp);

void pcieuni_cleanup_module_exp(pcieuni_cdev** pcieuni_cdev_p) {
  int k = 0;
  dev_t devno;
  pcieuni_cdev* pcieuni_cdev_m = *pcieuni_cdev_p;

  devno = MKDEV(pcieuni_cdev_m->PCIEUNI_MAJOR, pcieuni_cdev_m->PCIEUNI_MINOR);
  unregister_chrdev_region(devno, (PCIEUNI_NR_DEVS + 1));
  class_destroy(pcieuni_cdev_m->pcieuni_class);
  for(k = 0; k <= PCIEUNI_NR_DEVS; k++) {
    if(pcieuni_cdev_m->pcieuni_dev_m[k]) {
      kfree(pcieuni_cdev_m->pcieuni_dev_m[k]);
      pcieuni_cdev_m->pcieuni_dev_m[k] = 0;
    }
  }
  kfree(*pcieuni_cdev_p);
  pcieuni_cdev_m = 0;
  *pcieuni_cdev_p = 0;
}
EXPORT_SYMBOL(pcieuni_cleanup_module_exp);

int pcieuni_open_exp(struct inode* inode, struct file* filp) {
  int minor;
  pcieuni_dev* dev;
  pcieuni_file_list* tmp_file_list;

  minor = iminor(inode);
  dev = container_of(inode->i_cdev, struct pcieuni_dev, cdev);
  dev->dev_minor = minor;

  if(mutex_lock_interruptible(&dev->dev_mut)) {
    return -ERESTARTSYS;
  }
  dev->dev_file_ref++;
  filp->private_data = dev;

  tmp_file_list = kzalloc(sizeof(pcieuni_file_list), GFP_KERNEL);
  INIT_LIST_HEAD(&tmp_file_list->node_file_list);
  tmp_file_list->file_cnt = dev->dev_file_ref;
  tmp_file_list->filp = filp;
  list_add(&(tmp_file_list->node_file_list), &(dev->dev_file_list.node_file_list));

  mutex_unlock(&dev->dev_mut);
  return 0;
}
EXPORT_SYMBOL(pcieuni_open_exp);

int pcieuni_release_exp(struct inode* inode, struct file* filp) {
  struct pcieuni_dev* dev = filp->private_data;
  u16 cur_proc = 0;
  pcieuni_file_list* tmp_file_list;
  struct list_head *pos, *q;

  if(mutex_lock_interruptible(&dev->dev_mut)) {
    return -ERESTARTSYS;
  }
  dev->dev_file_ref--;
  cur_proc = current->group_leader->pid;

  list_for_each_safe(pos, q, &(dev->dev_file_list.node_file_list)) {
    tmp_file_list = list_entry(pos, pcieuni_file_list, node_file_list);
    if(tmp_file_list->filp == filp) {
      list_del(pos);
      kfree(tmp_file_list);
    }
  }

  mutex_unlock(&dev->dev_mut);
  return 0;
}
EXPORT_SYMBOL(pcieuni_release_exp);

int pcieuni_set_drvdata(struct pcieuni_dev* dev, void* data) {
  if(!dev) return 1;
  dev->dev_str = data;
  return 0;
}
EXPORT_SYMBOL(pcieuni_set_drvdata);

void* pcieuni_get_drvdata(struct pcieuni_dev* dev) {
  if(dev && dev->dev_str) return dev->dev_str;
  return NULL;
}
EXPORT_SYMBOL(pcieuni_get_drvdata);

int pcieuni_get_brdnum(struct pci_dev* dev) {
  int m_brdNum;
  pcieuni_dev* pcieunidev;
  pcieunidev = dev_get_drvdata(&(dev->dev));
  m_brdNum = pcieunidev->brd_num;
  return m_brdNum;
}
EXPORT_SYMBOL(pcieuni_get_brdnum);

pcieuni_dev* pcieuni_get_pciedata(struct pci_dev* dev) {
  pcieuni_dev* pcieunidev;
  pcieunidev = dev_get_drvdata(&(dev->dev));
  return pcieunidev;
}
EXPORT_SYMBOL(pcieuni_get_pciedata);

void* pcieuni_get_baddress(int br_num, struct pcieuni_dev* dev) {
  void* tmp_address;

  tmp_address = 0;
  switch(br_num) {
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

int pcieuni_setup_interrupt(irqreturn_t (*pcieuni_interrupt)(int, void*), struct pcieuni_dev* pdev, char* dev_name) {
  int result = 0;

  /*******SETUP INTERRUPTS******/
  pdev->irq_mode = 1;
  result = request_irq(pdev->pci_dev_irq, pcieuni_interrupt, pdev->irq_flag, dev_name, pdev);
  printk(KERN_INFO "gpcieuni(%s): assigned IRQ %i RESULT %i\n", dev_name, pdev->pci_dev_irq, result);
  if(result) {
    printk(KERN_WARNING "gpcieuni(%s): can't get assigned irq %i\n", dev_name, pdev->pci_dev_irq);
    pdev->irq_mode = 0;
  }
  return result;
}
EXPORT_SYMBOL(pcieuni_setup_interrupt);

int pcieuni_get_brdinfo(struct pcieuni_dev* bdev) {
  void* baddress;
  void* address;
  int strbrd = 0;
  u32 tmp_data_32;

  bdev->startup_brd = 0;
  if(bdev->memmory_base0) {
    baddress = bdev->memmory_base0;
    address = baddress;
    tmp_data_32 = ioread32(address);
    if(tmp_data_32 == ASCII_BOARD_MAGIC_NUM || tmp_data_32 == ASCII_BOARD_MAGIC_NUM_L) {
      bdev->startup_brd = 1;
      address = baddress + WORD_BOARD_ID;
      tmp_data_32 = ioread32(address);
      bdev->brd_info_list.PCIEUNI_BOARD_ID = tmp_data_32;

      address = baddress + WORD_BOARD_VERSION;
      tmp_data_32 = ioread32(address);
      bdev->brd_info_list.PCIEUNI_BOARD_VERSION = tmp_data_32;

      address = baddress + WORD_BOARD_DATE;
      tmp_data_32 = ioread32(address);
      bdev->brd_info_list.PCIEUNI_BOARD_DATE = tmp_data_32;

      address = baddress + WORD_BOARD_HW_VERSION;
      tmp_data_32 = ioread32(address);
      bdev->brd_info_list.PCIEUNI_HW_VERSION = tmp_data_32;

      bdev->brd_info_list.PCIEUNI_PROJ_NEXT = 0;
      address = baddress + WORD_BOARD_TO_PROJ;
      tmp_data_32 = ioread32(address);
      bdev->brd_info_list.PCIEUNI_PROJ_NEXT = tmp_data_32;
    }
  }

  strbrd = bdev->startup_brd;

  return strbrd;
}
EXPORT_SYMBOL(pcieuni_get_brdinfo);

int pcieuni_fill_prj_info(struct pcieuni_dev* bdev, void* baddress) {
  void* address;
  int strbrd = 0;
  u32 tmp_data_32;
  struct pcieuni_prj_info* tmp_prj_info_list;

  address = baddress;
  tmp_data_32 = ioread32(address);
  if(tmp_data_32 == ASCII_PROJ_MAGIC_NUM) {
    bdev->startup_prj_num++;
    tmp_prj_info_list = kzalloc(sizeof(pcieuni_prj_info), GFP_KERNEL);

    address = baddress + WORD_PROJ_ID;
    tmp_data_32 = ioread32(address);
    tmp_prj_info_list->PCIEUNI_PROJ_ID = tmp_data_32;

    address = baddress + WORD_PROJ_VERSION;
    tmp_data_32 = ioread32(address);
    tmp_prj_info_list->PCIEUNI_PROJ_VERSION = tmp_data_32;

    address = baddress + WORD_PROJ_DATE;
    tmp_data_32 = ioread32(address);
    tmp_prj_info_list->PCIEUNI_PROJ_DATE = tmp_data_32;

    address = baddress + WORD_PROJ_RESERVED;
    tmp_data_32 = ioread32(address);
    tmp_prj_info_list->PCIEUNI_PROJ_RESERVED = tmp_data_32;

    bdev->brd_info_list.PCIEUNI_PROJ_NEXT = 0;
    address = baddress + WORD_PROJ_NEXT;
    tmp_data_32 = ioread32(address);
    tmp_prj_info_list->PCIEUNI_PROJ_NEXT = tmp_data_32;

    list_add(&(tmp_prj_info_list->prj_list), &(bdev->prj_info_list.prj_list));
    strbrd = tmp_data_32;
  }

  return strbrd;
}
EXPORT_SYMBOL(pcieuni_fill_prj_info);

int pcieuni_get_prjinfo(struct pcieuni_dev* bdev) {
  void* baddress;
  void* address;
  int strbrd = 0;
  int tmp_next_prj = 0;
  int tmp_next_prj1 = 0;

  bdev->startup_prj_num = 0;
  tmp_next_prj = bdev->brd_info_list.PCIEUNI_PROJ_NEXT;
  if(tmp_next_prj) {
    baddress = bdev->memmory_base0;
    while(tmp_next_prj) {
      address = baddress + tmp_next_prj;
      tmp_next_prj = pcieuni_fill_prj_info(bdev, address);
    }
  }
  else {
    if(bdev->memmory_base1) {
      tmp_next_prj = 1;
      tmp_next_prj1 = 0;
      baddress = bdev->memmory_base1;
      while(tmp_next_prj) {
        tmp_next_prj = tmp_next_prj1;
        address = baddress + tmp_next_prj;
        tmp_next_prj = pcieuni_fill_prj_info(bdev, address);
      }
    }
    if(bdev->memmory_base2) {
      tmp_next_prj = 1;
      tmp_next_prj1 = 0;
      baddress = bdev->memmory_base2;
      while(tmp_next_prj) {
        tmp_next_prj = tmp_next_prj1;
        address = baddress + tmp_next_prj;
        tmp_next_prj = pcieuni_fill_prj_info(bdev, address);
      }
    }
    if(bdev->memmory_base3) {
      tmp_next_prj = 1;
      tmp_next_prj1 = 0;
      baddress = bdev->memmory_base3;
      while(tmp_next_prj) {
        tmp_next_prj = tmp_next_prj1;
        address = baddress + tmp_next_prj;
        tmp_next_prj = pcieuni_fill_prj_info(bdev, address);
      }
    }
    if(bdev->memmory_base4) {
      tmp_next_prj = 1;
      tmp_next_prj1 = 0;
      baddress = bdev->memmory_base4;
      while(tmp_next_prj) {
        tmp_next_prj = tmp_next_prj1;
        address = baddress + tmp_next_prj;
        tmp_next_prj = pcieuni_fill_prj_info(bdev, address);
      }
    }
    if(bdev->memmory_base5) {
      tmp_next_prj = 1;
      tmp_next_prj1 = 0;
      baddress = bdev->memmory_base5;
      while(tmp_next_prj) {
        tmp_next_prj = tmp_next_prj1;
        address = baddress + tmp_next_prj;
        tmp_next_prj = pcieuni_fill_prj_info(bdev, address);
      }
    }
  }
  strbrd = bdev->startup_prj_num;
  return strbrd;
}
EXPORT_SYMBOL(pcieuni_get_prjinfo);

void register_gpcieuni_proc(int num, char* dfn, struct pcieuni_dev* p_upcie_dev, struct pcieuni_cdev* p_upcie_cdev) {
  char prc_entr[32];
  sprintf(prc_entr, "%ss%i", dfn, num);
  p_upcie_cdev->pcieuni_procdir = proc_create_data(prc_entr, S_IFREG | S_IRUGO, 0, &gpcieuni_proc_fops, p_upcie_dev);
}

void unregister_gpcieuni_proc(int num, char* dfn) {
  char prc_entr[32];
  sprintf(prc_entr, "%ss%i", dfn, num);
  remove_proc_entry(prc_entr, 0);
}

static int pcieuni_procinfo(struct seq_file* m, void* v) {
  pcieuni_dev* pcieuni_dev_m;
  struct list_head* pos;
  struct pcieuni_prj_info* tmp_prj_info_list;
  pcieuni_dev_m = m->private;

  seq_printf(m, "GPCIEUNI Driver Version:\t%i.%i\n", pcieuni_dev_m->parent_dev->GPCIEUNI_VER_MAJ,
      pcieuni_dev_m->parent_dev->GPCIEUNI_VER_MIN);
  seq_printf(m, "Driver Version:\t%i.%i\n", pcieuni_dev_m->parent_dev->PCIEUNI_DRV_VER_MAJ,
      pcieuni_dev_m->parent_dev->PCIEUNI_DRV_VER_MIN);
  seq_printf(m, "Board NUM:\t%i\n", pcieuni_dev_m->brd_num);
  seq_printf(m, "Slot NUM:\t%i\n", pcieuni_dev_m->slot_num);
  seq_printf(m, "Board ID:\t%X\n", pcieuni_dev_m->brd_info_list.PCIEUNI_BOARD_ID);
  seq_printf(m, "Board Version:\t%X\n", pcieuni_dev_m->brd_info_list.PCIEUNI_BOARD_VERSION);
  seq_printf(m, "Board Date:\t%X\n", pcieuni_dev_m->brd_info_list.PCIEUNI_BOARD_DATE);
  seq_printf(m, "Board HW Ver:\t%X\n", pcieuni_dev_m->brd_info_list.PCIEUNI_HW_VERSION);
  seq_printf(m, "Board Next Prj:\t%X\n", pcieuni_dev_m->brd_info_list.PCIEUNI_PROJ_NEXT);
  seq_printf(m, "Board Reserved:\t%X\n", pcieuni_dev_m->brd_info_list.PCIEUNI_BOARD_RESERVED);
  seq_printf(m, "Number of Proj:\t%i\n", pcieuni_dev_m->startup_prj_num);

  list_for_each(pos, &pcieuni_dev_m->prj_info_list.prj_list) {
    tmp_prj_info_list = list_entry(pos, struct pcieuni_prj_info, prj_list);
    seq_printf(m, "Project ID:\t%X\n", tmp_prj_info_list->PCIEUNI_PROJ_ID);
    seq_printf(m, "Project Version:\t%X\n", tmp_prj_info_list->PCIEUNI_PROJ_VERSION);
    seq_printf(m, "Project Date:\t%X\n", tmp_prj_info_list->PCIEUNI_PROJ_DATE);
    seq_printf(m, "Project Reserved:\t%X\n", tmp_prj_info_list->PCIEUNI_PROJ_RESERVED);
    seq_printf(m, "Project Next:\t%X\n", tmp_prj_info_list->PCIEUNI_PROJ_NEXT);
  }

  return 0;
}
int pcieuni_proc_open(struct inode* inode, struct file* file) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 17, 0)
  return single_open(file, pcieuni_procinfo, PDE_DATA(inode));
#else
  return single_open(file, pcieuni_procinfo, pde_data(inode));
#endif
}
EXPORT_SYMBOL(pcieuni_proc_open);

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
int pcieuni_register_write32(struct pcieuni_dev* dev, void* bar, u32 offset, u32 value, bool ensureFlush) {
  void* address = (void*)(bar + offset);
  u32 readbackData;

  // Write to device register
  iowrite32(value, address);

  if(ensureFlush) {
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
