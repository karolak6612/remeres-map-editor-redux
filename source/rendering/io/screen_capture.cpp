#include "rendering/io/screen_capture.h"
#include "app/main.h"
#include <vector>
#include <cstring>
#include <algorithm>

// glut include removed

void ScreenCapture::Capture(int width, int height, uint8_t* buffer) {
	// No glFinish needed, glReadPixels handles synchronization
	// gl API removed // 1 byte alignment

	// Read the entire buffer at once (much faster than row-by-row)
	// gl API removed

	// OpenGL origin is bottom-left, but we usually expect top-left.
	// Flip the image vertically on CPU.
	int row_size = width * 3;
	std::vector<uint8_t> temp_row(row_size);

	for (int y = 0; y < height / 2; ++y) {
		uint8_t* top_row = buffer + y * row_size;
		uint8_t* bottom_row = buffer + (height - 1 - y) * row_size;

		// Swap rows
		std::swap_ranges(top_row, top_row + row_size, bottom_row);
	}
}
