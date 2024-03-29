#include "pcieuni_io.h"
#include "pcieuni_ufn.h"
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/uaccess.h>

long pcieuni_ioctl_exp(struct file* filp, unsigned int* cmd_p, unsigned long* arg_p, pcieuni_cdev* pcieuni_cdev_m) {
  unsigned int cmd;
  unsigned long arg;
  pid_t cur_proc = 0;
  int minor = 0;
  int d_num = 0;
  int retval = 0;
  int err = 0;
  struct pci_dev* pdev;
  u8 tmp_revision = 0;
  u_int tmp_offset;
  u_int tmp_data;
  u_int tmp_cmd;
  u_int tmp_reserved;
  int io_size;
  device_ioctrl_data data;
  struct pcieuni_dev* dev = filp->private_data;

  cmd = *cmd_p;
  arg = *arg_p;
  io_size = sizeof(device_ioctrl_data);
  minor = dev->dev_minor;
  d_num = dev->dev_num;
  cur_proc = current->group_leader->pid;
  pdev = (dev->pcieuni_pci_dev);

  if(!dev->dev_sts) {
    printk(KERN_DEBUG "gpcieuni: ioctl called for non-existing device %d\n", dev->dev_num);
    retval = -EFAULT;
    return retval;
  }

  /*
   * the direction is a bitmask, and VERIFY_WRITE catches R/W
   * transfers. `Type' is user-oriented, while
   * access_ok is kernel-oriented, so the concept of "read" and
   * "write" is reversed
   */
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 0, 0)
  if(_IOC_DIR(cmd) & _IOC_READ)
    err = !access_ok(VERIFY_WRITE, (void __user*)arg, _IOC_SIZE(cmd));
  else if(_IOC_DIR(cmd) & _IOC_WRITE)
    err = !access_ok(VERIFY_READ, (void __user*)arg, _IOC_SIZE(cmd));
#else
  err = !access_ok((void __user*)arg, _IOC_SIZE(cmd));
#endif
  if(err) return -EFAULT;

  if(mutex_lock_interruptible(&dev->dev_mut)) return -ERESTARTSYS;

  switch(cmd) {
    case PCIEUNI_PHYSICAL_SLOT:
      retval = 0;
      if(copy_from_user(&data, (device_ioctrl_data*)arg, (size_t)io_size)) {
        retval = -EFAULT;
        mutex_unlock(&dev->dev_mut);
        return retval;
      }
      tmp_offset = data.offset;
      tmp_data = data.data;
      tmp_cmd = data.cmd;
      tmp_reserved = data.reserved;
      data.data = dev->slot_num;
      data.cmd = PCIEUNI_PHYSICAL_SLOT;
      if(copy_to_user((device_ioctrl_data*)arg, &data, (size_t)io_size)) {
        retval = -EFAULT;
        mutex_unlock(&dev->dev_mut);
        return retval;
      }
      break;
    case PCIEUNI_DRIVER_VERSION:
      data.data = pcieuni_cdev_m->PCIEUNI_DRV_VER_MAJ;
      data.offset = pcieuni_cdev_m->PCIEUNI_DRV_VER_MIN;
      if(copy_to_user((device_ioctrl_data*)arg, &data, (size_t)io_size)) {
        retval = -EFAULT;
        mutex_unlock(&dev->dev_mut);
        return retval;
      }
      break;
    case PCIEUNI_FIRMWARE_VERSION:
      pci_read_config_byte(dev->pcieuni_pci_dev, PCI_REVISION_ID, &tmp_revision);
      data.data = tmp_revision;
      data.offset = dev->revision;
      if(copy_to_user((device_ioctrl_data*)arg, &data, (size_t)io_size)) {
        retval = -EFAULT;
        mutex_unlock(&dev->dev_mut);
        return retval;
      }
      break;

    default:
      return -ENOTTY;
      break;
  }
  mutex_unlock(&dev->dev_mut);
  return retval;
}
EXPORT_SYMBOL(pcieuni_ioctl_exp);
