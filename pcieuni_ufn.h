/* 
 * File:   pcieuni_ufn.h
 * Author: petros
 *
 * Created on February 18, 2013, 9:29 AM
 */

#ifndef PCIEUNI_UFN_H
#define	PCIEUNI_UFN_H

#include <linux/version.h>
#include <linux/pci.h>
#include <linux/types.h>	/* size_t */
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/fs.h>	
#include <linux/proc_fs.h>


/**
 * @def PCIEUNI_DEBUG 
 *
 * This define should be defined in debug build only. It controls the expansion of the PDEBUG macro.
 */ 

/**
 * @def PDEBUG 
 *
 * In production build this macro will expand to nothing, but in debug build it will print debug messages
 * using printk() with KERN_INFO level.
 * 
 * @param ctx   String describing context where message is produced (usually PCI device name)
 * @param fmt   Debug message with format specifiers (like with printk())
 * @param args  Message arguments (like with printk())  
 */ 
#undef PDEBUG      
#ifdef PCIEUNI_DEBUG
#define PDEBUG(ctx, fmt, args...) printk( KERN_INFO "PCIEUNI(%s): " fmt, ctx, ## args)
#else
#define PDEBUG(ctx, fmt, args...) 
#endif

// Unit testing
#define TEST_RANDOM_EXIT(cnt, msg, ret)     \
{                                           \
    struct timeval currentTime;             \
    do_gettimeofday(&currentTime);          \
    if (currentTime.tv_usec % cnt == 0)     \
    {                                       \
        printk( KERN_ALERT msg);            \
        return ret;                         \
    }                                       \
}                                              
//#define PCIEUNI_TEST_MISSING_INTERRUPT
//#define PCIEUNI_TEST_DEVICE_DMA_BLOCKED
//#define PCIEUNI_TEST_BUFFER_ALLOCATION_FAILURE
//#define PCIEUNI_TEST_MDEV_ALLOC_FAILURE

#ifndef PCIEUNI_NR_DEVS
#define PCIEUNI_NR_DEVS 15    /* pcieuni0 through pcieuni15 */
#endif

#define ASCII_BOARD_MAGIC_NUM         0x424F5244 /*BORD*/
#define ASCII_PROJ_MAGIC_NUM             0x50524F4A /*PROJ*/
#define ASCII_BOARD_MAGIC_NUM_L      0x626F7264 /*bord*/

/*FPGA Standard Register Set*/
#define WORD_BOARD_MAGIC_NUM      0x00
#define WORD_BOARD_ID                       0x04
#define WORD_BOARD_VERSION            0x08
#define WORD_BOARD_DATE                  0x0C
#define WORD_BOARD_HW_VERSION     0x10
#define WORD_BOARD_RESET                0x14
#define WORD_BOARD_TO_PROJ            0x18

#define WORD_PROJ_MAGIC_NUM         0x00
#define WORD_PROJ_ID                          0x04
#define WORD_PROJ_VERSION               0x08
#define WORD_PROJ_DATE                    0x0C
#define WORD_PROJ_RESERVED            0x10
#define WORD_PROJ_RESET                   0x14
#define WORD_PROJ_NEXT                    0x18

#define WORD_PROJ_IRQ_ENABLE         0x00
#define WORD_PROJ_IRQ_MASK             0x04
#define WORD_PROJ_IRQ_FLAGS            0x08
#define WORD_PROJ_IRQ_CLR_FLAGSE  0x0C

/* FIXME: The pcie bar should be part of the device struct.
 * The information which BAR contains the DMA control registers
 * and whether DMA is used at all (and probably which mechanism)
 * should come from the firmware and be according to SHAPI.
 * As DMA is hard coded to bar 2 at the moment, we use a define
 * as intermediate solution.
 */
#define PCIEUNI_DMA_BAR 2

struct pcieuni_file_list {
    struct list_head node_file_list;
    struct file *filp;
    int      file_cnt;
};
typedef struct pcieuni_file_list pcieuni_file_list;

struct pcieuni_brd_info {
    u32 PCIEUNI_BOARD_ID;
    u32 PCIEUNI_BOARD_VERSION;
    u32 PCIEUNI_BOARD_DATE ; 
    u32 PCIEUNI_HW_VERSION  ;
    u32 PCIEUNI_BOARD_RESERVED  ;
    u32 PCIEUNI_PROJ_NEXT  ;
};
typedef struct pcieuni_brd_info pcieuni_brd_info;

struct pcieuni_prj_info {
    struct list_head prj_list;
    u32 PCIEUNI_PROJ_ID;
    u32 PCIEUNI_PROJ_VERSION;
    u32 PCIEUNI_PROJ_DATE ; 
    u32 PCIEUNI_PROJ_RESERVED  ;
    u32 PCIEUNI_PROJ_NEXT  ;
};
typedef struct pcieuni_prj_info pcieuni_prj_info;

struct pcieuni_cdev;
struct pcieuni_dev {
    char                name[64];       /**< Card name. */
    struct cdev         cdev;	  /* Char device structure      */
    struct mutex      dev_mut;            /* mutual exclusion semaphore */
    int                 binded;         /* is binded to device*/
    
    struct pci_dev    *pcieuni_pci_dev;
    int                        dev_num;
    int                        brd_num;
    int                        dev_minor;
    int                        dev_sts;
    int                 dev_file_ref;
    int                 null_dev;

    pcieuni_file_list dev_file_list;

    int                        slot_num;
    u16                      vendor_id;
    u16                      device_id;
    u16                      subvendor_id;
    u16                      subdevice_id;
    u16                      class_code;
    u8                        revision;
    u32                      devfn;
    u32                      busNumber;
    u32                      devNumber;
    u32                      funcNumber;
    int                        bus_func;
    
    u32    mem_base0;
    u32    mem_base0_end;
    u32    mem_base0_flag;
    u32    mem_base1;
    u32    mem_base1_end;
    u32    mem_base1_flag;
    u32    mem_base2;
    u32    mem_base2_end;
    u32    mem_base2_flag;
    u32    mem_base3;
    u32    mem_base3_end;
    u32    mem_base3_flag;
    u32    mem_base4;
    u32    mem_base4_end;
    u32    mem_base4_flag;
    u32    mem_base5;
    u32    mem_base5_end;
    u32    mem_base5_flag;
    loff_t  rw_off0;
    loff_t  rw_off1;
    loff_t  rw_off2;
    loff_t  rw_off3;
    loff_t  rw_off4;
    loff_t  rw_off5;
    int      dev_dma_64mask;
    int      pcieuni_all_mems ;
    int      dev_payload_size;
    void __iomem    *memmory_base0;
    void __iomem    *memmory_base1;
    void __iomem    *memmory_base2;
    void __iomem    *memmory_base3;
    void __iomem    *memmory_base4;
    void __iomem    *memmory_base5;
    
    u8                         msi;
    int                         irq_flag;
    u16                       irq_mode;
    u8                         irq_line;
    u8                         irq_pin;
    u32                       pci_dev_irq;
    
    struct pcieuni_cdev *parent_dev;
    void                          *dev_str;
    
    struct pcieuni_brd_info brd_info_list;
    struct pcieuni_prj_info prj_info_list;
    int                     startup_brd;
    int                     startup_prj_num;
    
};
typedef struct pcieuni_dev pcieuni_dev;

struct pcieuni_cdev {
    u16 GPCIEUNI_VER_MAJ;
    u16 GPCIEUNI_VER_MIN;
    u16 PCIEUNI_DRV_VER_MAJ;
    u16 PCIEUNI_DRV_VER_MIN;
    int   PCIEUNI_MAJOR ;     /* major by default */
    int   PCIEUNI_MINOR  ;    /* minor by default */

    pcieuni_dev                   *pcieuni_dev_m[PCIEUNI_NR_DEVS + 1];
    struct class                    *pcieuni_class;
    struct proc_dir_entry     *pcieuni_procdir;
    int                                   pcieuniModuleNum;
};
typedef struct pcieuni_cdev pcieuni_cdev;

int     pcieuni_init_module_exp(pcieuni_cdev **, struct file_operations *, char *);
void    pcieuni_cleanup_module_exp(pcieuni_cdev  **);

int     pcieuni_open_exp( struct inode *, struct file * );
int     pcieuni_release_exp(struct inode *, struct file *);
ssize_t pcieuni_read_exp(struct file *, char __user *, size_t , loff_t *);
ssize_t pcieuni_write_exp(struct file *, const char __user *, size_t , loff_t *);
long    pcieuni_ioctl_exp(struct file *, unsigned int* , unsigned long* , pcieuni_cdev *);

int     pcieuni_set_drvdata(pcieuni_dev *, void *);
void*   pcieuni_get_drvdata(pcieuni_dev *);
int     pcieuni_get_brdnum(struct pci_dev *);
pcieuni_dev* pcieuni_get_pciedata(struct pci_dev *);
void*   pcieuni_get_baddress(int, pcieuni_dev *);

int     pcieuni_probe_exp(struct pci_dev *, const struct pci_device_id *,  struct file_operations *, pcieuni_cdev *, char *, int * );
int     pcieuni_remove_exp(struct pci_dev *dev, pcieuni_cdev *, char *, int *);

int     pcieuni_get_prjinfo(pcieuni_dev *);
int     pcieuni_fill_prj_info(pcieuni_dev *, void *);
int     pcieuni_get_brdinfo(pcieuni_dev *);

int     pcieuni_register_write32(struct pcieuni_dev *dev, void* bar, u32 offset, u32 value, bool ensureFlush);

/** posix style read function without struct */
ssize_t  pcieuni_read_no_struct_exp(struct file *, char __user *, size_t , loff_t *);
/** posix style write function without struct */
ssize_t  pcieuni_write_no_struct_exp(struct file *, const char __user *, size_t , loff_t *);

int pcieuni_setup_interrupt(irqreturn_t (*pcieuni_interrupt)(int , void *), pcieuni_dev *, char *);

void register_gpcieuni_proc(int num, char * dfn, pcieuni_dev *p_upcie_dev, pcieuni_cdev *p_upcie_cdev);
void unregister_gpcieuni_proc(int num, char *dfn);
ssize_t pcieuni_procinfo(struct file *filp,char *buf,size_t count,loff_t *offp );
#if LINUX_VERSION_CODE < KERNEL_VERSION(5,6,0)
    static const struct file_operations gpcieuni_proc_fops = { 
        .read = pcieuni_procinfo,
    }; 
#else
    static const struct proc_ops gpcieuni_proc_fops = {
        .proc_read = pcieuni_procinfo,
    };
#endif   /* KERNEL_VERSION(5,6,0) */

#endif	/* PCIEUNI_UFN_H */

