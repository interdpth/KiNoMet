#include "..\KiNomet\KiNoMet.h"
#include "VideoFile.h"
#include "VideoFile.h"
#include "audio_outputmain.h"
#include "..\KiNomet\Gba.h"
#include <stdio.h>
#include "../KiNomet/AudioHandler.h"
int frameHandled;
#define ARM __attribute__((__target__("arm")))
#define REG_IFBIOS (*(unsigned short*)(0x3007FF8))
#define BG2X *(short long*)0x4000020// - BG2X_L - BG2 Reference Point X-Coordinate, lower 16 bit (W)
#define BG2Y *(signed long*)0x400002c// - BG2X_L - BG2 Reference Point X-Coordinate, lower 16 bit (W)
AudioHandler* audio;
//indicates if framebufer can be used as a buffer or not.
int canDmaImage;
int vblankcounter = 0;

char frameReady;
int lastDrawn;
int numFrames = 0;
int fps = 0;
IWRAM void memcpy32_dma(unsigned short* dest, unsigned short* source, int amount) {
	*dma3_source = (unsigned int)source;
	*dma3_destination = (unsigned int)dest;
	*dma3_control = DMA_ENABLE | DMA_32 | (amount >> 1);
	/*for (int i = 0; i < amount; i++) dest[i] = source[i];*/
}
/* define the timer control registers */
volatile unsigned short* timer0_data = (volatile unsigned short*)0x4000100;
volatile unsigned short* timer0_control = (volatile unsigned short*)0x4000102;

/* this function is called each vblank to get the timing of sounds right */
int totalFrames = 0;
unsigned short* frameBuffer;
unsigned long requestCount = 0;
IWRAM void frameRequest() {
	//if (drawing) return; //If we're drawing we don't want to be allowed to draw.
	requestCount++;
}
IWRAM void onInterrupt() {

	/* disable interrupts for now and save current state of interrupt */
	*interrupt_enable = 0;
	unsigned short temp = *REG_IF;

	/* look for vertical refresh */
	if ((*REG_IF & INTERRUPT_VBLANK) == INTERRUPT_VBLANK) {

		/* update channel A */
		if (channel_a_vblanks_remaining == 0) {
			/* restart the sound again when it runs out */
			channel_a_vblanks_remaining = channel_a_total_vblanks;
			*dma1_control = 0;

			*dma1_control = DMA_DEST_FIXED | DMA_REPEAT | DMA_16 |
				DMA_SYNC_TO_TIMER | DMA_ENABLE;
		}
		else {
			channel_a_vblanks_remaining--;
		}

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
	/*	numFrames++;
		auto srcFrame = frameBuffer;
		if (frameReady && numFrames > 30)
		{

		}*/

	}

	if ((*REG_IF & INTERRUPT_T3) == INTERRUPT_T3)
	{
		frameRequest();
	}
	vblankcounter++;
	//if ((*REG_IF & INTERRUPT_T0) == INTERRUPT_T0) {
	//	
	//}
	/* restore/enable interrupts */
	*REG_IF = temp;
	REG_IFBIOS |= 1;
	*interrupt_enable = 1;

}
int GetSiz()
{
	return (*dma1_control) & 0xFFFF;
}
int TickCounter = 0;
IWRAM unsigned int GetTicks()
{
	return TickCounter++;
}


//5 is bgmode5
//3 is bgmode3
void Setup(KinometPacket* packet)
{

	int screenType = (packet->screen->h == 160 && packet->screen->w == 240 ? 3 : 5);
	fps = ((int)(packet->rect)) + 1;//we don't use rect early on, fps gets packed into rect.
	

	(*(unsigned short*)0x4000000) = (0x400 | screenType);

	//if (screenType == 5)
	//{
	//	BG2X = 0xAC;
	//	BG2Y = -80;
	//}
	canDmaImage = 1;
	/* create custom interrupt handler for vblank - whole point is to turn off sound at right time
	   * we disable interrupts while changing them, to avoid breaking things */
	*interrupt_enable = 0;
	*interrupt_callback = (unsigned int)&onInterrupt;
	*interrupt_selection |= INTERRUPT_VBLANK | INTERRUPT_T3;
	*display_interrupts |= 9;//;
	*interrupt_enable = 1;
	// Timer divider 2 == 256 -> 16*1024*1024 cycles/s / 256 = 65536/s
	REG_TM3CNT_H = TIMER_START | TIMER_IRQ | 2;
	// Timer interval = 1 / fps (where 65536 == 1s)
	REG_TM3CNT_L = 65536 - (65536 / (fps));
	//timerEnable(0);
	//timerSet(0x4000, 0, TIMER_FREQ_1024 | TIMER_ENABLE);
	/* clear the sound control initially */

}


/* play a sound with a number of samples, and sample rate on one channel 'A' or 'B' */
IWRAM void StartPlaying(const signed char* sound, int len) {
	/* start by disabling the timer and dma controller (to reset a previous sound) */
	*timer0_control = 0;

	*dma1_control = 0;



	/* set the dma channel to transfer from the sound array to the sound buffer */

	*dma1_source = (unsigned int)sound;
	*dma1_destination = (unsigned int)fifo_buffer_a;
	*dma1_control = DMA_DEST_FIXED | DMA_ENABLE | (len&0xFFFF) ;


}
//screen rect describes length


int lastFrame;
IWRAM void handleAudio(KinometPacket* pack)
{
	//if (pack->frameid == -1)
	//{
	//	auto funcPoint = ((int (*)() )&GetSiz);
	//	audio = new AudioHandler(0, fps, 10512, funcPoint);
	//	*sound_control = 0;
	//	/* output to both sides and reset the FIFO */

	//	*sound_control |= SOUND_A_RIGHT_CHANNEL | SOUND_A_LEFT_CHANNEL | SOUND_A_FIFO_RESET;



	//	/* enable all sound */
	//	*master_sound = SOUND_MASTER_ENABLE;
	//	return;
	//}


	//StartPlaying((const signed char*)pack->frame, (int)pack->rect );
}

IWRAM void handleFrame(KinometPacket* packet)
{
	//we are gba so frame is always 240*160*2;
	if (packet->frame == nullptr)
	{
		Setup(packet);
		numFrames = 0;
		//this will be on gba, so we're just gonna load the whole thing in and work with pointers.
		frameHandled = 0;
		lastDrawn = 0;

		return;
	}

	while (requestCount < 1) {}
	requestCount--;

	auto srcFrame = (unsigned short*)packet->frame;
	//for (int y = 0; y < 160; y++)
	//{
	//	memcpy16_dma(
	//		&((unsigned short*)0x6000000)[y * 240], srcFrame, 240); srcFrame += 240;
	//}

	memcpy16_dma((unsigned short*)0x6000000, srcFrame, 240 * 160);
	lastDrawn = vblankcounter;

	frameReady = 0;
	numFrames = 0;
	int newfps = totalFrames / vblankcounter; //We should be getting 30
}


int main()
{
	lastFrame = 0;
	LoadAVI((unsigned char*)VideoFile, VideoFile_size, (unsigned char*)audio_outputmain, audio_outputmain_size, (void (*)(KinometPacket*) )&handleFrame, (void (*)(KinometPacket*)) &handleAudio,  (int (*)()) &GetSiz, (unsigned int (*)())&GetTicks);

	return 0;
}

