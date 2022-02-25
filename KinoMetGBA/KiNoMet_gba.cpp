#include "..\KiNomet\KiNoMet.h"
#include "VideoFile.h"
#include "..\KiNomet\Gba.h"
#include <stdio.h>
int frameHandled;
#define ARM __attribute__((__target__("arm")))
#define REG_IFBIOS (*(unsigned short*)(0x3007FF8))

//indicates if framebufer can be used as a buffer or not.
int canDmaImage;
int vblankcounter = 0;
void VBlankIntrWait()
{

	__asm("swi 0x05");

}
char frameReady;
int lastDrawn;
int numFrames = 0;
int fps = 0;
#define REG_VCOUNT *(volatile unsigned short*)0x04000006
void memcpy32_dma(unsigned short* dest, unsigned short* source, int amount) {
	*dma3_source = (unsigned int)source;
	*dma3_destination = (unsigned int)dest;
	*dma3_control = DMA_ENABLE | DMA_32 | (amount>>1);
	/*for (int i = 0; i < amount; i++) dest[i] = source[i];*/
}

void vid_vsync()
{
	while (REG_VCOUNT < 160);
}
/* this function is called each vblank to get the timing of sounds right */

IWRAM void onInterrupt() {

	/* disable interrupts for now and save current state of interrupt */
	*interrupt_enable = 0;
	unsigned short temp = *REG_IF;

	/* look for vertical refresh */
	if ((*REG_IF & INTERRUPT_VBLANK) == INTERRUPT_VBLANK) {

		/* update channel A */
		//if (channel_a_vblanks_remaining == 0) {
		//	/* restart the sound again when it runs out */
		//	channel_a_vblanks_remaining = channel_a_total_vblanks;
		//	*dma1_control = 0;
		//	*dma1_source = (unsigned int)alie;
		//	*dma1_control = DMA_DEST_FIXED | DMA_REPEAT | DMA_32 |
		//		DMA_SYNC_TO_TIMER | DMA_ENABLE;
		//}
		//else {
		//	channel_a_vblanks_remaining--;
		//}

		///* update channel B */
		//if (channel_b_vblanks_remaining == 0) {
		//	/* disable the sound and DMA transfer on channel B */
		//	*sound_control &= ~(SOUND_B_RIGHT_CHANNEL | SOUND_B_LEFT_CHANNEL | SOUND_B_FIFO_RESET);
		//	*dma2_control = 0;
		//}
		//else {
		//	channel_b_vblanks_remaining--;
		//}

		//Is it time?
		if (frameReady && vblankcounter - lastDrawn > 30)
		{
			vid_vsync();
			memcpy32_dma((unsigned short*)0x6000000, (unsigned short*)Kinomet_FrameBuffer, 240 * 160);
			lastDrawn = vblankcounter;
			numFrames++;
			frameReady = 0;	
			if (vblankcounter % 60 == 0 ) fps = numFrames / vblankcounter;
		}
		vblankcounter++;
	}

	//if ((*REG_IF & INTERRUPT_T0) == INTERRUPT_T0) {
	//	
	//}
	/* restore/enable interrupts */
	*REG_IF = temp;
	REG_IFBIOS |= 1;
	*interrupt_enable = 1;

}

void Setup()
{
	(*(unsigned short*)0x4000000) = 0x403;
	canDmaImage = 1;
	/* create custom interrupt handler for vblank - whole point is to turn off sound at right time
	   * we disable interrupts while changing them, to avoid breaking things */
	*interrupt_enable = 0;
	*interrupt_callback = (unsigned int)&onInterrupt;
	*interrupt_selection |= INTERRUPT_VBLANK | INTERRUPT_T0;
	*display_interrupts |= 9;//;
	*interrupt_enable = 1;
	//timerEnable(0);
	//timerSet(0x4000, 0, TIMER_FREQ_1024 | TIMER_ENABLE);
	/* clear the sound control initially */
	*sound_control = 0;
}

void handleFrame(unsigned char* framePointer)
{
	//we are gba so frame is always 240*160*2;


	//unsigned short* srcFrame = (unsigned short*)framePointer;
	//for (int y = 160 - 1; y >= 0; y--)
	//{
	//	memcpy16_dma(
	//		&((unsigned short*)0x6000000)[y * 240], srcFrame, 240); srcFrame += 240;
	//}
	frameReady = 1;

}

/* play a sound with a number of samples, and sample rate on one channel 'A' or 'B' */
void play_sound(const signed char* sound, int total_samples, int sample_rate, char channel) {
	/* start by disabling the timer and dma controller (to reset a previous sound) */
	
	if (channel == 'A') {
		*dma1_control = 0;
	}
	else if (channel == 'B') {
		*dma2_control = 0;
	}

	/* output to both sides and reset the FIFO */
	if (channel == 'A') {
		*sound_control |= SOUND_A_RIGHT_CHANNEL | SOUND_A_LEFT_CHANNEL | SOUND_A_FIFO_RESET;
	}
	else if (channel == 'B') {
		*sound_control |= SOUND_B_RIGHT_CHANNEL | SOUND_B_LEFT_CHANNEL | SOUND_B_FIFO_RESET;
	}

	/* enable all sound */
	*master_sound = SOUND_MASTER_ENABLE;

	/* set the dma channel to transfer from the sound array to the sound buffer */
	if (channel == 'A') {
		*dma1_source = (unsigned int)sound;
		*dma1_destination = (unsigned int)fifo_buffer_a;
		*dma1_control = DMA_DEST_FIXED | DMA_REPEAT | DMA_32 | DMA_SYNC_TO_TIMER | DMA_ENABLE;
	}
	else if (channel == 'B') {
		*dma2_source = (unsigned int)sound;
		*dma2_destination = (unsigned int)fifo_buffer_b;
		*dma2_control = DMA_DEST_FIXED | DMA_REPEAT | DMA_32 | DMA_SYNC_TO_TIMER | DMA_ENABLE;
	}

	/* set the timer so that it increments once each time a sample is due
	 * we divide the clock (ticks/second) by the sample rate (samples/second)
	 * to get the number of ticks/samples */
	unsigned short ticks_per_sample = CLOCK / sample_rate;

	
	/* determine length of playback in vblanks
	 * this is the total number of samples, times the number of clock ticks per sample,
	 * divided by the number of machine cycles per vblank (a constant) */
	if (channel == 'A') {
		channel_a_vblanks_remaining = (total_samples * ticks_per_sample) / CYCLES_PER_BLANK;
		channel_a_total_vblanks = channel_a_vblanks_remaining;
	}
	else if (channel == 'B') {
		channel_b_vblanks_remaining = (total_samples * ticks_per_sample) / CYCLES_PER_BLANK;
	}

}


int main()
{
	numFrames = 0;
	//this will be on gba, so we're just gonna load the whole thing in and work with pointers.
	frameHandled = 0;
	lastDrawn = 0;
	Setup();
	//	play_sound((const signed char*)alie, alie_size, 10512, 'A');
	LoadAVI((unsigned char*)VideoFile, VideoFile_size, &handleFrame);
	return 0;
}

