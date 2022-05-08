#pragma once
#include <vector>
using namespace std;
//save on memory lol

struct AudioHeader
{
	unsigned long hdr;
	unsigned short type;
	unsigned short fps;
	unsigned long freq;
	
	unsigned char* datPtr;//whever your buffer is after reading freq 
};

enum flags
{
	START,
	DATA,
	END,
};
struct AudioPacket
{
	unsigned char eventFlag;
	unsigned char* start;
	int len;
	int tracked;
};
//we need a callback to get data 
extern int (*getSize)();

class AudioHandler
{
private:
	vector<AudioPacket*> packets;
		//Source info.
	unsigned char* srcBuffer; 

	//Playing pointers
	unsigned char* startBuf;
	unsigned char* endBuf;
	unsigned char* tmpBuf;
	unsigned char* currentBuf;
	unsigned char* limitBuf;
	
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
	void Init(int type, int fp, int sam);
	/// <summary>
	/// Calls maint init
	/// </summary>
	/// <param name="type"></param>
	/// <param name="fp"></param>
	/// <param name="sam"></param>

	void Init(AudioHeader* hdr, int len);
	int ringSize;
	/// <summary>
	/// Basic init.
	/// </summary>
	/// <param name="type">Type of audio handler</param>
	/// <param name="fps">FPS we are </param>
	AudioHandler(int type, int fp, int sam, int frames, int rsize, int (*func)());
	int Copy(AudioPacket* curPack, unsigned char* dstBuf, int size);
	/// <summary>
	/// Basic init, but also queues track.
	/// </summary>
	/// <param name="type"></param>
	/// <param name="fps"></param>
	/// <param name="sam"></param>
	/// <param name="src"></param>
	/// <param name="len"></param>
	AudioHandler(unsigned char* src, int len, int fps, int frames, int rsize, int (*func)());
	int Fillbuffers(unsigned int bytesLeft, AudioPacket* curPack);
	void ProcessPackets();
	/// <summary>
	/// Returns current packet beging processed.
	/// </summary>
	/// <returns></returns>
	AudioPacket* GetCurrentPacket();

	/// <summary>
	/// True when main buffer is exhausted
	/// </summary>
	/// <returns></returns>
	bool Exhausted();
	void ClearAudio();
	/// <summary>
	/// Queues Audio into the ring buffer. 
	/// </summary>
	/// <param name="src">Audio being added</param>
	/// <param name="len">Length of audio</param>
	void QueueAudio(AudioPacket* packet);

	/// <summary>
	/// Dump audio to buffer.
	/// </summary>
	int Processs();

	/// <summary>
	/// Attempts to sync audio to frame. Handly differently per type.
	/// 0 = source length / fps * frame;
	/// 1 = index[frame]
	/// </summary>
	/// <param name="frame">Frame to seek to</param>
	/// <returns>True if frame isn't out of bounds. </returns>
	bool SeekAudio(int frame);

	/// <summary>
	/// Returns ring bytes left
	/// </summary>
	/// <returns></returns>
	int GetRemainingBytes();

	/// <summary>
	/// Returns ring byte position
	/// </summary>
	int GetBytesUsed();

	int GetSampleFreq();

	int GetType();
	unsigned char* GetBuffer();

};

