#pragma once
#include "types.h"
#include "pci.h"

#define AHCI_MAX_PORTS 32
#define AHCI_MAX_CMDS 32

#define HBA_PxCMD_CR    (1<<15)
#define HBA_PxCMD_FRE   (1<<4)
#define HBA_PxCMD_ST    (1<<0)
#define HBA_PxCMD_FR    (1<<14)

#define HBA_PxIS_TFES   (1<<30)
#define HBA_PxIS_HBFS   (1<<29)
#define HBA_PxIS_HBDS   (1<<28)
#define HBA_PxIS_IFS    (1<<27)
#define HBA_PxIS_INFS   (1<<26)
#define HBA_PxIS_OF     (1<<24)
#define HBA_PxIS_IPMS   (1<<23)
#define HBA_PxIS_PRCS   (1<<22)
#define HBA_PxIS_DMPS   (1<<7)
#define HBA_PxIS_PCS    (1<<6)
#define HBA_PxIS_DPS    (1<<5)
#define HBA_PxIS_UFS    (1<<4)
#define HBA_PxIS_SDBS   (1<<3)
#define HBA_PxIS_DSS    (1<<2)
#define HBA_PxIS_PSS    (1<<1)
#define HBA_PxIS_DHRS   (1<<0)

#define FIS_TYPE_REG_H2D    0x27
#define FIS_TYPE_REG_D2H    0x34
#define FIS_TYPE_DMA_ACT    0x39
#define FIS_TYPE_DMA_SETUP  0x41
#define FIS_TYPE_DATA       0x46
#define FIS_TYPE_BIST       0x58
#define FIS_TYPE_PIO_SETUP  0x5F
#define FIS_TYPE_DEV_BITS   0xA1

#define ATA_CMD_READ_DMA_EXT     0x25
#define ATA_CMD_WRITE_DMA_EXT    0x35
#define ATA_CMD_IDENTIFY         0xEC
#define ATA_CMD_FLUSH_CACHE_EXT  0xEA

typedef struct {
    uint32_t clb;
    uint32_t clbu;
    uint32_t fb;
    uint32_t fbu;
    uint32_t is;
    uint32_t ie;
    uint32_t cmd;
    uint32_t rsv0;
    uint32_t tfd;
    uint32_t sig;
    uint32_t ssts;
    uint32_t sctl;
    uint32_t serr;
    uint32_t sact;
    uint32_t ci;
    uint32_t sntf;
    uint32_t fbs;
    uint32_t rsv1[11];
    uint32_t vendor[4];
} hba_port_t;

typedef struct {
    uint32_t cap;
    uint32_t ghc;
    uint32_t is;
    uint32_t pi;
    uint32_t vs;
    uint32_t ccc_ctl;
    uint32_t ccc_pts;
    uint32_t em_loc;
    uint32_t em_ctl;
    uint32_t cap2;
    uint32_t bohc;
    uint8_t rsv[116];
    uint8_t vendor[128];
    hba_port_t ports[AHCI_MAX_PORTS];
} hba_mem_t;

typedef struct {
    uint8_t cfl;
    uint8_t a;
    uint16_t w;
    uint32_t c;
    uint32_t rsv0;
    uint32_t prdtl;
    uint32_t ctba;
    uint32_t ctbau;
} __attribute__((packed)) hba_cmd_header_t;

typedef struct {
    uint32_t dba;
    uint32_t dbau;
    uint32_t rsv0;
    uint32_t dbc;
} __attribute__((packed)) hba_prdt_entry_t;

typedef struct {
    uint8_t  cfl;
    uint8_t  a;
    uint16_t flags;
    uint32_t c;
    uint32_t rsv0;
    uint32_t prdtl;
} __attribute__((packed)) hba_cmd_tbl_t;

typedef struct {
    uint32_t dba;
    uint32_t dbau;
    uint32_t rsv0;
    uint32_t dbc;
} __attribute__((packed)) hba_prdt_t;

#define AHCI_MAX_PRDT 65535

extern hba_mem_t *ahci_abar;
extern int ahci_port_count;

int ahci_init(pci_dev_t *dev);
int ahci_port_init(int port);
int ahci_read(int port, uint32_t start, uint32_t count, void *buf);
int ahci_write(int port, uint32_t start, uint32_t count, const void *buf);
int ahci_identify(int port, void *buf);
int ahci_flush(int port);