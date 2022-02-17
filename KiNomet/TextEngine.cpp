/*
-------------------------------------------------------------------------------
File:		text,cpp
Author:		Tim Crockford (raven@shakari.myvnc.com)
Version:	0.1
-------------------------------------------------------------------------------
Refer to header file for detailed descriptions of this class.
*/

#ifndef __text_cpp__
#define __text_cpp__
#include "Cinepak.h"
#include "TextEngine.h"

TextEngine::TextEngine(unsigned char* font_array,
	unsigned short* palette_array,
	unsigned char	font_size,
	unsigned char* size_array, unsigned short* screenBuffer)
{
	this->initialize(font_array, palette_array, font_size, size_array, false, screenBuffer);
}

TextEngine::TextEngine(unsigned char* font_array,
	unsigned short* palette_array,
	unsigned char	font_size,
	unsigned char	fixed_size, unsigned short* screenBuffer)
{
	// Create a temporary array which becomes the 'size' array for the font
	unsigned char* size_array = new unsigned char[GLYPHS];

	// Copy fixed_size into each element of the size array.
	for (unsigned char x = 0; x < GLYPHS; x++) { size_array[x] = fixed_size; }

	// Call the other constructor will the new size array as a parameter.
	this->initialize(font_array, palette_array, font_size, size_array, true, screenBuffer);
}

TextEngine::~TextEngine()
{
	// Delete the widths array if it is necessary
	if (this->dynamic)
	{
		delete[] this->widths;
	}
	this->widths = NULL;

	// Delete the offsets array (this was dynamic, so it _will_ need to be
	// deleted.)
	delete[] this->offsets;
	this->offsets = NULL;
}

void TextEngine::initialize(unsigned char* font_array,
	unsigned short* palette_array,
	unsigned char	 font_size,
	unsigned char* size_array,
	bool setDynamic, unsigned short* screenBuffer)
{
	// Sets a pointer to the font array so the print routine can use it.
	this->font = font_array;

	// Load the font palette into memory.
	this->loadPalette(palette_array);

	// Store the height on the font.
	this->height = font_size;

	// Store the font widths array, and generate the offsets array.
	this->widths = size_array;
	this->offsets = new unsigned short[GLYPHS];
	this->calcOffsets();

	// Set the 'dynamic' boolean variable
	this->dynamic = setDynamic;


}

void TextEngine::calcOffsets()
{
	// Set the initial offset to zero
	this->offsets[0] = 0;

	// Create the offset table by adding the width of the current character and
	// the current running total of the total offset value and storing in the
	// next offset.
	for (unsigned short loop = 0; loop < (GLYPHS - 1); loop++)
	{
		this->offsets[loop + 1] = this->offsets[loop] + this->widths[loop];
	}
}

void TextEngine::loadPalette(unsigned short* palette)
{
	// Copy only the first 16 entries, the rest are irrelavent as only 16 color
	// palettes will be used. Should be plenty, even with anti-aliasing.
	palMem = palette;
}

unsigned short TextEngine::length(char* text)
{
	// Define a temporary variable to store the total
	unsigned short total = 0;

	// Add the widths of the characters in text
	for (unsigned short n = 0; n < strlen(text); n++)
	{
		// Determines the ascii value of the character and subtracts the value
		// of the space character (since that is our first character).
		total += this->widths[text[n] - ' '];
	}

	// Return the total
	return (total);
}

void TextEngine::print(char* text, unsigned char x, unsigned char y)
{
	// Okay, first step is to declare some variables used by this function.
	// I mean, text doesn't just appear after all. I'll try and describe each
	// variable as I declare it.

	// The shift_offset variable contains the current offset value within the
	// current word. In 16 color mode, we're writing 4 pixels at a time, so
	// this will keep track of what nibble of the word we're currently in.
	unsigned short		shift_offset;

	// The mem_offset variable keeps track of what the address of the current
	// memory block is. Basically it is the upper left value of the tile we're
	// currently dealing with. The next_block boolean variable is also used to
	// determine the next value of this variable. It detemines the value to get
	// to the next column of tiles.
	unsigned short		mem_offset;
	bool	next_block;

	// The row_offset variable keeps track of the current row in the tile. That
	// a clear enough explaination? No? Well, look at the code, it should
	// hopefully make things clearer.
	unsigned short		row_offset;
	unsigned short		row_offset_init;

	// These are just some other variables used for counters, looping,
	// the usual, etc.
	unsigned short		n, a, b;
	unsigned short		glyph_width = offsets[GLYPHS - 1] + widths[GLYPHS - 1];

	// Okay, here's where the code starts. Firstly we determine the initial
	// values based on the x,y parameters those pesky users gave us.

	// Determine the initial row offset in memory for the text
	row_offset_init = (y << 1);

	// Now determine the initial memory offset. Sorry, but this uses a modulo
	// function :( Any recommendations are heartily accepted.
	n = x % 8;
	mem_offset = (x - n) * 40;
	if (n > 3)
	{
		shift_offset = n - 4;
		next_block = true;
	}
	else
	{
		shift_offset = n;
		next_block = false;
	}
	int curX = x;
	int curY = y;
	// Okay, the main block of code begins here. This will copy the text across
	// to the display as needed.	
	for (n = 0; n < strlen(text); n++)
	{
		// Get the offset and width of the character to print
		unsigned short offset = offsets[text[n] - ' '];
		unsigned short width = widths[text[n] - ' '];

		// This loop prints the character column by column
		for (a = 0; a < width; a++)
		{
			// Reset the row position for each column
			row_offset = row_offset_init;

			// This loop prints the column, pixel by pixel. It looks up the
			// font array to get the pixel at that point.  (The shift offset
			// value is used to move the pixel into the correct position in
			// the word, as we're dealing with 4 pixels per word.)
			for (b = 0; b < height; b++)
			{
				screenBuffer[mem_offset + row_offset] |=
					palMem[font[b * glyph_width + a + offset]] << (shift_offset << 2);

				row_offset += 2;
			}

			// Increase the shift offset, and move onto the next word if the
			// current one is finished. If the current tile is done, then the
			// next column of tiles is selected.
			shift_offset++;
			if (shift_offset == 4)
			{
				shift_offset = 0;

				if (next_block)
				{
					mem_offset += 320;
					mem_offset--;

					// This section will allow text to wrap on the screen if it
					// exceeds the width of the display.
					if (mem_offset == 9600)
					{
						mem_offset = 0;
						row_offset_init += ((height << 1) + 4);
					}
				}
				else
				{
					mem_offset++;
				}

				next_block = !next_block;
				curX++;
			}
		}
		curY++;
	}
}

// This function simply clears the screen.
void TextEngine::clear()
{
	for (unsigned short x = 0; x < 16384; x++)
	{
		//charBaseBlock[x] = 0;
	}
}

// This function will set up the map for BG0. This doesn't change, as the text
// is blitted onto the tile memory. Tile 0xFF is kept blank, as it will be the
// transparent background.
void TextEngine::setupMap()
{
	//for (unsigned short x = 0; x < 32; x++)
	//{
	//	for (unsigned short y = 0; y < 32; y++)
	//	{
	//		if ((x < 30) && (y < 20))
	//		{
	//			screenBaseBlock[y * 32 + x] = y + x * 20;
	//		}
	//		else
	//		{
	//			screenBaseBlock[y * 32 + x] = 0xFF;
	//		}
	//	}
	//}
}

#endif
