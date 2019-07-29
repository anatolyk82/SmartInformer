#ifndef ESP_LED_MATRIX_DRIVER_H
#define ESP_LED_MATRIX_DRIVER_H

#include <SPI.h>
#include <cstring>

class LEDMatrixDriver
{
	// Commands from the datasheet
	const static uint16_t CMD_ENABLE = 0x0C00;
	const static uint16_t CMD_INTENSITY =	0x0A00;
	const static uint16_t CMD_TEST = 0x0F00;
	const static uint16_t CMD_SCAN_LIMIT = 0x0B00;
	const static uint16_t CMD_DECODE = 0x0900;


	public:
		const static uint8_t INVERT_SEGMENT_X = 1;
		const static uint8_t INVERT_DISPLAY_X = 2;
		const static uint8_t INVERT_Y = 4;

		// Constructor:
		// N - Number of segments
		// ssPin - Select slave pin
		// flags - defines orientation of the matrixes
		// frameBuffer - External frame buffer
		LEDMatrixDriver(uint8_t N, uint8_t ssPin, uint8_t flags = 0, uint8_t* frameBuffer = nullptr);
		~LEDMatrixDriver();

		// We don't want to copy the object
		LEDMatrixDriver(const LEDMatrixDriver& other) = delete;
		LEDMatrixDriver(LEDMatrixDriver&& other) = delete;
		LEDMatrixDriver& operator=(const LEDMatrixDriver& other) = delete;

		// All these commands work on ALL segments
		void setEnabled(bool enabled);

		// Set brightness: 0 - 15
		void setBrightness(uint8_t level);

		void setPixel(int16_t x, int16_t y, bool enabled);
		bool getPixel(int16_t x, int16_t y) const;

		/* Sets pixels in the column acording to value (LSB => y=0) */
		void setColumn(int16_t x, uint8_t value);

		uint8_t getSegments() const { return m_nsegments; }
		uint8_t* getFrameBuffer() const { return m_frameBuffer; }

		// Writes the data to the display
		void display();

		// Writes a single row to the display
		void displayRow(uint8_t row);

		// Clear the framebuffer
		void clear() { memset(m_frameBuffer, 0, 8*m_nsegments); }

		// Draws a sprite at x,y coordinates with width,height pixels
		void drawSprite( uint8_t* sprite, int x, int y, int width, int height );

		void drawString( const char* text, int len, int x, int y );

		enum class ScrollDirection
		{
			Up = 0,
			Down,
			Left,
			Right
		};

		// Scrolls the framebuffer 1 pixel in the given direction
		void scroll( ScrollDirection direction );

		// A helper function to reverse bits in a byte
		static uint8_t reverseByte(uint8_t b);

	private:
		// Returns a pointer to the byte which contains the specified pixel
		uint8_t* getPixelsBytePtr(int16_t x, int16_t y) const;

		// Transfers a 16-bit word by SPI
		void tranferCommand(uint16_t command);

		const uint8_t m_nsegments;
		SPISettings m_spiSettings;
		uint8_t m_flags = 0;
		uint8_t* m_frameBuffer = nullptr;
		uint8_t m_ssPin;
};

#endif /* LEDMATRIXDRIVER_H_ */
