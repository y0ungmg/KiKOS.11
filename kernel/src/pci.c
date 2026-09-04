#include "pci.h"
#include "lib.h"
#include "com1.h"

pci_dev_t pci_devices[256];
int pci_device_count = 0;

static uint32_t pci_read_config(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg) {
    uint32_t addr = (1U << 31) | (bus << 16) | (dev << 11) | (func << 8) | (reg & 0xFC);
    outl(PCI_CONFIG_ADDRESS, addr);
    return inl(PCI_CONFIG_DATA);
}

static void pci_write_config(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg, uint32_t val) {
    uint32_t addr = (1U << 31) | (bus << 16) | (dev << 11) | (func << 8) | (reg & 0xFC);
    outl(PCI_CONFIG_ADDRESS, addr);
    outl(PCI_CONFIG_DATA, val);
}

void pci_init(void) {
    pci_device_count = 0;
    dbg("[PCI] Scanning bus...\n");

    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t dev = 0; dev < 32; dev++) {
            for (uint8_t func = 0; func < 8; func++) {
                uint32_t vendor_device = pci_read_config(bus, dev, func, 0);
                uint16_t vendor = vendor_device & 0xFFFF;
                uint16_t device = (vendor_device >> 16) & 0xFFFF;

                if (vendor == 0xFFFF) continue;

                pci_dev_t *p = &pci_devices[pci_device_count++];
                p->bus = bus;
                p->device = dev;
                p->function = func;

                p->config.vendor_id = vendor;
                p->config.device_id = device;
                p->config.command = pci_read_config(bus, dev, func, 4);
                p->config.status = pci_read_config(bus, dev, func, 6);
                p->config.revision_id = pci_read_config(bus, dev, func, 8) & 0xFF;
                p->config.prog_if = (pci_read_config(bus, dev, func, 8) >> 8) & 0xFF;
                p->config.subclass = (pci_read_config(bus, dev, func, 8) >> 16) & 0xFF;
                p->config.class_code = (pci_read_config(bus, dev, func, 8) >> 24) & 0xFF;
                p->config.cache_line_size = pci_read_config(bus, dev, func, 12) & 0xFF;
                p->config.latency_timer = (pci_read_config(bus, dev, func, 12) >> 8) & 0xFF;
                p->config.header_type = (pci_read_config(bus, dev, func, 12) >> 16) & 0xFF;
                p->config.bist = (pci_read_config(bus, dev, func, 12) >> 24) & 0xFF;

                for (int i = 0; i < 6; i++) {
                    p->config.bar[i] = pci_read_config(bus, dev, func, 16 + i * 4);
                }

                p->config.cardbus_cis = pci_read_config(bus, dev, func, 40);
                uint32_t subsys = pci_read_config(bus, dev, func, 44);
                p->config.subsys_vendor_id = subsys & 0xFFFF;
                p->config.subsys_device_id = (subsys >> 16) & 0xFFFF;
                p->config.expansion_rom = pci_read_config(bus, dev, func, 48);
                uint32_t cap = pci_read_config(bus, dev, func, 52);
                p->config.cap_ptr = cap & 0xFF;
                p->config.int_line = (pci_read_config(bus, dev, func, 60) >> 0) & 0xFF;
                p->config.int_pin = (pci_read_config(bus, dev, func, 60) >> 8) & 0xFF;
                p->config.min_gnt = (pci_read_config(bus, dev, func, 60) >> 16) & 0xFF;
                p->config.max_lat = (pci_read_config(bus, dev, func, 60) >> 24) & 0xFF;

                char buf[64];
                utoa_dec(p->config.vendor_id, buf);
                dbg("[PCI] Bus "); utoa_dec(bus, buf); dbg(buf);
                dbg(" Dev "); utoa_dec(dev, buf); dbg(buf);
                dbg(" Func "); utoa_dec(func, buf); dbg(buf);
                dbg(" Vendor "); utoa_dec(vendor, buf); dbg(buf);
                dbg(" Device "); utoa_dec(device, buf); dbg(buf);
                dbg(" Class "); utoa_dec(p->config.class_code, buf); dbg(buf);
                dbg(" Subclass "); utoa_dec(p->config.subclass, buf); dbg(buf);
                dbg(" ProgIF "); utoa_dec(p->config.prog_if, buf); dbg(buf);
                dbg("\n");

                if (func == 0 && (p->config.header_type & 0x80) == 0) break;
            }
        }
    }
    dbg("[PCI] Found ");
    utoa_dec(pci_device_count, (char[16]){0});
    dbg((char[16]){0});
    dbg(" devices\n");
}

uint32_t pci_config_read(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg) {
    return pci_read_config(bus, dev, func, reg);
}

void pci_config_write(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg, uint32_t val) {
    pci_write_config(bus, dev, func, reg, val);
}

pci_dev_t *pci_find_device(uint16_t vendor, uint16_t device) {
    for (int i = 0; i < pci_device_count; i++) {
        if (pci_devices[i].config.vendor_id == vendor && pci_devices[i].config.device_id == device) {
            return &pci_devices[i];
        }
    }
    return 0;
}

pci_dev_t *pci_find_class(uint8_t class_code, uint8_t subclass) {
    for (int i = 0; i < pci_device_count; i++) {
        if (pci_devices[i].config.class_code == class_code && pci_devices[i].config.subclass == subclass) {
            return &pci_devices[i];
        }
    }
    return 0;
}

void pci_enable_bus_master(pci_dev_t *dev) {
    if (!dev) return;
    uint32_t cmd = dev->config.command;
    cmd |= (1 << 2);
    pci_write_config(dev->bus, dev->device, dev->function, 4, cmd);
    dev->config.command = cmd;
}

void pci_enable_memory(pci_dev_t *dev) {
    if (!dev) return;
    uint32_t cmd = dev->config.command;
    cmd |= (1 << 1);
    pci_write_config(dev->bus, dev->device, dev->function, 4, cmd);
    dev->config.command = cmd;
}

void pci_enable_io(pci_dev_t *dev) {
    if (!dev) return;
    uint32_t cmd = dev->config.command;
    cmd |= 1;
    pci_write_config(dev->bus, dev->device, dev->function, 4, cmd);
    dev->config.command = cmd;
}

uint32_t pci_get_bar(pci_dev_t *dev, int bar) {
    if (!dev || bar < 0 || bar >= 6) return 0;
    return dev->config.bar[bar];
}

void pci_set_bar(pci_dev_t *dev, int bar, uint32_t addr) {
    if (!dev || bar < 0 || bar >= 6) return;
    pci_write_config(dev->bus, dev->device, dev->function, 16 + bar * 4, addr);
    dev->config.bar[bar] = addr;
}

void pci_enable_msi(pci_dev_t *dev) {
    if (!dev || !dev->config.cap_ptr) return;
    uint8_t cap = dev->config.cap_ptr;
    while (cap) {
        uint8_t cap_id = pci_read_config(dev->bus, dev->device, dev->function, cap) & 0xFF;
        if (cap_id == 0x05) {
            uint32_t msg_ctrl = pci_read_config(dev->bus, dev->device, dev->function, cap + 2);
            msg_ctrl |= 1;
            pci_write_config(dev->bus, dev->device, dev->function, cap + 2, msg_ctrl);
            break;
        }
        cap = (pci_read_config(dev->bus, dev->device, dev->function, cap + 1) >> 8) & 0xFF;
    }
}