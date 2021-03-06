#include <kernel.h>
#include <pci.h>
#include <gic.h>
#include <drivers/console.h>

//#define PCI_PLATFORM_DEBUG
#ifdef PCI_PLATFORM_DEBUG
#define pci_plat_debug rprintf
#else
#define pci_plat_debug(...) do { } while(0)
#endif

#define PIO_DATA (dev_base_pointer(PCIE_PIO))
#define pio_in8(port) (*(u8*)(PIO_DATA + port))
#define pio_in16(port) (*(u16*)(PIO_DATA + port))
#define pio_in32(port) (*(u32*)(PIO_DATA + port))
#define pio_out8(port, source) (*(u8*)(PIO_DATA + port) = (source))
#define pio_out16(port, source) (*(u16*)(PIO_DATA + port) = (source))
#define pio_out32(port, source) (*(u32*)(PIO_DATA + port) = (source))

/* stub ... really shouldn't be hardwired into console.c */
void vga_pci_register(kernel_heaps kh, console_attach a)
{
}

/* ECAM */
u32 pci_cfgread(pci_dev dev, int reg, int bytes)
{
    u32 data = -1;
    void *base = dev_base_pointer(PCIE_ECAM)
        + (dev->bus << 20) + (dev->slot << 15) + (dev->function << 12) + reg;
    pci_plat_debug("%s: dev %p, bus %d, reg %d, bytes %d, base 0x%lx\n", __func__,
              dev, dev->bus, reg, bytes, base);
    switch (bytes) {
    case 1:
        data = *(u8*)base;
        break;
    case 2:
        data = *(u16*)base;
        break;
    case 4:
        data = *(u32*)base;
        break;
    }
    pci_plat_debug("...data = 0x%x\n", data);
    return data;
}

void pci_cfgwrite(pci_dev dev, int reg, int bytes, u32 source)
{
    void *base = dev_base_pointer(PCIE_ECAM)
        + (dev->bus << 20) + (dev->slot << 15) + (dev->function << 12) + reg;
    pci_plat_debug("%s: dev %p, bus %d, reg %d, bytes %d, base 0x%lx, source 0x%x\n", __func__,
              dev, dev->bus, reg, bytes, base, source);
    switch (bytes) {
    case 1:
        *(u8*)base = source;
        break;
    case 2:
        *(u16*)base = source;
        break;
    case 4:
        *(u32*)base = source;
        break;
    }
    pci_plat_debug("...done\n");
}

u8 pci_bar_read_1(struct pci_bar *b, u64 offset)
{
    return b->type == PCI_BAR_MEMORY ? *(u8 *) (b->vaddr + offset) : pio_in8(b->addr + offset);
}

void pci_bar_write_1(struct pci_bar *b, u64 offset, u8 val)
{
    if (b->type == PCI_BAR_MEMORY)
        *(u8 *) (b->vaddr + offset) = val;
    else
        pio_out8(b->addr + offset, val);
}

u16 pci_bar_read_2(struct pci_bar *b, u64 offset)
{
    return b->type == PCI_BAR_MEMORY ? *(u16 *) (b->vaddr + offset) : pio_in16(b->addr + offset);
}

void pci_bar_write_2(struct pci_bar *b, u64 offset, u16 val)
{
    if (b->type == PCI_BAR_MEMORY)
        *(u16 *) (b->vaddr + offset) = val;
    else
        pio_out16(b->addr + offset, val);
}

u32 pci_bar_read_4(struct pci_bar *b, u64 offset)
{
    return b->type == PCI_BAR_MEMORY ? *(u32 *) (b->vaddr + offset) : pio_in32(b->addr + offset);
}

void pci_bar_write_4(struct pci_bar *b, u64 offset, u32 val)
{
    if (b->type == PCI_BAR_MEMORY)
        *(u32 *) (b->vaddr + offset) = val;
    else
        pio_out32(b->addr + offset, val);
}

void pci_setup_non_msi_irq(pci_dev dev, int idx, thunk h, const char *name)
{
    /* queue index ignored; virtio ints are shared */
    u64 v = GIC_SPI_INTS_START + VIRT_PCIE_IRQ_BASE + (dev->slot % 4);
    pci_plat_debug("%s: dev %p, idx %d, irq %d, handler %F, name %s\n",
                   __func__, dev, idx, v, h, name);
    reserve_interrupt(v); /* failure ok if shared int */
    register_interrupt(v, h, name);
}

void pci_platform_init_bar(pci_dev dev)
{
    /* XXX consts */
    u64 base = 0x1000 + ((dev->bus << 12) | (dev->slot << 8) | (dev->function << 6));
    pci_plat_debug("%s: dev %d:%d:%d, base 0x%x\n", __func__, dev->bus, dev->slot, dev->function, base);
    pci_cfgwrite(dev, PCIR_BAR(0), 4, base);
}
