
#ifndef WAKS_ARCH_H
#define WAKS_ARCH_H

#include "./types.h"

#ifdef __cplusplus 
extern "C" {
#endif 

// posix compatible syscall numbers for baremetal
#define WAKS_SYS_READ   0 
#define WAKS_SYS_WRITE  1
#define WAKS_SYS_BREAK  12 
#define WAKS_SYS_ALLOC  9 
#define WAKS_SYS_EXIT   60

// vga && hardware display 
#define WAKS_VGA_MEMORY ((volatile waks_u16*)0xB8000)
#define WAKS_VGA_WIDTH  80
#define WAKS_VGA_HEIGHT 25
// vga console print

// this allows us to write characters into the video memory buffer 
// we move the color to the high byte, and the lower byte remains 
// for attribute. VGA hardware reads the 16 bit word and renders out 
// the glyph in the specified color.
waks_u16 waks_vga_entry(waks_uchar uc, waks_uchar color);

// writes one character to the screen buffer, tracking a software cursor.
void     waks_vga_putc(waks_char c);

// this reads data from active input device (UART serial)
// return the number of successfully read bytes
long  waks_bm_read(int flag, waks_char *buf, waks_ssize len); 

// this writes output data to active display/serial outputs
// returns the number of bytes successfully written
long  waks_bm_write(int flag, const waks_char *buf, waks_ssize len);

///////////////////////////////////////////////////////////////////////////
// add this to your linker file
// @NOTE(waks-work): your linkeer script must export the heap start symbol
//     . = ALIGN(4096);
//     _waks_heap_start = .;
//
//////////////////////////////////////////////////////////////////////////

// linker symbol marking the start of free memory 
extern waks_uchar _waks_heap_start[]; 

// this adjusts or queries the current program break address.
// if parameter is 0 or null returns the current break address 
// returns the current/new program break address as a long, -1 on error.
long  waks_bm_break(void *addr); 

// this allocates a chunk of contigous physical memory.
// returns a pointer to the start address of the allocated block
// or 0 if it fails.
void *waks_bm_alloc(waks_ssize size); 

// this terminates execution, print halt messages and triggers shutdown
void  waks_bm_exit(void);

// this is a baremetal syscall dispatcher interface and accepts upto 6 syscall 
// arguement 
// n is the syscall code
//
// returns a value or -1 if it fails
long  waks_bm_syscall6(long n, long a1, long a2, long a3, long a4, long a5, long a6); 

#ifdef __cplusplus
}
#endif

#endif // WAKS_ARCH_H 

#ifdef WAKS_ARCH_IMPLEMENTATION

// Port I/O primitives — the freestanding equivalent of waks_syscall6.
// On Linux, waks_syscall6 talks to the OS via the syscall instruction.
// On bare metal, there is no OS underneath us — direct hardware access
// happens through the in/out instructions instead.

// writes out a single byte(8bits) from AL register to an io port.
static inline void waks_outb(waks_u16 port, waks_uchar value)
{
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

// reads a single byte(8bits) from an io port into an AL register
static inline waks_uchar waks_inb(waks_u16 port)
{
    waks_uchar ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// writes out a single word(16bits) from AL register to an io port.
static inline void waks_outw(waks_u16 port, waks_u16 value)
{
    __asm__ volatile ("outw %0, %1" : : "a"(value), "Nd"(port));
}


// reads a single word(16bits) from an io port into an AL register
static inline waks_u16 waks_inw(waks_u16 port)
{
    waks_u16 ret;
    __asm__ volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

#define WAKS_COM1 0x3F8

void waks_serial_init(void)
{
    waks_outb(WAKS_COM1 + 1, 0x00); // disable interrupts
    waks_outb(WAKS_COM1 + 3, 0x80); // enable DLAB (set baud rate divisor)
    waks_outb(WAKS_COM1 + 0, 0x03); // divisor low byte (38400 baud)
    waks_outb(WAKS_COM1 + 1, 0x00); // divisor high byte
    waks_outb(WAKS_COM1 + 3, 0x03); // 8 bits, no parity, one stop bit
    waks_outb(WAKS_COM1 + 2, 0xC7); // enable FIFO, clear, 14-byte threshold
    waks_outb(WAKS_COM1 + 4, 0x0B); // IRQs enabled, RTS/DSR set
    waks_outb(WAKS_COM1 + 4, 0x1E); // enable loopback mode
    waks_outb(WAKS_COM1 + 0, 0xAE); // send a test byte
	
    if (waks_inb(WAKS_COM1 + 0) != 0xAE) return; 
	waks_outb(WAKS_COM1 + 4, 0x0F); // disable loopback, enable normal operation (IRQs, RTS/DSR/OUT2)
}

static inline waks_bool waks_serial_transmit_empty(void)
{
    return waks_inb(WAKS_COM1 + 5) & 0x20;
}

static inline waks_bool waks_serial_recieved(void)
{
	return waks_inb(WAKS_COM1 + 5) & 0x01; // LSR Bit 0:  Data ready
}

static inline void waks_serial_putc(waks_char c)
{
    while (!waks_serial_transmit_empty());
    waks_outb(WAKS_COM1, (waks_uchar)c);
}

static inline waks_char waks_serial_getc(void) 
{
    while (!waks_serial_recieved());
    return (waks_char)waks_inb(WAKS_COM1);
}

// VGA
static waks_u16   waks_vga_row = 0;
static waks_u16   waks_vga_col = 0; 
static waks_uchar waks_vga_color = 0xF;

waks_u16 waks_vga_entry(waks_uchar uc, waks_uchar color)
{
	return (waks_u16)uc | ((waks_u16)color << 8);
}

void waks_vga_putc(waks_char c) 
{
	// reset column to 0 and advanced row
	if (c == '\n') {
		waks_vga_col = 0;
		if (++waks_vga_row == WAKS_VGA_HEIGHT) waks_vga_row = 0;
		return;
	}
	
	// reset column to 0 only
	if (c == '\r') {
		waks_vga_col = 0;
		return;
	}

	const waks_u16 index   = waks_vga_row * WAKS_VGA_WIDTH + waks_vga_col;

	// write packed entry to WAKS_VGA_MEMORY[index], advance column 
	WAKS_VGA_MEMORY[index] = waks_vga_entry((waks_uchar)c, waks_vga_color); 

	if (++waks_vga_col == WAKS_VGA_WIDTH) {
		waks_vga_col = 0;
		if (++waks_vga_row == WAKS_VGA_HEIGHT) waks_vga_row = 0;
	}
}

long waks_bm_read(int flag, waks_char *buf, waks_ssize len) 
{
	(void)flag;
	for (waks_ssize i = 0; i < len; i++) {
        buf[i] = waks_serial_getc();

		// echo character back to the screen
		waks_vga_putc(buf[i]);
		if (buf[i] == '\n' || buf[i] == '\r') return (long)(i+1);
	}
	return (long)len;

}
long waks_bm_write(int flag,const waks_char *buf, waks_ssize len) 
{
	(void)flag;
	for (waks_ssize i = 0; i < len; i++) {
        waks_char c = buf[i];
		waks_serial_putc(c);
		waks_vga_putc(c);
	}
	return (long)len;
}

// BM_BREAK 

// tracking variable for dynamic program break 
static waks_uchar *waks_g_program_break = 0;

long waks_bm_break(void *addr) 
{
	// lazy initialisation at first call
	if (waks_g_program_break == 0)
		waks_g_program_break = _waks_heap_start;

	// query mode: return current break location
	if (addr == 0) return (long)waks_g_program_break;

	waks_uchar *requested_break = (waks_uchar *)addr;

	if (requested_break < _waks_heap_start) return -1;

	waks_g_program_break = requested_break;
	return (long)waks_g_program_break;
}

void *waks_bm_alloc(waks_ssize size) 
{
     if (size == 0) return (void *)waks_g_program_break;

	 // ensure byte alignment
	 size = (size + 15) & ~15;

	 // fetch the current base pointer 
	 long current_ptr = waks_bm_break(0);
	 if (current_ptr == -1) return 0;

	 // advance the program break by requested size
	 long new_ptr = waks_bm_break((void *)(current_ptr + size));
	 if (new_ptr == -1) return 0;

	 return (void *)current_ptr;
} 

void waks_bm_exit(void) 
{
	// print the msg 
	waks_char msg[] = "\n -SYSTEM HALTED-";
	waks_bm_write(1, msg, sizeof(msg) - 1);

	// shut down qemu through the debug exit port exit 
	waks_outw(0x604, 0x2000);

	// qemu shutdown port fallback 
	waks_outw(0xB004, 0x2000);

	// disable interupts and halt the cpu infinitely 
	while(1) __asm__ volatile ("cli;hlt");
}

// @TODO(waks-work): document the usage of this and may be remove the unused  parameters
//                   but we may need them
long waks_bm_syscall6(long n, long a1, long a2, long a3, long a4, long a5, long a6) {
    (void)a4; (void)a5; (void)a6;
	switch(n) {
		case WAKS_SYS_READ: { 
			return waks_bm_read((int)a1, (waks_char*)a2, (waks_ssize)a3); 
		} break; 
        case WAKS_SYS_WRITE: {
			return waks_bm_write((int)a1, (const waks_char*)a2, (waks_ssize)a3); 
		} break; 
        case WAKS_SYS_BREAK: {
            return waks_bm_break((void *)a1); 
		} break; 
        case WAKS_SYS_ALLOC: { 
			return (long)waks_bm_alloc((waks_ssize)a1);
		} break;
        case WAKS_SYS_EXIT: {
			waks_bm_exit(); 
			return 0;
		} break;
		default: return -1;
	}
}

#endif //WAKS_ARCH_IMPLEMENTATION
