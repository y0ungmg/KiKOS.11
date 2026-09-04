#include "ps2kbd.h"
#include "lib.h"
#include "idt.h"

#define QBUF 64

static volatile char qbuf[QBUF];
static volatile u8 qhead = 0, qtail = 0;
static int shift = 0, ctrl = 0, caps = 0, ext = 0;

static const char map_norm[128] = {
    0, 27, '1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,'\\','z','x','c','v','b','n','m',',','.','/',0,
    '*',0,' ',0,0,0,0,0,0,0,0,0,0,0,0,0,
    '7','8','9','-','4','5','6','+','1','2','3','0','.',
    0,0,0,0,0
};

static const char map_shift[128] = {
    0, 27, '!','@','#','$','%','^','&','*','(',')','_','+','\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
    0,'A','S','D','F','G','H','J','K','L',':','"','~',
    0,'|','Z','X','C','V','B','N','M','<','>','?',0,
    '*',0,' ',0,0,0,0,0,0,0,0,0,0,0,0,0,
    '7','8','9','-','4','5','6','+','1','2','3','0','.',
    0,0,0,0,0
};

static void push(char c)
{
    u8 next = (u8)((qhead + 1) % QBUF);
    if (next == qtail) return;
    qbuf[qhead] = c;
    qhead = next;
}

void kbd_init(void)
{
    while (inb(0x64) & 1) inb(0x60);
}

void irq_kbd_handler(void)
{
    u8 sc = inb(0x60);

    if (sc == 0xE0) { ext = 1; goto done; }

    if (ext) {
        ext = 0;
        /* E0-prefixed keys: push with high bit to avoid ASCII clashes */
        switch (sc) {
        case 0x48: push(0xC8); break;   /* Up    */
        case 0x50: push(0xD0); break;   /* Down  */
        case 0x4B: push(0xCB); break;   /* Left  */
        case 0x4D: push(0xCD); break;   /* Right */
        case 0x1C: push('\n'); break;   /* Keypad Enter */
        default: break;
        }
        goto done_eoi;
    }

    if (sc & 0x80) {
        u8 k = sc & 0x7F;
        if (k == 0x2A || k == 0x36) shift = 0;
        if (k == 0x1D) ctrl = 0;
        goto done_eoi;
    }

    switch (sc) {
    case 0x2A: case 0x36: shift = 1; goto done_eoi;
    case 0x3A: caps ^= 1; goto done_eoi;
    case 0x1D: ctrl = 1; goto done_eoi;
    }

    if (sc >= 0x3B && sc <= 0x44) { push((char)(0xA1 + (sc - 0x3B))); goto done_eoi; } /* F1-F10 */

    char c = shift ? map_shift[sc] : map_norm[sc];
    if (!c) goto done_eoi;

    if (caps && c >= 'a' && c <= 'z') c -= 32;
    else if (caps && c >= 'A' && c <= 'Z') c += 32;

    if (ctrl) {
        if (c >= 'a' && c <= 'z') c -= ('a' - 1);
        else if (c >= 'A' && c <= 'Z') c -= ('A' - 1);
    }

    push(c);

done_eoi:
    irq_eoi(1);
done:
    return;
}

int kbd_pop(void)
{
    if (qtail == qhead) return -1;
    char c = qbuf[qtail];
    qtail = (u8)((qtail + 1) % QBUF);
    return (u8)c;
}
