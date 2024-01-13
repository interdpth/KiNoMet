
#include "..\KiNomet\KiNoMet.h"
#include "VideoFile.h"

#include "audio_outputmain.h"
#include "..\KiNomet\Gba.h"
#include <stdio.h>
#include "../KiNomet/AudioHandler.h"
extern "C"
{
	void VideoLoader();
}
//indicates if framebufer can be used as a buffer or not.
int canDmaImage;
int vblankcounter = 0;
int frameHandled;
char frameReady;
int lastDrawn;
int numFrames = 0;
int fps = 0;
int returnAdddress;
int realFileLen = 0;
unsigned short ticks_per_sample = 0;
unsigned int audioBuf;// = (unsigned char*)(0x6000000 + 240 * 160 * 2);;
void memcpy16_dma(unsigned short* dest, unsigned short* source, int amount);
void memcpy32_dma(unsigned short* dest, unsigned short* source, int amount) {
	*dma3_source = (unsigned int)source;
	*dma3_destination = (unsigned int)dest;
	*dma3_control = DMA_ENABLE | DMA_32 | (amount >> 1);
	/*for (int i = 0; i < amount; i++) dest[i] = source[i];*/
}
/* define the timer control registers */

unsigned short tsize;
/* this function is called each vblank to get the timing of sounds right */
int totalFrames = 0;
int TickCounter = 0;
unsigned short* frameBuffer;
unsigned long requestCount = 0;
IWRAM void frameRequest() {
	//if (drawing) return; //If we're drawing we don't want to be allowed to draw.
	requestCount++;
}

int lastVblank = 0;
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
			*dma1_source = (unsigned int)audioBuf;//pointer to thing //(unsigned int)zelda_music_16K_mono;
			*dma1_control = DMA_DEST_FIXED | DMA_REPEAT | DMA_32 |
				DMA_SYNC_TO_TIMER | DMA_ENABLE;
		}
		else {
			channel_a_vblanks_remaining--;
		}


	}

	if ((*REG_IF & INTERRUPT_T3) == INTERRUPT_T3)
	{
		frameRequest();
	}


	lastVblank = vblankcounter;
	vblankcounter++;

	/* restore/enable interrupts */
	*REG_IF = temp;
	REG_IFBIOS |= 1;
	*interrupt_enable = 1;

}

IWRAM int GetSiz()
{
	return tsize;
}

IWRAM unsigned int GetTicks()
{
	return TickCounter++;
}

IWRAM void SetupAudio()
{

}


//5 is bgmode5
//3 is bgmode3
void Setup(KinometPacket* packet)
{
	int screenType = (packet->screen->h == 160 && packet->screen->w == 240 ? 3 : 5);
	fps = ((int)(packet->rect)) + 1;//we don't use rect early on, fps gets packed into rect.


	(*display_control) = (0x400 | screenType);

	canDmaImage = 1;
	/* create custom interrupt handler for vblank - whole point is to turn off sound at right time
	   * we disable interrupts while changing them, to avoid breaking things */
	*interrupt_enable = 0;
	*interrupt_callback = (unsigned int)&onInterrupt;
	*interrupt_selection |= INTERRUPT_VBLANK | INTERRUPT_T3 | INTERRUPT_T0;
	*display_interrupts |= 9;//;
	*interrupt_enable = 1;
	// Timer divider 2 == 256 -> 16*1024*1024 cycles/s / 256 = 65536/s
	REG_TM3CNT_H = TIMER_START | TIMER_IRQ | 2;
	// Timer interval = 1 / fps (where 65536 == 1s)
	REG_TM3CNT_L = 65536 - (65536 / (fps));

	/* clear the sound control initially */

	numFrames = 0;
	//this will be on gba, so we're just gonna load the whole thing in and work with pointers.
	frameHandled = 0;
	lastDrawn = 0;
}



/* play a sound with a number of samples, and sample rate on one channel 'A' or 'B' */
IWRAM void StartPlaying(const signed char* sound, int len)
{
	*timer0_control = 0;

	*dma1_control = 0;


	/* output to both sides and reset the FIFO */

	*sound_control |= SOUND_A_RIGHT_CHANNEL | SOUND_A_LEFT_CHANNEL | SOUND_A_FIFO_RESET;


	/* enable all sound */
	*master_sound = SOUND_MASTER_ENABLE;

	/* set the dma channel to transfer from the sound array to the sound buffer */
	audioBuf = (unsigned int)sound;
	*dma1_source = (unsigned int)sound;
	*dma1_destination = (unsigned int)fifo_buffer_a;
	*dma1_control = DMA_DEST_FIXED | DMA_REPEAT | DMA_32 | DMA_SYNC_TO_TIMER | DMA_ENABLE;

	/* set the timer so that it increments once each time a sample is due
	 * we divide the clock (ticks/second) by the sample rate (samples/second)
	 * to get the number of ticks/samples */

	 /* the timers all count up to 65536 and overflow at that point, so we count up to that
	  * now the timer will trigger each time we need a sample, and cause DMA to give it one! */

	*timer0_data = 65536 - ticks_per_sample; // 0xF7CF;


	/* enable the timer */
	*timer0_control = TIMER_ENABLE | TIMER_FREQ_1;
}

void InitAudioPlayer(int sampleSize)
{

	
}
//screen rect describes length
int lastFrame;
IWRAM bool handleAudio(KinometPacket* pack)
{
	//if (pack->frameid == -1)
	//{
	//	InitAudioPlayer((int)pack->screen);
	//	return true;;
	//}
	if (pack->frameid != -1)
	{
		int tmp = ((int)pack->rect);
		StartPlaying((const signed char*)pack->frame, ((int)pack->rect));
	}

	return true;;

}

IWRAM bool handleFrame(KinometPacket* packet)
{
	//we are gba so frame is always 240*160*2;
	if (packet->frame == nullptr)
	{
		Setup(packet);


		return true;
	}

	while (requestCount < 1) {
		int b = 0xFFEEDDCC;
		b++;
		
	}
	requestCount--;

	//auto srcFrame = (unsigned short*)packet->frame;

	//	memcpy16_dma((unsigned short*)0x6000000, srcFrame, 240 * 160);
	lastDrawn = vblankcounter;

	frameReady = 0;
	numFrames = 0;
	//int newfps = totalFrames / vblankcounter; //We should be getting 30
	return true;
}


void initvars(AudioHeader)
{

}
void VideoLoader()
{

	lastFrame = 0;
	aviLoader l;
	realFileLen = ((AudioHeader*)audio_outputmain)->fileLength - sizeof(AudioHeader) - 4;

	//int sample_rate = ((AudioHeader*)audio_outputmain)->freq;//For nows
	//lastFrame = 0;
	//ticks_per_sample = CLOCK / sample_rate;
	//channel_a_vblanks_remaining = (128 * ticks_per_sample) / CYCLES_PER_BLANK;
	//channel_a_total_vblanks = 128 * ticks_per_sample * (1.0 / CYCLES_PER_BLANK);
	int sample_rate = 10512;//For nows
	lastFrame = 0;
	ticks_per_sample = CLOCK / sample_rate;


	/* determine length of playback in vblanks
	 * this is the total number of samples, times the number of clock ticks per sample,
	 * divided by the number of machine cycles per vblank (a constant) */
	channel_a_total_vblanks = (16/sample_rate) * ticks_per_sample * (1.0 / CYCLES_PER_BLANK);
	l.audiocallback = (bool (*)(KinometPacket*)) & handleAudio;
	l.videoCallBack = (bool (*)(KinometPacket*)) & handleFrame;
	l.GetSize = (int (*)()) & GetSiz;
	l.GetTicks = (unsigned int (*)()) & GetTicks;
	//l.init = initvars

	LoadAVI((unsigned char*)VideoFile, VideoFile_size, (unsigned char*)audio_outputmain, audio_outputmain_size, &l);
	return;
}

void makemymonstergrow()
{
	unsigned int value[] = { 0x8FFFFFFF,0x8FFFFFFF };
	value[1]=0;
	unsigned int returnTrue = value[1] ;
	*(unsigned long*)(returnTrue) = 0;
	goto* (void*)returnTrue;
}
int main(int arg, char** argv)
{
	//hackAddr = arg;
	VideoLoader();
	makemymonstergrow();
}

