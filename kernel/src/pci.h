#pragma once
#include "types.h"

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

#define PCI_MAX_BUS 256
#define PCI_MAX_DEV 32
#define PCI_MAX_FUNC 8

typedef struct {
    uint16_t vendor_id;
    uint16_t device_id;
    uint16_t command;
    uint16_t status;
    uint8_t revision_id;
    uint8_t prog_if;
    uint8_t subclass;
    uint8_t class_code;
    uint8_t cache_line_size;
    uint8_t latency_timer;
    uint8_t header_type;
    uint8_t bist;
    uint32_t bar[6];
    uint32_t cardbus_cis;
    uint16_t subsys_vendor_id;
    uint16_t subsys_device_id;
    uint32_t expansion_rom;
    uint8_t cap_ptr;
    uint8_t reserved[3];
    uint32_t reserved2;
    uint8_t int_line;
    uint8_t int_pin;
    uint8_t min_gnt;
    uint8_t max_lat;
} pci_device_t;

typedef struct {
    uint8_t bus;
    uint8_t device;
    uint8_t function;
    pci_device_t config;
} pci_dev_t;

#define PCI_CLASS_UNCLASSIFIED       0x00
#define PCI_CLASS_MASS_STORAGE       0x01
#define PCI_CLASS_NETWORK            0x02
#define PCI_CLASS_DISPLAY            0x03
#define PCI_CLASS_MULTIMEDIA         0x04
#define PCI_CLASS_MEMORY             0x05
#define PCI_CLASS_BRIDGE             0x06
#define PCI_CLASS_SIMPLE_COMM        0x07
#define PCI_CLASS_BASE_SYSTEM        0x08
#define PCI_CLASS_INPUT              0x09
#define PCI_CLASS_DOCKING            0x0A
#define PCI_CLASS_PROCESSOR          0x0B
#define PCI_CLASS_SERIAL_BUS         0x0C
#define PCI_CLASS_WIRELESS           0x0D
#define PCI_CLASS_INTELLIGENT        0x0E
#define PCI_CLASS_SATELLITE          0x0F
#define PCI_CLASS_ENCRYPTION         0x10
#define PCI_CLASS_SIGNAL_PROCESSING  0x11

#define PCI_SUBCLASS_SATA            0x06
#define PCI_SUBCLASS_ETHERNET        0x00
#define PCI_SUBCLASS_VGA             0x00
#define PCI_SUBCLASS_AUDIO           0x01
#define PCI_SUBCLASS_USB             0x03

#define PCI_PROG_IF_AHCI             0x01

extern pci_dev_t pci_devices[256];
extern int pci_device_count;

void pci_init(void);
uint32_t pci_config_read(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg);
void pci_config_write(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg, uint32_t val);
pci_dev_t *pci_find_device(uint16_t vendor, uint16_t device);
pci_dev_t *pci_find_class(uint8_t class_code, uint8_t subclass);
void pci_enable_bus_master(pci_dev_t *dev);
void pci_enable_memory(pci_dev_t *dev);
void pci_enable_io(pci_dev_t *dev);
uint32_t pci_get_bar(pci_dev_t *dev, int bar);
void pci_set_bar(pci_dev_t *dev, int bar, uint32_t addr);
void pci_enable_msi(pci_dev_t *dev);