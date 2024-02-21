#pragma once

class Codec
{
private:
	int ctype;
public:
	Codec(int type);
	void* dat;
	virtual void Loader();
	virtual void Player();
};

