#pragma once

//save on memory lol

class AudioHandler
{
private:
	//Source info.
	unsigned char* srcBuffer; 
	int length;


	//Playing pointers
	unsigned char* startBuf;
	unsigned char* endBuf;

	unsigned char* tmpBuf;

	unsigned char* currentBuf;
	unsigned char* limitBuf;
	int type;
	int fps;
	void Init(int type, int fp);
public:
	/// <summary>
	/// Basic init.
	/// </summary>
	/// <param name="type">Type of audio handler</param>
	/// <param name="fps">FPS we are </param>
	AudioHandler(int type, int fps);
	AudioHandler(int type, int fps, unsigned char* src, int len);
	bool Exhausted();
	/// <summary>
	/// Queues Audio into the ring buffer. 
	/// </summary>
	/// <param name="src">Audio being added</param>
	/// <param name="len">Length of audio</param>
	void QueueAudio(unsigned char* src, int len);

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
};

