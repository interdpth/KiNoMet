#pragma once
#ifdef UI
//On GBA the graphics will be loaded into high vram
class UserInterface
{
private:
	int CurTimeStamp;
public:

	UserInterface();
	void Draw();
	void Fastforward();
	void Rewind();
	void Play();
	void Pause();
};
#endif

