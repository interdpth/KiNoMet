#pragma once
#include <vector>
#include <stdlib.h>
#include "AudioDataPacket.h"
#include "AudioKinometPacket.h"
using namespace std;
//save on memory lol
#define GBA_RING_MEM 0x6000000 + 240 * 160 * 2

#define RING_SIZE (240 * 160 * 2) + 0x1000
enum AudioVersion
{
	V0,
	V1,
	V2,
	UNK
};

typedef struct AudioHeader
{
	unsigned long hdr;
	unsigned long compressedlength;
	unsigned long fileLength;
	unsigned short type;
	unsigned short fps;
	unsigned long freq;	
	unsigned char data[];//get address wherever your buffer is after reading freq 
}AudioHeader;

//we need a callback to get data 
extern int (*getSize)();

class AudioHandler
{
private:
	vector<AudioDataPacket*> packets;
		//Source info.
	unsigned char* srcBuffer; 


	unsigned char* BeginBuffer;
	unsigned char* tmpBuf;
	unsigned char* currentBuf;
	unsigned char* limitBuf;
	unsigned long filesize;
	int type;
	int fps;
	int sample_rate;
	bool swapped;
	int swapsize;

	//we need a callback to get data 
	int (*GetSize)();

	void Swap();

public:
	/// <summary>
/// Main INit
/// </summary>
/// <param name="type"></param>
/// <param name="fp"></param>
/// <param name="sam"></param>
/// 
	virtual void Init(int type, int l, int fp, int sam);
	/// <summary>
	/// Calls maint init
	/// </summary>
	/// <param name="type"></param>
	/// <param name="fp"></param>
	/// <param name="sam"></param>
	virtual void InitAudioHandler(AudioHeader* p);
	
	/// <summary>
	/// Basic init.
	/// </summary>
	/// <param name="type">Type of audio handler</param>
	/// <param name="fps">FPS we are </param>
	AudioHandler(int type, int fp, int sam, int frames, int rsize, int (*func)());
	virtual int Copy(AudioDataPacket* curPack, unsigned char* dstBuf, int size);
	/// <summary>
	/// Basic init, but also queues track.
	/// </summary>
	/// <param name="type"></param>
	/// <param name="fps"></param>
	/// <param name="sam"></param>
	/// <param name="src"></param>
	/// <param name="len"></param>
	AudioHandler(AudioHeader* src,   int frames, int (*func)());

	virtual AudioDataPacket* ProcessPackets();
	/// <summary>
	/// Returns current packet beging processed.
	/// </summary>
	/// <returns></returns>
	virtual AudioDataPacket* GetCurrentPacket();

	/// <summary>
	/// True when main buffer is exhausted
	/// </summary>
	/// <returns></returns>
	virtual bool Exhausted();
	virtual void ClearAudio();
	/// <summary>
	/// Queues Audio into the ring buffer. 
	/// </summary>
	/// <param name="src">Audio being added</param>
	/// <param name="len">Length of audio</param>
	virtual void QueueAudio(AudioDataPacket* packet);

	/// <summary>
	/// Dump audio to buffer.
	/// </summary>
	virtual int ProcessAudio();

	/// <summary>
	/// Attempts to sync audio to frame. Handly differently per type.
	/// 0 = source length / fps * frame;
	/// 1 = index[frame]
	/// </summary>
	/// <param name="frame">Frame to seek to</param>
	/// <returns>True if frame isn't out of bounds. </returns>
	virtual bool SeekAudio(int frame);

	/// <summary>
	/// Returns ring bytes left
	/// </summary>
	/// <returns></returns>
	virtual int GetRemainingBytes();

	/// <summary>
	/// Returns ring byte position
	/// </summary>
	virtual int GetBytesUsed();

	virtual int GetSampleFreq();

	AudioDataPacket* StartProcessing();
	virtual int GetType();
	virtual unsigned char* GetBuffer();
	virtual AudioDataPacket* GetNextFrame();
};

