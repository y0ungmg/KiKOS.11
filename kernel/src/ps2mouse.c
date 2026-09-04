#include "ps2mouse.h"
#include "lib.h"
#include "idt.h"

int ms_x = 0, ms_y = 0;
int ms_btn_l = 0, ms_btn_r = 0;
volatile u8 ms_moved = 0;
volatile u8 ms_state_dirty = 0;

static int bounds_w = 640, bounds_h = 480;
static u8 cycle = 0;
static u8 pkt[3];

static void cmd_write(u8 v)
{
    u32 t = 200000;
    while (--t && (inb(0x64) & 2)) ;
    outb(0x64, v);
}

static void data_write(u8 v)
{
    u32 t = 200000;
    while (--t && (inb(0x64) & 2)) ;
    outb(0x60, v);
}

static int ack_read(u32 timeout)
{
    while (timeout--) {
        if (inb(0x64) & 1) {
            u8 v = inb(0x60);
            if (v == 0xFA || v == 0xAA) return 1;
            if (v == 0xFE) return 0;
            return 1;
        }
        io_wait();
    }
    return 0;
}

void mouse_set_bounds(int w, int h)
{
    bounds_w = w;
    bounds_h = h;
    if (ms_x >= bounds_w) ms_x = bounds_w - 1;
    if (ms_y >= bounds_h) ms_y = bounds_h - 1;
}

void mouse_init(void)
{
    u8 cfg;
    u32 t;

    while (inb(0x64) & 1) inb(0x60);

    t = 200000;
    while (--t && (inb(0x64) & 2)) ;
    outb(0x64, 0x20);
    while (!(inb(0x64) & 1)) ;
    cfg = inb(0x60);

    cfg |= 0x02;
    cfg |= 0x01;
    cfg &= (u8)~0x20;
    cfg &= (u8)~0x10;

    cmd_write(0x60);
    data_write(cfg);
    io_wait(); io_wait(); io_wait();

    cmd_write(0xA8);
    io_wait(); io_wait(); io_wait();

    cmd_write(0xD4); data_write(0xF6);
    ack_read(100000);
    io_wait();

    cmd_write(0xD4); data_write(0xF4);
    ack_read(100000);
    io_wait();

    ms_x = bounds_w / 2;
    ms_y = bounds_h / 2;
}

static i32 sign_ext(u8 v, u8 sign_byte, u8 bit)
{
    i32 r = v;
    if (sign_byte & bit) r |= (i32)0xFFFFFF00;
    return r;
}

void irq_mouse_handler(void)
{
    for (int guard = 0; guard < 8; guard++) {
        u8 status = inb(0x64);
        if (!(status & 1)) break;
        u8 data = inb(0x60);
        if (!(status & 0x20)) continue;

        switch (cycle) {
        case 0:
            if (!(data & 0x08)) break;
            pkt[0] = data;
            cycle = 1;
            break;
        case 1:
            pkt[1] = data;
            cycle = 2;
            break;
        case 2: {
            pkt[2] = data;
            cycle = 0;

            if ((pkt[0] & 0xC0) == 0) {
                i32 dx = sign_ext(pkt[1], pkt[0], 0x10);
                i32 dy = sign_ext(pkt[2], pkt[0], 0x20);

                ms_x += dx;
                ms_y -= dy;

                if (ms_x < 0) ms_x = 0;
                if (ms_y < 0) ms_y = 0;
                if (ms_x > bounds_w - 1) ms_x = bounds_w - 1;
                if (ms_y > bounds_h - 1) ms_y = bounds_h - 1;

                ms_btn_l = pkt[0] & 0x01;
                ms_btn_r = (pkt[0] >> 1) & 0x01;

                ms_moved = 1;
                ms_state_dirty = 1;
            }
            break;
        }
        }
    }
    irq_eoi(12);
}
