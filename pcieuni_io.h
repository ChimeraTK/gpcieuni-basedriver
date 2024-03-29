#ifndef _PCIEUNI_IO_H
#define _PCIEUNI_IO_H

#include <linux/ioctl.h> /* needed for the _IOW etc stuff used later */
#include <linux/types.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 6, 0)
#  include <linux/time.h>
#endif

#define PCIEUNI_DMA_SYZE 4096

struct device_ioctrl_data {
  u_int offset;
  u_int data;
  u_int cmd;
  u_int reserved;
};
typedef struct device_ioctrl_data device_ioctrl_data;

struct device_ioctrl_dma {
  u_int dma_offset;
  u_int dma_size;
  u_int dma_cmd;       // value to DMA Control register
  u_int dma_pattern;   // DMA BAR num
  u_int dma_reserved1; // DMA Control register offset (31:16) DMA Length register offset (15:0)
  u_int dma_reserved2; // DMA Read/Write Source register offset (31:16) Destination register offset (15:0)
};
typedef struct device_ioctrl_dma device_ioctrl_dma;

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 6, 0)

typedef struct timeval pcieuni_timeval;

#else

struct pcieuni_timeval {
  long long tv_sec;
  long long tv_usec;
};

typedef struct pcieuni_timeval pcieuni_timeval;
#endif

struct device_ioctrl_time {
  pcieuni_timeval start_time;
  pcieuni_timeval stop_time;
};
typedef struct device_ioctrl_time device_ioctrl_time;

/** Information about the offsets of the bars in the address space of the character device.
 */
static const loff_t PCIEUNI_BAR_OFFSETS[6] = {0L, (1L) << 60, (2L) << 60, (3L) << 60, (4L) << 60, (5L) << 60};

/** Information about the bar sizes. It is retrieved via IOCTL.
 */
typedef struct _pcieuni_ioctl_bar_sizes {
  size_t barSizes[6]; /** Sizes of bar 0 to 5*/
  size_t dmaAreaSize; /** Size of the address range which can transferred via DMA.*/
} pcieuni_ioctl_bar_sizes;

/* Use 'U' like pcieUni as magic number */
#define PCIEUNI_IOC 'U'
/* relative to the new IOC we keep the same ioctls as upciedev*/
#define PCIEUNI_PHYSICAL_SLOT _IOWR(PCIEUNI_IOC, 60, int)
#define PCIEUNI_DRIVER_VERSION _IOWR(PCIEUNI_IOC, 61, int)
#define PCIEUNI_FIRMWARE_VERSION _IOWR(PCIEUNI_IOC, 62, int)
#define PCIEUNI_GET_DMA_TIME _IOWR(PCIEUNI_IOC, 70, int)
#define PCIEUNI_WRITE_DMA _IOWR(PCIEUNI_IOC, 71, int)
#define PCIEUNI_READ_DMA _IOWR(PCIEUNI_IOC, 72, int)
#define PCIEUNI_SET_IRQ _IOWR(PCIEUNI_IOC, 73, int)
#define PCIEUNI_IOC_MINNR 60
#define PCIEUNI_IOC_MAXNR 63
#define PCIEUNI_IOC_DMA_MINNR 70
#define PCIEUNI_IOC_DMA_MAXNR 74

#endif
