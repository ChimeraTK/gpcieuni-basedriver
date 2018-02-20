#include <linux/module.h>
#include <linux/fs.h>	
#include <linux/uaccess.h>

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
    printk("PCIEUNI_TRANSFER_INFO_CHECK: Invalid bar number %d\n", transferInformation->bar);
    return -EFAULT;
  }
  
  if (!transferInformation->barStart){
    printk("BAR %d not implemented in this device",  transferInformation->bar);
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
    checkAndCalculateTransferInformation( deviceData, count, *f_pos, &transferInformation);
  
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
    u32 outputWord;
    for( nBytesActuallyTransferred = 0; nBytesActuallyTransferred < transferInformation.nBytesToTransfer;
	 nBytesActuallyTransferred += sizeof(u32) ){
      
      /* bar start is a 32 bit pointer. It increased by number of words, not bytes */
      outputWord = ioread32(transferInformation.barStart + (transferInformation.offset + nBytesActuallyTransferred)/sizeof(u32) );

     // buf is a char buffer, so we add the size in bytes
      if (copy_to_user(buf + nBytesActuallyTransferred, &outputWord, sizeof(u32)) ){
	/* copying to user space failed. report back the number of actually read bytes.
	 */
	mutex_unlock(&deviceData->dev_mut);
	*f_pos += nBytesActuallyTransferred;
	return nBytesActuallyTransferred;
      }
      rmb();
    }
  }/* end of local variable space */
  
  mutex_unlock(&deviceData->dev_mut);
  *f_pos += nBytesActuallyTransferred;
  return nBytesActuallyTransferred;
}
EXPORT_SYMBOL(pcieuni_read_no_struct_exp);

ssize_t pcieuni_write_no_struct_exp(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
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
    checkAndCalculateTransferInformation( deviceData, count, *f_pos, &transferInformation);
  
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
