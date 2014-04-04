/* 
 * File:   pciedev_ufn.h
 * Author: petros
 *
 * Created on February 18, 2013, 9:29 AM
 */

#ifndef PCIEDEV_UFN_H
#define	PCIEDEV_UFN_H

#include <linux/version.h>
#include <linux/pci.h>
#include <linux/types.h>	/* size_t */
#include <linux/cdev.h>
#include <linux/interrupt.h>

#include "pciedev_io.h"

#ifndef PCIEDEV_NR_DEVS
#define PCIEDEV_NR_DEVS 15    /* pciedev0 through pciedev15 */
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

struct pciedev_brd_info {
    u32 PCIEDEV_BOARD_ID;
    u32 PCIEDEV_BOARD_VERSION;
    u32 PCIEDEV_BOARD_DATE ; 
    u32 PCIEDEV_HW_VERSION  ;
    u32 PCIEDEV_BOARD_RESERVED  ;
    u32 PCIEDEV_PROJ_NEXT  ;
};
typedef struct pciedev_brd_info pciedev_brd_info;

struct pciedev_prj_info {
    struct list_head prj_list;
    u32 PCIEDEV_PROJ_ID;
    u32 PCIEDEV_PROJ_VERSION;
    u32 PCIEDEV_PROJ_DATE ; 
    u32 PCIEDEV_PROJ_RESERVED  ;
    u32 PCIEDEV_PROJ_NEXT  ;
};
typedef struct pciedev_prj_info pciedev_prj_info;

struct pciedev_cdev ;
struct pciedev_dev {
   
    struct cdev         cdev;	  /* Char device structure      */
    struct mutex      dev_mut;            /* mutual exclusion semaphore */
    struct pci_dev    *pciedev_pci_dev;
    int                        dev_num;
    int                        brd_num;
    int                        dev_minor;
    int                        dev_sts;
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
    
    u32    mem_base[PCIEDEV_N_BARS];
    u32    mem_base_end[PCIEDEV_N_BARS];
    u32    mem_base_flag[PCIEDEV_N_BARS];
    loff_t  rw_off[PCIEDEV_N_BARS];
    int      dev_dma_64mask;
    int      pciedev_all_mems ;
    int      dev_payload_size;
    void __iomem * memmory_base[PCIEDEV_N_BARS];

    u8                         msi;
    int                         irq_flag;
    u16                       irq_mode;
    u8                         irq_line;
    u8                         irq_pin;
    u32                       pci_dev_irq;
    
    struct pciedev_cdev *parent_dev;
    void                          *dev_str;
    
    struct pciedev_brd_info brd_info_list;
    struct pciedev_prj_info  prj_info_list;
    int                                 startup_brd;
    int                                 startup_prj_num;
};
typedef struct pciedev_dev pciedev_dev;

struct pciedev_cdev {
    u16 UPCIEDEV_VER_MAJ;
    u16 UPCIEDEV_VER_MIN;
    u16 PCIEDEV_DRV_VER_MAJ;
    u16 PCIEDEV_DRV_VER_MIN;
    int   PCIEDEV_MAJOR ;     /* major by default */
    int   PCIEDEV_MINOR  ;    /* minor by default */

    pciedev_dev                   *pciedev_dev_m[PCIEDEV_NR_DEVS];
    struct class                    *pciedev_class;
    struct proc_dir_entry     *pciedev_procdir;
    int                                   pciedevModuleNum;
};
typedef struct pciedev_cdev pciedev_cdev;

int        pciedev_open_exp( struct inode *, struct file * );
int        pciedev_release_exp(struct inode *, struct file *);
ssize_t  pciedev_read_exp(struct file *, char __user *, size_t , loff_t *);
ssize_t  pciedev_write_exp(struct file *, const char __user *, size_t , loff_t *);
long     pciedev_ioctl_exp(struct file *, unsigned int* , unsigned long* , pciedev_cdev *);

int        pciedev_procinfo(char *, char **, off_t, int, int *,void *);
int        pciedev_set_drvdata(struct pciedev_dev *, void *);
void*    pciedev_get_drvdata(struct pciedev_dev *);
int        pciedev_get_brdnum(struct pci_dev *);
pciedev_dev*   pciedev_get_pciedata(struct pci_dev *);
void*    pciedev_get_baddress(int, struct pciedev_dev *);

int       pciedev_probe_exp(struct pci_dev *, const struct pci_device_id *,  struct file_operations *, pciedev_cdev **, char *, int * );
int       pciedev_remove_exp(struct pci_dev *dev, pciedev_cdev **, char *, int *);

int       pciedev_get_prjinfo(struct pciedev_dev *);
int       pciedev_fill_prj_info(struct pciedev_dev *, void *);
int       pciedev_get_brdinfo(struct pciedev_dev *);

#if LINUX_VERSION_CODE < 0x20613 // irq_handler_t has changed in 2.6.19
int pciedev_setup_interrupt(irqreturn_t (*pciedev_interrupt)(int , void *, struct pt_regs *), struct pciedev_dev *, char *);
#else
int pciedev_setup_interrupt(irqreturn_t (*pciedev_interrupt)(int , void *), struct pciedev_dev *, char *);
#endif

#endif	/* PCIEDEV_UFN_H */

