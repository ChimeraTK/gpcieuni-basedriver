#include <linux/module.h>
#include <linux/fs.h>	
#include <asm/uaccess.h>

#include "pcieuni_ufn.h"
#include "pcieuni_io.h"

/* macro do create the variable names like memory_base0 and memory_base0_end from the bar number, incl.
   the case of the switch statement to save typing. */
#define DEFINE_BAR_START_AND_SIZE( BAR )\
  case BAR:\
    transferInformation->barStart = deviceData->memmory_base  ## BAR ;\
    transferInformation->barSizeInBytes = deviceData->mem_base ## BAR ## _end - deviceData->mem_base ## BAR;\
    break;

/** A struct which contains information about the transfer which is to be performed.
 *  Used to get data out of a common calculate and check function for read and write.
 *  Not all variables are needed later, but they are kept in case they are needed in future
 *  because they have to be calculated anyway for the checks.
 */
typedef struct _transfer_information{
  unsigned int bar;
  unsigned long offset; /* offset inside the bar */
  unsigned long barSizeInBytes;
  u32 * barStart, * barEnd;
  unsigned int nBytesToTransfer;
} transfer_information;

/** Checks the input and calculates the transfer information.
 *  Returns a negative value in case of error, 0 upon success.
 */
int checkAndCalculateTransferInformation( pcieuni_dev const * deviceData,
					  size_t count, loff_t virtualOffset,
					  transfer_information * transferInformation ){

  /* check that the device is there (what the f* is sts?)*/
  if(!deviceData->dev_sts){
    printk("PCIEUNI_WRITE_EXP: NO DEVICE %d\n", deviceData->dev_num);
    return -EFAULT;
  }

  /* check the input data. Only 32 bit reads are supported */
  if ( virtualOffset%4 ){
    printk("%s\n", "Incorrect position, has the be a multiple of 4");
    return -EFAULT;
  }
  if ( count%4 ){
    printk("%s\n", "Incorrect size, has the be a multiple of 4");
    return -EFAULT;
  }

  /*  printk("gpcieuni::checkAndCalculateTransferInformation: count %zx , virtualOffsets %Lx\n", count, virtualOffset );
   */

  /* Before locking the mutex check if the request is valid (do not write after the end of the bar). */
  /* Do not access the registers, only check the pointer values without locking the mutex! */
  
  /* determine the bar from the f_pos */
  transferInformation->bar = (virtualOffset >> 60) & 0x7;
  /* mask out the bar position from the offset */
  transferInformation->offset = virtualOffset & 0x0FFFFFFFFFFFFFFFL;
  
  /*  printk("gpcieuni::checkAndCalculateTransferInformation: bar %x, offset %lx\n",
	    transferInformation->bar,
	    transferInformation->offset);  
  */

  /* get the bar's start and end address */
  /* FIXME: organise the information as arrays, not as individual variables, and you might get rid of this block */
  switch (transferInformation->bar){
    DEFINE_BAR_START_AND_SIZE( 0 );
    DEFINE_BAR_START_AND_SIZE( 1 );
    DEFINE_BAR_START_AND_SIZE( 2 );
    DEFINE_BAR_START_AND_SIZE( 3 );
    DEFINE_BAR_START_AND_SIZE( 4 );
    DEFINE_BAR_START_AND_SIZE( 5 );

  default:
    printk("PCIEUNI_WRITE_NO_STRUCT_EXP: Invalid bar number %d\n", transferInformation->bar);
    return -EFAULT;
  }
  /* When adding to a pointer, the + operator expects number of items, not the size in bytes */
  transferInformation->barEnd = transferInformation->barStart + 
    transferInformation->barSizeInBytes/sizeof(u32);

  /*
  printk("gpcieuni::checkAndCalculateTransferInformation: barStart %p, barSize %lx, barEnd %p\n",
	    transferInformation->barStart, transferInformation->barSizeInBytes,
	    transferInformation->barEnd);  
  */

  /* check that writing does not start after the end of the bar */
  if ( transferInformation->offset > transferInformation->barSizeInBytes ){
    printk("%s\n", "Cannot start writing after the end of the bar.");
    return -EFAULT;
  }

  /* limit the number of transferred by to the end of the bar. */
  /* The second line is safe because we checked before that offset <= barSizeInBytes */
  transferInformation->nBytesToTransfer = 
    ( (transferInformation->barSizeInBytes < transferInformation->offset + count) ?
      transferInformation->barSizeInBytes - transferInformation->offset  :
      count);

  /*
  printk("gpcieuni::checkAndCalculateTransferInformation:  nBytesToTransfer %x, count %lx",
	 transferInformation->nBytesToTransfer, count);
  */

  return 0;
}

ssize_t pcieuni_read_no_struct_exp(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    u64        itemsize       = 0;
    ssize_t    retval         = 0;
    int        minor          = 0;
    int        d_num          = 0;
    int        tmp_offset     = 0;
    int        tmp_mode       = 0;
    int        tmp_barx       = 0;
    int        tmp_rsrvd_rw   = 0;
    int        tmp_size_rw    = 0;
    u8         tmp_revision   = 0;
    u32        mem_tmp        = 0;
    int        i              = 0;
    u8         tmp_data_8;
    u16        tmp_data_16;
    u32        tmp_data_32;
    device_rw  reading;
    void*       address;
    
    struct pcieuni_dev *dev = filp->private_data;
    minor = dev->dev_minor;
    d_num = dev->dev_num;
    
    if(!dev->dev_sts){
        printk("PCIEUNI_READ_EXP: NO DEVICE %d\n", dev->dev_num);
        retval = -EFAULT;
        return retval;
    }

    itemsize = sizeof(device_rw);

    if (mutex_lock_interruptible(&dev->dev_mut)){
                    return -ERESTARTSYS;
    }
    if (copy_from_user(&reading, buf, count)) {
            retval = -EFAULT;
            mutex_unlock(&dev->dev_mut);
            return retval;
    }
      
    tmp_mode     = reading.mode_rw;
    if(tmp_mode == RW_INFO){
        pci_read_config_byte(dev->pcieuni_pci_dev, PCI_REVISION_ID, &tmp_revision);
        reading.offset_rw = dev->parent_dev->PCIEUNI_DRV_VER_MIN;
        reading.data_rw   = dev->parent_dev->PCIEUNI_DRV_VER_MAJ;
        reading.mode_rw   = tmp_revision;
        reading.barx_rw   = dev->pcieuni_all_mems;
        reading.size_rw   = dev->slot_num; /*SLOT NUM*/
        retval            = itemsize;
        if (copy_to_user(buf, &reading, count)) {
             printk(KERN_ALERT "PCIEUNI_READ_EXP 3\n");
             retval = -EFAULT;
             mutex_unlock(&dev->dev_mut);
             retval = 0;
             return retval;
        }

        mutex_unlock(&dev->dev_mut);
        return retval;
    }
    tmp_offset   = reading.offset_rw;
    tmp_barx     = reading.barx_rw;
    tmp_rsrvd_rw = reading.rsrvd_rw;
    tmp_size_rw  = reading.size_rw;
    switch(tmp_barx){
	case 0:
	   if(!dev->memmory_base0){
	       retval = -EFAULT;
	       mutex_unlock(&dev->dev_mut);
	       return retval;
	   }
	   address = (void *) dev->memmory_base0;
	   mem_tmp = (dev->mem_base0_end -2);
	   break;
	case 1:
	   if(!dev->memmory_base1){
	       retval = -EFAULT;
	       mutex_unlock(&dev->dev_mut);
	       return retval;
	   }
	   address = (void *)dev->memmory_base1;
	   mem_tmp = (dev->mem_base1_end -2);
	   break;
	case 2:
	   if(!dev->memmory_base2){
	       retval = -EFAULT;
	       mutex_unlock(&dev->dev_mut);
	       return retval;
	    }
	   address = (void *)dev->memmory_base2;
	   mem_tmp = (dev->mem_base2_end -2);
	   break;
        case 3:
	   if(!dev->memmory_base3){
	       retval = -EFAULT;
	       mutex_unlock(&dev->dev_mut);
	       return retval;
	   }
	   address = (void *) dev->memmory_base3;
	   mem_tmp = (dev->mem_base3_end -2);
	   break;
	case 4:
	   if(!dev->memmory_base4){
	       retval = -EFAULT;
	       mutex_unlock(&dev->dev_mut);
	       return retval;
	   }
	   address = (void *)dev->memmory_base4;
	   mem_tmp = (dev->mem_base4_end -2);
	   break;
	case 5:
	   if(!dev->memmory_base5){
	       retval = -EFAULT;
	       mutex_unlock(&dev->dev_mut);
	       return retval;
	    }
	   address = (void *)dev->memmory_base5;
	   mem_tmp = (dev->mem_base5_end -2);
	   break;
	default:
	   if(!dev->memmory_base0){
	       retval = -EFAULT;
	       mutex_unlock(&dev->dev_mut);
	       return retval;
	   }
	   address = (void *)dev->memmory_base0;
	   mem_tmp = (dev->mem_base0_end -2);
	   break;
    }	

    if(tmp_size_rw < 2){
        if(tmp_offset > (mem_tmp -2) || (!address)){
              reading.data_rw   = 0;
              retval            = 0;
        }else{
              switch(tmp_mode){
                case RW_D8:
                    tmp_offset = (tmp_offset/sizeof(u8))*sizeof(u8);
                    tmp_data_8        = ioread8(address + tmp_offset);
                    rmb();
                    reading.data_rw   = tmp_data_8 & 0xFF;
                    retval = itemsize;
                    break;
                case RW_D16:
                    tmp_offset = (tmp_offset/sizeof(u16))*sizeof(u16);
                    tmp_data_16       = ioread16(address + tmp_offset);
                    rmb();
                    reading.data_rw   = tmp_data_16 & 0xFFFF;
                    retval = itemsize;
                    break;
                case RW_D32:
                    tmp_offset = (tmp_offset/sizeof(u32))*sizeof(u32);
                    tmp_data_32       = ioread32(address + tmp_offset);
                    rmb();
                    
                    reading.data_rw   = tmp_data_32 & 0xFFFFFFFF;
                    retval = itemsize;
                    break;
                default:
                    tmp_offset = (tmp_offset/sizeof(u16))*sizeof(u16);
                    tmp_data_16       = ioread16(address + tmp_offset);
                    rmb();
                    reading.data_rw   = tmp_data_16 & 0xFFFF;
                    retval = itemsize;
                    break;
              }
          }

        if (copy_to_user(buf, &reading, count)) {
             retval = -EFAULT;
             mutex_unlock(&dev->dev_mut);
             retval = 0;
             return retval;
        }
    }else{
          switch(tmp_mode){
            case RW_D8:
                if((tmp_offset + tmp_size_rw) > (mem_tmp -2) || (!address)){
                      reading.data_rw   = 0;
                      retval            = 0;
                }else{
                    for(i = 0; i< tmp_size_rw; i++){
                        tmp_data_8        = ioread8(address + tmp_offset + i);
                        rmb();
                        reading.data_rw   = tmp_data_8 & 0xFF;
                        retval = itemsize;
                        if (copy_to_user((buf + i), &reading, 1)) {
                             retval = -EFAULT;
                             mutex_unlock(&dev->dev_mut);
                             retval = 0;
                             return retval;
                        }
                    }
                }
                break;
            case RW_D16:
                if((tmp_offset + tmp_size_rw*2) > (mem_tmp -2) || (!address)){
                      reading.data_rw   = 0;
                      retval            = 0;
                }else{
                    for(i = 0; i< tmp_size_rw; i++){
                        tmp_data_16       = ioread16(address + tmp_offset);
                        rmb();
                        reading.data_rw   = tmp_data_16 & 0xFFFF;
                        retval = itemsize;
                        if (copy_to_user((buf + i*2), &reading, 2)) {
                             retval = -EFAULT;
                             mutex_unlock(&dev->dev_mut);
                             retval = 0;
                             return retval;
                        }
                    }
                }
                break;
            case RW_D32:
                if((tmp_offset + tmp_size_rw*4) > (mem_tmp -2) || (!address)){
                      printk(KERN_ALERT "NO READ SIZE MORE THAN MEM \n");
                      reading.data_rw   = 0;
                      retval            = 0;
                }else{
                    
                    for(i = 0; i< tmp_size_rw; i++){
                        tmp_data_32       = ioread32(address + tmp_offset + i*4);
                        rmb();
                        retval = itemsize;
                        if (copy_to_user((buf + i*4), &tmp_data_32, 4)) {
                             retval = -EFAULT;
                             mutex_unlock(&dev->dev_mut);
                             retval = 0;
                             return retval;
                        }
                    }
/*
                     //pDataBuf = (void *)__get_free_page(GFP_KERNEL | GFP_ATOMIC);
                     pDataBuf = (void *)__get_free_page(GFP_KERNEL );
                     ioread32_rep(address + tmp_offset ,  (void*)pDataBuf, tmp_size_rw);
                     //memcpy_fromio((void*)pDataBuf, (address + tmp_offset), tmp_size_rw*sizeof(int));
                     if (copy_to_user((void*)buf , (void*)pDataBuf, tmp_size_rw*sizeof(int))) {
                             retval = -EFAULT;
                             free_page((ulong)pDataBuf);
                             mutex_unlock(&dev->dev_mut);
                             retval = 0;
                             return retval;
                        }
                     free_page((ulong)pDataBuf);
*/
                     
                }
                break;
            default:
                if((tmp_offset + tmp_size_rw*2) > (mem_tmp -2) || (!address)){
                      reading.data_rw   = 0;
                      retval            = 0;
                }else{
                    for(i = 0; i< tmp_size_rw; i++){
                        tmp_data_16       = ioread16(address + tmp_offset);
                        rmb();
                        reading.data_rw   = tmp_data_16 & 0xFFFF;
                        retval = itemsize;
                        if (copy_to_user((buf + i*2), &reading, 2)) {
                             retval = -EFAULT;
                             mutex_unlock(&dev->dev_mut);
                             retval = 0;
                             return retval;
                        }
                    }
                }
                break;
          }
    }
    mutex_unlock(&dev->dev_mut);
    return retval;
}
EXPORT_SYMBOL(pcieuni_read_no_struct_exp);

ssize_t pcieuni_write_no_struct_exp(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
  /*    device_rw       reading;
    int             itemsize       = 0;
    ssize_t         retval         = 0;
    int             tmp_offset     = 0;
    int             tmp_mode       = 0;
    int             tmp_barx       = 0;
    u32             mem_tmp        = 0;
    int             tmp_rsrvd_rw   = 0;
    int             tmp_size_rw    = 0;
    u16             tmp_data_8;
    u16             tmp_data_16;
    u32             tmp_data_32;
    void            *address ;
  */
  struct pcieuni_dev *deviceData;
  transfer_information transferInformation;
  int transferInfoError;
  unsigned int nBytesActuallyTransferred;

  deviceData = filp->private_data;

  /* The checkAndCalculateTransferInformation is only accessing static data in the 
     deviceData struct. No need to hold the mutex.
     FIXME: Is this correct? What if the device goes offline in the mean time?
  */
  transferInfoError = 
    checkAndCalculateTransferInformation( deviceData,count, *f_pos, &transferInformation);
  
  if (transferInfoError){
    return transferInfoError;
  }

  /* now we really want to access, so we need the mutex */
  if (mutex_lock_interruptible(&deviceData->dev_mut)) {
    printk("mutex_lock_interruptible %s\n", "- locking attempt was interrupted by a signal");
    return -ERESTARTSYS;
  }

  /* first implementation: a stupid loop, but inside the kernel space to
     safe the many context changes if the loop was in user space */
  { /* keep the variables local */
    u32 inputWord;
    for( nBytesActuallyTransferred = 0; nBytesActuallyTransferred < transferInformation.nBytesToTransfer;
	 nBytesActuallyTransferred += sizeof(u32) ){
      
      // buf is a char buffer, so we add the size in bytes
      if (copy_from_user(&inputWord, buf + nBytesActuallyTransferred, sizeof(u32)) ){
	/* copying the user input failed. report back the number of actually written bytes.
	 */
	mutex_unlock(&deviceData->dev_mut);
	*f_pos += nBytesActuallyTransferred;
	return nBytesActuallyTransferred;
      }
      /* bar start is a 32 bit pointer. It increased by number of words, not bytes */
      iowrite32(inputWord, transferInformation.barStart + (transferInformation.offset + nBytesActuallyTransferred)/sizeof(u32) );
      /* fixme: the write barrier is here in the original code because there only is
	 one transfer. I think it could be moved after the end of the loop. */
      wmb();
    }
  }/* end of local variable space */

  mutex_unlock(&deviceData->dev_mut);
  *f_pos += nBytesActuallyTransferred;
  return nBytesActuallyTransferred;
}
EXPORT_SYMBOL(pcieuni_write_no_struct_exp);
