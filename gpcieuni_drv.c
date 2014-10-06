#include <linux/module.h>

#include "pcieuni_ufn.h"
#include "pcieuni_io.h"

MODULE_AUTHOR("Ludwig Petrosyan");
MODULE_DESCRIPTION("DESY AMC-PCIE board driver");
MODULE_VERSION("2.2.0");
MODULE_LICENSE("Dual BSD/GPL");


static void __exit pcieuni_cleanup_module(void)
{
    printk(KERN_NOTICE "GPCIEUNI_CLEANUP_MODULE CALLED\n");
}

static int __init pcieuni_init_module(void)
{
    int   result  = 0;
    
    printk(KERN_ALERT "GPCIEUNI_INIT:REGISTERING PCI DRIVER\n");
    return result; /* succeed */
}

module_init(pcieuni_init_module);
module_exit(pcieuni_cleanup_module);


