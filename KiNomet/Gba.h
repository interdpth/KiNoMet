#pragma once
#ifndef GBA_H
#define GBA_H
#ifndef GBA
#include <stdio.h>
#include <stdlib.h>
#else 
void printf(char* message, ...);

#endif
extern volatile unsigned long* display_control;
#define MODE0 0x00
#define BG0_ENABLE 0x100
#define BG1_ENABLE 0x200
#define BG2_ENABLE 0x400
#define BG3_ENABLE 0x800

/* the button register holds the bits which indicate whether each button has
 * been pressed - this has got to be extern volatile as well
 */
extern volatile unsigned short* buttons;

/* the bit positions indicate each button - the first bit is for A, second for
 * B, and so on, each constant below can be ANDED into the register to get the
 * status of any one button */
#define BUTTON_A (1 << 0)
#define BUTTON_B (1 << 1)
#define BUTTON_SELECT (1 << 2)
#define BUTTON_START (1 << 3)
#define BUTTON_RIGHT (1 << 4)
#define BUTTON_LEFT (1 << 5)
#define BUTTON_UP (1 << 6)
#define BUTTON_DOWN (1 << 7)
#define BUTTON_R (1 << 8)
#define BUTTON_L (1 << 9)

 /* the control registers for the four tile layers */
extern volatile unsigned short* bg0_control;
extern volatile unsigned short* bg1_control;
extern volatile unsigned short* bg2_control;
extern volatile unsigned short* bg3_control;

/* palette is always 256 colors */
#define PALETTE_SIZE 256

/* the address of the color palette */
extern volatile unsigned short* bg_palette;

/* define the timer control registers */
extern volatile unsigned short* timer0_data;
extern volatile unsigned short* timer0_control;

/* make defines for the bit positions of the control register */
#define TIMER_FREQ_1 0x0
#define TIMER_FREQ_64 0x2
#define TIMER_FREQ_256 0x3
#define TIMER_FREQ_1024 0x4
#define TIMER_ENABLE 0x80

/* the GBA clock speed is fixed at this rate */
#define CLOCK 16777216 
#define CYCLES_PER_BLANK 280896

/* turn DMA on for different sizes */
#define DMA_ENABLE 0x80000000
#define DMA_16 0x00000000
#define DMA_32 0x04000000

/* this causes the DMA destination to be the same each time rather than increment */
#define DMA_DEST_FIXED 0x400000

/* this causes the DMA to repeat the transfer automatically on some interval */
#define DMA_REPEAT 0x2000000
#define VIDEOCAPTURE 0x20000000
/* this causes the DMA repeat interval to be synced with timer 0 */
#define DMA_SYNC_TO_TIMER 0x30000000

/* pointers to the DMA source/dest locations and control registers */
extern volatile unsigned int* dma0_source;
extern volatile unsigned int* dma0_destination;
extern volatile unsigned int* dma0_control;

extern volatile unsigned int* dma1_source;
extern volatile unsigned int* dma1_destination;
extern volatile unsigned int* dma1_control;

extern volatile unsigned int* dma2_source;
extern volatile unsigned int* dma2_destination;
extern volatile unsigned int* dma2_control;

extern volatile unsigned int* dma3_source;
extern volatile unsigned int* dma3_destination;
extern volatile unsigned int* dma3_control;
/* the global interrupt enable register */
extern volatile unsigned short* interrupt_enable;

/* this register stores the individual interrupts we want */
extern volatile unsigned short* interrupt_selection;

/* this registers stores which interrupts if any occured */
extern volatile unsigned short* REG_IF;

/* the address of the function to call when an interrupt occurs */
extern volatile unsigned int* interrupt_callback;

/* this register needs a bit set to tell the hardware to send the vblank interrupt */
extern volatile unsigned short* display_interrupts;

/* the interrupts are identified by number, we only care about this one */
#define INTERRUPT_VBLANK 0x1

/* allows turning on and off sound for the GBA altogether */
extern volatile unsigned short* master_sound;
#define SOUND_MASTER_ENABLE 0x80

/* has various bits for controlling the direct sound channels */
extern volatile unsigned short* sound_control;

/* bit patterns for the sound control register */
#define SOUND_A_RIGHT_CHANNEL 0x100
#define SOUND_A_LEFT_CHANNEL 0x200
#define SOUND_A_FIFO_RESET 0x800
#define SOUND_B_RIGHT_CHANNEL 0x1000
#define SOUND_B_LEFT_CHANNEL 0x2000
#define SOUND_B_FIFO_RESET 0x8000

/* the location of where sound samples are placed for each channel */
extern volatile unsigned char* fifo_buffer_a;
extern volatile unsigned char* fifo_buffer_b;

/* global variables to keep track of how much longer the sounds are to play */
extern unsigned int channel_a_vblanks_remaining;
extern unsigned int channel_a_total_vblanks;
extern unsigned int channel_b_vblanks_remaining;
#define INT_VBLANK 	0x0001
#define INT_HBLANK 	0x0002
#define INT_VCOUNT 	0x0004
#define INT_TIMER0 	0x0008
#define INT_TIMER1 	0x0010
#define INT_TIMER2 	0x0020
#define INT_TIMER3 	0x0040
#define INT_COM 	0x0080
#define INT_DMA0 	0x0100
#define INT_DMA1	0x0200
#define INT_DMA2 	0x0400
#define INT_DMA3 	0x0800
#define INT_BUTTON 	0x1000
#define INT_CART 	0x2000

#define IWRAM __attribute__((section(".iwram"), target("arm"), noinline))
void* memcpy(void* dest, const void* src, int olen);
#endif
