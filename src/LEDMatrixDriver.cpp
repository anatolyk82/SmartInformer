#include "LEDMatrixDriver.h"
#include <Arduino.h>

#include "Font.h"

LEDMatrixDriver::LEDMatrixDriver(uint8_t nsegments, uint8_t ssPin, uint8_t flags, uint8_t* fb) :
	m_nsegments(nsegments),
	m_spiSettings(10000000, MSBFIRST, SPI_MODE0),
	m_flags(flags),
	m_frameBuffer(fb),
	m_ssPin(ssPin)
{
	if (fb == nullptr) {
		m_frameBuffer = new uint8_t[m_nsegments*8];
	}

	clear();

	pinMode(m_ssPin, OUTPUT);
	digitalWrite(m_ssPin, 1);
	SPI.begin();

	tranferCommand(LEDMatrixDriver::CMD_TEST);			      // no test
	tranferCommand(LEDMatrixDriver::CMD_DECODE);			    // No decode mode
	tranferCommand(LEDMatrixDriver::CMD_SCAN_LIMIT | 7);	// all lines

	setEnabled(false);
}

LEDMatrixDriver::~LEDMatrixDriver()
{
	if (m_frameBuffer) {
		delete[] m_frameBuffer;
	}
}


void LEDMatrixDriver::setPixel(int16_t x, int16_t y, bool enabled)
{
	uint8_t* p = getPixelsBytePtr(x, y);
	if (p) {
		uint16_t b = 7 - (x & 7);		// Create a mask to get the required bit
		if (enabled)
			*p |=  (1<<b);
		else
			*p &= ~(1<<b);
	}
}


bool LEDMatrixDriver::getPixel(int16_t x, int16_t y) const
{
	uint8_t* p = getPixelsBytePtr(x, y);
	if (!p) {
		return false;
	}

	uint16_t b = 7 - (x & 7); // Create a mask to check the required bit
	return *p & (1 << b);
}


void LEDMatrixDriver::setColumn(int16_t x, uint8_t value)
{
	//no need to check x, will be checked by setPixel
	for (uint8_t y = 0; y < 8; ++y)
	{
		setPixel(x, y, value & 1);
		value >>= 1;
	}
}

void LEDMatrixDriver::setEnabled(bool enabled)
{
	tranferCommand(CMD_ENABLE | (enabled ? 1 : 0));
}

/**
 * Set display intensity
 *
 * level:
 * 	0 - lowest (1/32)
 * 15 - highest (31/32)
 */

void LEDMatrixDriver::setBrightness(uint8_t level)
{
	// Maximum brightness is 0x0F
	if (level > 0x0F) level = 0x0F;
	tranferCommand(CMD_INTENSITY | level);
}


void LEDMatrixDriver::tranferCommand(uint16_t command)
{
	SPI.beginTransaction(m_spiSettings);
	// Make the slave listen the master
	digitalWrite(m_ssPin, 0);
	// Send the same command to all segments
	for (uint8_t i = 0; i < m_nsegments; i++)
	{
		SPI.transfer16(command);
	}
	digitalWrite(m_ssPin, 1);
	SPI.endTransaction();
}

uint8_t LEDMatrixDriver::reverseByte(uint8_t b) {
   b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
   b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
   b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
	 return b;
}

void LEDMatrixDriver::displayRow(uint8_t row)
{
	// Calculates row address based on flags
	uint8_t address_row = m_flags & INVERT_Y ? 7 - row: row;

	bool display_x_inverted = m_flags & INVERT_DISPLAY_X;
	bool segment_x_inverted = m_flags & INVERT_SEGMENT_X;

	//for x inverted display change iterating order
	//inverting segments may still be needed!
	int16_t from = display_x_inverted ? m_nsegments-1:  0;		// start from ...
	int16_t to =   display_x_inverted ? -1  : m_nsegments;		// where to stop
	int16_t step = display_x_inverted ? -1 :  1;		        // directon

	SPI.beginTransaction(m_spiSettings);
	digitalWrite(m_ssPin, 0);

	for (int16_t d = from; d != to; d += step)
	{
		uint8_t data = m_frameBuffer[d + row*m_nsegments];
		if (segment_x_inverted)
			data = reverseByte(data);
		uint16_t cmd = ((address_row + 1) << 8) | data;
		SPI.transfer16(cmd);
	}

	digitalWrite(m_ssPin, 1);
	SPI.endTransaction();
}


uint8_t* LEDMatrixDriver::getPixelsBytePtr(int16_t x, int16_t y) const
{
	if ((y < 0) or (y >= 8))
		return nullptr;
	if ((x < 0) or (x >= (8*m_nsegments)))
		return nullptr;

	uint16_t B = x >> 3;		// Devide by 8

	return m_frameBuffer + y*m_nsegments + B;
}


void LEDMatrixDriver::display()
{
	for (uint8_t y = 0; y < 8; y++)
	{
		displayRow(y);
	}
}


void LEDMatrixDriver::scroll( ScrollDirection direction )
{
	int cnt = 0;
	switch( direction )
	{
		case ScrollDirection::Up:
			cnt = 7*(m_nsegments);	// moving 7 rows of N segments
			memmove(m_frameBuffer, m_frameBuffer + m_nsegments, cnt);
			memset(m_frameBuffer+cnt, 0, m_nsegments);		// Clear last row
			break;

		case ScrollDirection::Down:
			cnt = 7*m_nsegments; // moving 7 rows of m_nsegments segments
			memmove(m_frameBuffer+m_nsegments, m_frameBuffer, cnt);
			memset(m_frameBuffer, 0, m_nsegments);		// Clear first row
			break;

		case ScrollDirection::Right:
			// Scrolling right needs to be done by bit shifting every uint8_t in the frame buffer
			// Carry is reset between rows
			for (int y = 0; y < 8; y++)
			{
				uint8_t carry = 0x00;
				for (int x = 0; x < m_nsegments; x++)
				{
					uint8_t& v = m_frameBuffer[y*m_nsegments+x];
					uint8_t newCarry = v & 1;
					v = (carry << 7) | (v >> 1);
					carry = newCarry;
				}
			}
			break;

		case ScrollDirection::Left:
			// Scrolling left needs to be done by bit shifting every uint8_t in the frame buffer
			// Carry is reset between rows
			for (int y = 0; y < 8; y++)
			{
				uint8_t carry = 0x00;
				for (int x = m_nsegments-1; x >= 0; x--)
				{
					uint8_t& v = m_frameBuffer[y*m_nsegments+x];
					uint8_t newCarry = v & 0x80;
					v = (carry >> 7) | (v << 1);
					carry = newCarry;
				}
			}
			break;
	}
}

void LEDMatrixDriver::drawSprite( uint8_t* sprite, int x, int y, int width, int height )
{
  // The mask is used to get the column bit from the sprite row
  uint8_t mask = 0x80;
  for( int iy = 0; iy < height; iy++ )
  {
    for( int ix = 0; ix < width; ix++ )
    {
      setPixel(x + ix, y + iy, (bool)(sprite[iy] & mask));

      // Shift the mask by one pixel to the right
      mask = mask >> 1;
    }
    // Reset column mask
    mask = 0x80;
  }
}

void LEDMatrixDriver::drawString( const char* text, int len, int x, int y )
{
  for( int idx = 0; idx < len; idx ++ )
  {
    int c = text[idx] - 32;

    // Stop if char is outside visible area
    if( x + idx * 8  > m_nsegments*8 )
      return;

    // Only draw if char is visible
    if( 8 + x + idx * 8 > 0 ) {
			drawSprite( font[c], x + idx * 8, y, 8, 8 );
    }

  }
}
