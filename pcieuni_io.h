#ifndef _PCIEUNI_IO_H
#define _PCIEUNI_IO_H

#include <linux/types.h>
#include <linux/ioctl.h> /* needed for the _IOW etc stuff used later */

#define RW_D8        0x0
#define RW_D16      0x1
#define RW_D32      0x2
#define RW_DMA     0x3
#define RW_INFO    0x4
#define DMA_DATA_OFFSET             6 
#define DMA_DATA_OFFSET_BYTE  24
#define PCIEUNI_DMA_SYZE                    4096
#define PCIEUNI_DMA_MIN_SYZE           128

#define IOCTRL_R      0x00
#define IOCTRL_W     0x01
#define IOCTRL_ALL  0x02

#define BAR0 0
#define BAR1 1
#define BAR2 2
#define BAR3 3
#define BAR4 4
#define BAR5 5

/* generic register access */
struct device_rw  {
       u_int            offset_rw; /* offset in address                       */
       u_int            data_rw;   /* data to set or returned read data       */
       u_int            mode_rw;   /* mode of rw (RW_D8, RW_D16, RW_D32)      */
       u_int            barx_rw;   /* BARx (0, 1, 2, 3, 4, 5)                 */
       u_int            size_rw;   /* transfer size in bytes                  */             
       u_int            rsrvd_rw;  /* transfer size in bytes                  */
};
typedef struct device_rw device_rw;

struct device_ioctrl_data  {
        u_int    offset;
        u_int    data;
        u_int    cmd;
        u_int    reserved;
};
typedef struct device_ioctrl_data device_ioctrl_data;

struct device_ioctrl_dma  {
        u_int    dma_offset;    
        u_int    dma_size;
        u_int    dma_cmd;       // value to DMA Control register
        u_int    dma_pattern;   // DMA BAR num
        u_int    dma_reserved1; // DMA Control register offset (31:16) DMA Length register offset (15:0)
        u_int    dma_reserved2; // DMA Read/Write Source register offset (31:16) Destination register offset (15:0)
};
typedef struct device_ioctrl_dma device_ioctrl_dma;

struct device_ioctrl_time  {
        struct timeval   start_time;
        struct timeval   stop_time;
};
typedef struct device_ioctrl_time device_ioctrl_time;

/** Information about the start addresses of the bars in user space.
 *  @fixme Should this be a fixed size array so addressing with the bar number is 
 *  easier? What about a range check to avoid buffer overruns in that case?
 */
typedef struct _pcieuni_bar_start{
  loff_t addressBar0; /** Start address of bar 0*/
  loff_t addressBar1; /** Start address of bar 1*/
  loff_t addressBar2; /** Start address of bar 2*/
  loff_t addressBar3; /** Start address of bar 3*/
  loff_t addressBar4; /** Start address of bar 4*/
  loff_t addressBar5; /** Start address of bar 5*/
} pcieuni_bar_start;

/** Information about the bar sizes. It is retrieved via IOCTL.
 */
typedef struct _pcieuni_ioctl_bar_sizes{
  size_t sizeBar0; /** Size of bar 0*/
  loff_t sizeBar1; /** Size of bar 1*/
  loff_t sizeBar2; /** Size of bar 2*/
  loff_t sizeBar3; /** Size of bar 3*/
  loff_t sizeBar4; /** Size of bar 4*/
  loff_t sizeBar5; /** Size of bar 5*/
} pcieuni_ioctl_bar_sizes;

/* Use 'U' like pcieUni as magic number */
#define PCIEUNI_IOC                               'U'
/* relative to the new IOC we keep the same ioctls */
#define PCIEUNI_PHYSICAL_SLOT           _IOWR(PCIEUNI_IOC, 60, int)
#define PCIEUNI_DRIVER_VERSION          _IOWR(PCIEUNI_IOC, 61, int)
#define PCIEUNI_FIRMWARE_VERSION        _IOWR(PCIEUNI_IOC, 62, int)
#define PCIEUNI_GET_DMA_TIME            _IOWR(PCIEUNI_IOC, 70, int)
#define PCIEUNI_WRITE_DMA               _IOWR(PCIEUNI_IOC, 71, int)
#define PCIEUNI_READ_DMA                _IOWR(PCIEUNI_IOC, 72, int)
#define PCIEUNI_SET_IRQ                 _IOWR(PCIEUNI_IOC, 73, int)
#define PCIEUNI_IOC_MINNR  60
#define PCIEUNI_IOC_MAXNR 63
#define PCIEUNI_IOC_DMA_MINNR  70
#define PCIEUNI_IOC_DMA_MAXNR  74

#endif
