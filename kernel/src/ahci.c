#include "ahci.h"
#include "pci.h"
#include "lib.h"
#include "timer.h"
#include "mm.h"
#include "paging.h"
#include "com1.h"

hba_mem_t *ahci_abar = 0;
int ahci_port_count = 0;
static uint8_t *ahci_cmd_list[AHCI_MAX_PORTS];
static uint8_t *ahci_rx_fis[AHCI_MAX_PORTS];
static uint8_t *ahci_cmd_tbl[AHCI_MAX_PORTS];

int ahci_init(pci_dev_t *dev) {
    if (!dev) return -1;

    if (dev->config.class_code != 0x01 || dev->config.subclass != 0x06 || dev->config.prog_if != 0x01) {
        return -1;
    }

    pci_enable_bus_master(dev);
    pci_enable_memory(dev);

    uint32_t bar5 = pci_get_bar(dev, 5);
    ahci_abar = (hba_mem_t *)(bar5 & 0xFFFFF000);

    uint32_t cap = ahci_abar->cap;
    uint32_t pi = ahci_abar->pi;

    dbg("[AHCI] CAP: "); utoa_dec(cap, (char[16]){0}); dbg((char[16]){0});
    dbg(" PI: "); utoa_dec(pi, (char[16]){0}); dbg((char[16]){0});
    dbg("\n");

    ahci_abar->ghc |= 1;

    for (int i = 0; i < 32; i++) {
        if (pi & (1 << i)) {
            if (ahci_port_init(i) == 0) {
                ahci_port_count++;
            }
        }
    }

    dbg("[AHCI] Initialized ");
    utoa_dec(ahci_port_count, (char[16]){0});
    dbg((char[16]){0});
    dbg(" ports\n");
    return 0;
}

int ahci_port_init(int port) {
    if (port >= 32) return -1;

    hba_port_t *p = &ahci_abar->ports[port];
    if (!(p->cmd & HBA_PxCMD_ST)) {
        p->cmd &= ~HBA_PxCMD_ST;
        while (p->cmd & HBA_PxCMD_CR);
    }

    p->cmd &= ~HBA_PxCMD_FRE;

    uint8_t *clb = (uint8_t *)kmalloc_page();
    if (!clb) return -1;
    memset(clb, 0, 4096);
    ahci_cmd_list[port] = clb;
    p->clb = (uint32_t)clb;
    p->clbu = 0;

    uint8_t *fb = (uint8_t *)kmalloc_page();
    if (!fb) return -1;
    memset(fb, 0, 256);
    ahci_rx_fis[port] = fb;
    p->fb = (uint32_t)fb;
    p->fbu = 0;

    uint8_t *ct = (uint8_t *)kmalloc_page();
    if (!ct) return -1;
    memset(ct, 0, 4096);
    ahci_cmd_tbl[port] = ct;

    hba_cmd_header_t *hdr = (hba_cmd_header_t *)clb;
    for (int i = 0; i < AHCI_MAX_CMDS; i++) {
        hdr[i].prdtl = 8;
        uint8_t *tbl = ct + i * 256;
        hdr[i].ctba = (uint32_t)tbl;
        hdr[i].ctbau = 0;
    }

    p->is = 0xFFFFFFFF;
    p->ie = 0xFFFFFFFF;

    p->cmd |= HBA_PxCMD_FRE | HBA_PxCMD_ST;

    dbg("[AHCI] Port "); utoa_dec(port, (char[16]){0}); dbg((char[16]){0});
    dbg(" initialized\n");
    return 0;
}

static int ahci_issue_cmd(int port, int slot, uint8_t cmd, uint32_t lba, uint32_t count, void *buf, int write) {
    (void)lba; (void)count;
    hba_port_t *p = &ahci_abar->ports[port];
    if (p->cmd & HBA_PxCMD_CR) return -1;

    hba_cmd_header_t *hdr = (hba_cmd_header_t *)ahci_cmd_list[port];
    hdr += slot;
    memset(hdr, 0, sizeof(hba_cmd_header_t));
    hdr->cfl = 5;
    hdr->w = write ? 1 : 0;
    hdr->prdtl = 1;

    hba_cmd_tbl_t *tbl = (hba_cmd_tbl_t *)ahci_cmd_tbl[port];
    hba_cmd_tbl_t *ct = &tbl[slot];
    memset(ct, 0, sizeof(hba_cmd_tbl_t));

    uint8_t *cfis = (uint8_t *)ct + 0x40;
    memset(cfis, 0, 64);

    uint8_t *acmd = (uint8_t *)ct + 0x80;
    acmd[0] = cmd;
    acmd[1] = 0;
    acmd[2] = 0;
    acmd[3] = 0;
    acmd[4] = 0;
    acmd[5] = 0;
    acmd[6] = 0;
    acmd[7] = 0;
    acmd[8] = 0;
    acmd[9] = 0;
    acmd[10] = 0;
    acmd[11] = 0;
    acmd[12] = 0;
    acmd[13] = 0;
    acmd[15] = 0;

    hba_prdt_t *prdt = (hba_prdt_t *)((uint8_t *)ct + 0x100);
    prdt[0].dba = (uint32_t)((uintptr_t)buf & 0xFFFFFFFF);
    prdt[0].dbau = 0;
    prdt[0].rsv0 = 0;
    prdt[0].dbc = 512 - 1;

    p->ci = 1 << slot;

    int timeout = 10000;
    while (timeout--) {
        if (!(p->ci & (1 << slot))) break;
        udelay(1000);
    }

    if (p->is & HBA_PxIS_TFES) return -1;
    return 0;
}

int ahci_identify(int port, void *buf) {
    return ahci_issue_cmd(port, 0, ATA_CMD_IDENTIFY, 0, 1, buf, 0);
}

int ahci_read(int port, uint32_t start, uint32_t count, void *buf) {
    if (port >= 32) return -1;
    return ahci_issue_cmd(port, 0, ATA_CMD_READ_DMA_EXT, start, count, buf, 0);
}

int ahci_write(int port, uint32_t start, uint32_t count, const void *buf) {
    if (port >= 32) return -1;
    return ahci_issue_cmd(port, 0, ATA_CMD_WRITE_DMA_EXT, start, count, (void *)buf, 1);
}

int ahci_flush(int port) {
    if (port >= 32) return -1;
    return ahci_issue_cmd(port, 0, ATA_CMD_FLUSH_CACHE_EXT, 0, 0, 0, 0);
}