/*
-------------------------------------------------------------------------------
File:		text,h
Author:		Tim Crockford (raven@shakari.myvnc.com)
Version:	0.1
-------------------------------------------------------------------------------
Description:
The text engine for the GBA. You need to create your own font and then convert
it into a header file (using at most a 16 color palette (4 bit) and keeping all
values as unsigned char values.) The author recommends the gfx2gba converted for this task
with the following settings:

> gfx2gba <file>.bmp <file>.h -8 -s 1 -w 16 -o

The font should include all characters from the space character to the tilde
character in the usual ascii order. You will also need to provide an array
containing the length of each character (for variable width fonts) or if you
are using a fixed width font, you can pass in the width of a single character.

As the widths of a font are very unlikely to result in a number larger than
255, the array to store these in should be a unsigned char array.

This engine uses BG0 to display text, so you should not use it in your game if
you intend to use this text engine. It will utilize character base block 3, and
screen base block 23. The first 16 color palette will also be used in the
background palette.

The class will ensure that BG0 is enabled, however ensure that you set the GBA
mode to either 0 or 1 before calling these functions.

You can use multiple instances of this class to create different fonts. You can
even display them on the screen simultaniously as long as they don't overlap
and they use the same palette.
-------------------------------------------------------------------------------
*/

#ifndef __text_h__
#define __text_h__
#include "Gba.h"

#include	<string.h>

// This define will simply determine the number of 'glyphs' or characters in
// the class. Since the data should be the ascii set from space to tilde, this
// is 95 characters.
#define		GLYPHS		95

class TextEngine
{
public:
	// The various constructor/destructor combinations. The destructor will
	// simply erase the map and delete any temporary data. The first
	// constructor should be used for a variable width font with the array
	// containing the widths, whilst the second should be used for a fixed
	// width font, where just the size of the font is passed in.
	// (Note that the size of a character is based not only on the
	// character itself, but the whitespace around it. Based on that, the
	// sum of your size_array array should be the width in pixels of the
	// original bitmap file.) The font_size variable determines the height
	// of the font in pixels. The setDynamic variable should not be
	// actually used in code, it will be set automatically.
	TextEngine(unsigned char* font_array, unsigned short* palette_array,
		unsigned char font_size, unsigned char* size_array, unsigned short* screenBuffer);

	TextEngine(unsigned char* font_array, unsigned short* palette_array,
		unsigned char font_size, unsigned char fixed_size, unsigned short* screenBuffer);

	~TextEngine();

	void	initialize(unsigned char* font_array, unsigned short* palette_array,
		unsigned char font_size, unsigned char* size_array, bool setDynamic, unsigned short* screenBuffer);

	// The print function will print the specified string at the point
	// designated by (x, y).
	void	print(char* text, unsigned char x, unsigned char y);

	// The clear function will clear the screen of any text currently
	// displayed. It does this by flushing the charBaseBlock buffer. The
	// map remains untouched, as since data is written to the char and not
	// the screen base block, the map is static.
	void	clear();

	// The length function will return the length of a string in pixels
	// based on the array passed in at run time.
	unsigned short		length(char* text);

private:
	// The font variable contains the pointer to the font data.
	unsigned char*		font;

	// The widths variable contains the array with the widths of the
	// characters in the font.
	unsigned char* widths;

	// The dynamic variable will determine if the widths array was
	// generated dynamically and will need to be deleted when the object is
	// eventually destroyed.
	bool	dynamic;

	// The offsets variable contains the array which stores the offset of a
	// character from the beginning of the file.
	unsigned short*	offsets;

	// The heights variable contains the height of the font in pixels.
	unsigned char		height;
	unsigned short* palMem;
	unsigned short* screenBuffer;
	// The calcOffsets function will determine the offsets from the widths
	// array, this way they are always available. There's a slight memory
	// overhead to store the array, however the speed increase is worth it.
	// Note that since this is a private function, it doesn't take any
	// parameters or return any data, it will simply operate on the widths
	// and offsets variables.
	void	calcOffsets();

	// The loadPalette function will load the palette into the first slot
	// for the background palette memory. It won't store it anyway, so if
	// it is overwritten, you're gonna get some pretty funky looking text!
	void	loadPalette(unsigned short* palette);



	// The the setupMap function will setup the map so it maps to the
	// correct tiles.
	void	setupMap();
};

#endif
