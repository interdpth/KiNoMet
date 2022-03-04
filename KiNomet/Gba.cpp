#include "Gba.h"
#ifndef GBA
#include <Windows.h>
#endif

volatile unsigned long* display_control = (volatile unsigned long*)0x4000000;

/* the button register holds the bits which indicate whether each button has
 * been pressed - this has got to be volatile as well
 */
volatile unsigned short* buttons = (volatile unsigned short*)0x04000130;

 /* the control registers for the four tile layers */
volatile unsigned short* bg0_control = (volatile unsigned short*)0x4000008;
volatile unsigned short* bg1_control = (volatile unsigned short*)0x400000a;
volatile unsigned short* bg2_control = (volatile unsigned short*)0x400000c;
volatile unsigned short* bg3_control = (volatile unsigned short*)0x400000e;

/* palette is always 256 colors */
#define PALETTE_SIZE 256

/* the address of the color palette */
volatile unsigned short* bg_palette = (volatile unsigned short*)0x5000000;

/* define the timer control registers */
volatile unsigned short* REG_TM0VAL = (volatile unsigned short*)0x4000100;
volatile unsigned short* REG_TM0CNT = (volatile unsigned short*)0x4000102;

volatile unsigned int* dma0_source = (volatile unsigned int*)0x40000B0;
volatile unsigned int* dma0_destination = (volatile unsigned int*)0x40000B4;
volatile unsigned int* dma0_control = (volatile unsigned int*)0x40000B8;
/* pointers to the DMA source/dest locations and control registers */
volatile unsigned int* dma1_source = (volatile unsigned int*)0x40000BC;
volatile unsigned int* dma1_destination = (volatile unsigned int*)0x40000C0;
volatile unsigned int* dma1_control = (volatile unsigned int*)0x40000C4;

volatile unsigned int* dma2_source = (volatile unsigned int*)0x40000C8;
volatile unsigned int* dma2_destination = (volatile unsigned int*)0x40000CC;
volatile unsigned int* dma2_control = (volatile unsigned int*)0x40000D0;

volatile unsigned int* dma3_source = (volatile unsigned int*)0x40000D4;
volatile unsigned int* dma3_destination = (volatile unsigned int*)0x40000D8;
volatile unsigned int* dma3_control = (volatile unsigned int*)0x40000DC;

/* the global interrupt enable register */
volatile unsigned short* interrupt_enable = (unsigned short*)0x4000208;

/* this register stores the individual interrupts we want */
volatile unsigned short* interrupt_selection = (unsigned short*)0x4000200;

/* this registers stores which interrupts if any occured */
volatile unsigned short* REG_IF = (unsigned short*)0x4000202;

/* the address of the function to call when an interrupt occurs */
volatile unsigned int* interrupt_callback = (unsigned int*)0x3007FFC;

/* this register needs a bit set to tell the hardware to send the vblank interrupt */
volatile unsigned short* display_interrupts = (unsigned short*)0x4000004;

/* allows turning on and off sound for the GBA altogether */
volatile unsigned short* master_sound = (volatile unsigned short*)0x4000084;

/* has various bits for controlling the direct sound channels */
volatile unsigned short* sound_control = (volatile unsigned short*)0x4000082;

/* the location of where sound samples are placed for each channel */
volatile unsigned char* fifo_buffer_a = (volatile unsigned char*)0x40000A0;
volatile unsigned char* fifo_buffer_b = (volatile unsigned char*)0x40000A4;

/* global variables to keep track of how much longer the sounds are to play */
unsigned int channel_a_vblanks_remaining = 0;
unsigned int channel_a_total_vblanks = 0;
unsigned int channel_b_vblanks_remaining = 0;


//SWI 06h(GBA) or SWI 09h(NDS7 / NDS9 / DSi7 / DSi9) - Div
//Signed Division, r0 / r1.
//r0  signed 32bit Number
//r1  signed 32bit Denom
//Return :
//r0  Number DIV Denom; signed
//r1  Number MOD Denom; signed
//r3  ABS(Number DIV Denom); unsigned
//For example, incoming - 1234, 10 should return -123, -4, +123.
//The function usually gets caught in an endless loop upon division by zero.
//Note: The NDS9 and DSi9 additionally support hardware division, by math coprocessor, accessed via I / O Ports, however, the SWI function is a raw software division.
//
//SWI 07h(GBA) - DivArm

void memcpy16_dma(unsigned short* dest, unsigned short* source, int amount) {
    *dma3_source = (unsigned int)source;
    *dma3_destination = (unsigned int)dest;
    *dma3_control = DMA_ENABLE | DMA_16 | amount;
    /*for (int i = 0; i < amount; i++) dest[i] = source[i];*/
}




#ifdef GBA

memcpy(void* dest, const void* src, int olen)
{

    memcpy16_dma((unsigned short*)dest, (unsigned short*)src, olen >> 1);

 

}




int SwiDiv(int num, int denom)
{
	int ret = 0;
	

	__asm("swi 6");
	


	
	return ret;

}



int Div(int num, int denom)
{
#ifdef GBA
	return SwiDiv(num, denom);
#else
	return num / denom;
#endif
}


int __aeabi_div(int num, int denom)
{
	return SwiDiv(num, denom);
}


int __aeabi_ldiv0(int num, int denom)
{
	return SwiDiv(num, denom);
}


int _aeabi_idiv0(int num, int denom)
{
	return SwiDiv(num, denom);
}
int __aeabi_idiv0(int num, int denom)
{
	return SwiDiv(num, denom);
}


//
//int __aeabi_idivmod(int num, int denom)
//{
//	return SwiDivMod(num, denom);
//}
//
//int __divsi3(int num, int denom)
//{
//	return SwiDiv(num, denom);
//}
#endif
