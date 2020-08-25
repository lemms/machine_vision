#include <cstdio>
#include <functional>
#include <iostream>

extern "C"
{
	#include <png.h>
}

int read_png_file(
	const char* filename,
	int& width,
	int& height,
	png_byte& color_type,
	png_byte& bit_depth,
	int& number_of_passes,
	int& stride,
	png_bytep*& row_pointers)
{
	// open file and test for it being a png
	FILE *fp = fopen(filename, "rb");
	if (!fp) {
		std::cerr << "[read_png_file] File " << filename << " could not be opened for reading" << std::endl;
		return 1;
	}
	char header[8];
	fread(header, 1, 8, fp);
	if (png_sig_cmp(reinterpret_cast<unsigned char*>(header), 0, 8)) {
		std::cerr << "[read_png_file] File " << filename << " is not recognized as a PNG file" << std::endl;
		return 2;
	}

	// initialize stuff
	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if (!png_ptr) {
		std::cerr << "[read_png_file] png_create_read_struct failed" << std::endl;
		return 4;
	}

	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
	{
		std::cerr << "[read_png_file] png_create_info_struct failed" << std::endl;
		return 5;
	}

	if (setjmp(png_jmpbuf(png_ptr)))
	{
		std::cerr << "[read_png_file] Error during init_io" << std::endl;
		return 6;
	}


	png_init_io(png_ptr, fp);
	png_set_sig_bytes(png_ptr, 8);

	png_read_info(png_ptr, info_ptr);

	width = png_get_image_width(png_ptr, info_ptr);
	height = png_get_image_height(png_ptr, info_ptr);
	color_type = png_get_color_type(png_ptr, info_ptr);
	bit_depth = png_get_bit_depth(png_ptr, info_ptr);

	number_of_passes = png_set_interlace_handling(png_ptr);
	png_read_update_info(png_ptr, info_ptr);

	// read file
	if (setjmp(png_jmpbuf(png_ptr)))
	{
		std::cerr << "[read_png_file] Error during read_png_file" << std::endl;
	}

	row_pointers = reinterpret_cast<png_bytep*>(malloc(sizeof(png_bytep) * height));
	for (int y = 0; y < height; ++y)
		row_pointers[y] = reinterpret_cast<png_byte*>(malloc(png_get_rowbytes(png_ptr,info_ptr)));

	png_read_image(png_ptr, row_pointers);

	// print info
	if (png_get_color_type(png_ptr, info_ptr) == PNG_COLOR_TYPE_PALETTE)
	{
		std::cout << "Palette image detected" << std::endl;
		stride = 1;
	}
	else if (png_get_color_type(png_ptr, info_ptr) == PNG_COLOR_TYPE_RGB)
	{
		std::cout << "RGB image detected" << std::endl;
		stride = 3;
	}
	else if (png_get_color_type(png_ptr, info_ptr) == PNG_COLOR_TYPE_RGBA)
	{
		std::cout << "RGBA image detected" << std::endl;
		stride = 4;
	}
	else if (png_get_color_type(png_ptr, info_ptr) == PNG_COLOR_TYPE_GRAY_ALPHA)
	{
		std::cout << "Gray alpha image detected" << std::endl;
		stride = 2;
	}
	else if (png_get_color_type(png_ptr, info_ptr) == PNG_COLOR_TYPE_GRAY)
	{
		std::cout << "Gray image detected" << std::endl;
		stride = 1;
	}

	fclose(fp);


	return 0;
}

int write_png_file(
	const char* file_name,
	int& width,
	int& height,
	png_byte& color_type,
	png_byte& bit_depth,
	png_bytep*& row_pointers)
{
	// create file
	FILE *fp = fopen(file_name, "wb");
	if (!fp) {
		std::cerr << "[write_png_file] File " << file_name << " could not be opened for writing" << std::endl;
		return 1;
	}

	// initialize stuff
	png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if (!png_ptr)
	{
		std::cerr << "[write_png_file] png_create_write_struct failed" << std::endl;
		return 2;
	}

	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
	{
		std::cerr << "[write_png_file] png_create_info_struct failed" << std::endl;
		return 3;
	}

	if (setjmp(png_jmpbuf(png_ptr)))
	{
		std::cerr << "[write_png_file] Error during init_io" << std::endl;
		return 4;
	}

	png_init_io(png_ptr, fp);


	// write header
	if (setjmp(png_jmpbuf(png_ptr)))
	{
		std::cerr << "[write_png_file] Error during writing header" << std::endl;
		return 5;
	}

	png_set_IHDR(png_ptr, info_ptr, width, height,
			bit_depth, color_type, PNG_INTERLACE_NONE,
			PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

	png_write_info(png_ptr, info_ptr);


	// write bytes
	if (setjmp(png_jmpbuf(png_ptr)))
	{
		std::cerr << "[write_png_file] Error during writing bytes" << std::endl;
		return 6;
	}

	png_write_image(png_ptr, row_pointers);


	// end write
	if (setjmp(png_jmpbuf(png_ptr)))
	{
		std::cerr << "[write_png_file] Error during end of write" << std::endl;
		return 7;
	}

	png_write_end(png_ptr, NULL);

	// cleanup heap allocation
	for (int y = 0; y < height; ++y)
		free(row_pointers[y]);
	free(row_pointers);

	fclose(fp);
	return 0;
}

void print_image(png_bytep* row_pointers, int width, int height, int stride) {
	for (int y = 0; y < height; ++y) {
		png_byte* row = row_pointers[y];
		for (int x = 0; x < width; ++x) {
			png_byte* ptr = &row[x*stride];

			std::cout << "[";
			for (int offset = 0; offset < stride; ++offset) {
				std::cout << static_cast<int>(ptr[offset]) << " ";
			}
			std::cout << "], ";
		}
	}
}

int grayscale_copy(int lhs, int rhs) {
	return rhs;
}

int grayscale_add(int lhs, int rhs) {
	return lhs + rhs;
}

int grayscale_average(int lhs, int rhs) {
	return (lhs + rhs) / 2;
}

int grayscale_learn_average(int lhs, int rhs) {
	return (lhs * 16 + rhs + 8) / 16;
}

int grayscale_set(int value, int alpha) {
	return alpha;
}

int grayscale_brighten(int value, int beta) {
	return value + beta;
}

int grayscale_stretch(int value, float gamma, int beta) {
	return static_cast<int>(static_cast<float>(value) * gamma) + beta;
}

int grayscale_invert(int value) {
	return 255 - value;
}

int grayscale_threshold(int value, int threshold) {
	return value < threshold ? 0 : 1;
}

int grayscale_invert_threshold(int value, int threshold) {
	return value < threshold ? 1 : 0;
}

int bit_display(int value) {
	return value > 0 ? 255 : 0;
}

int bit_invert_display(int value) {
	return value <= 0 ? 255 : 0;
}

int bit_not(int value) {
	return !value;
}

int bit_invert(int value) {
	return 1 - value;
}

int bit_and(int lhs, int rhs) {
	return lhs && rhs;
}

void single_channel_apply(std::function<int(int)> fn, png_bytep* row_pointers, int width, int height) {
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			row_pointers[y][x] = std::max(0, std::min(255, fn(row_pointers[y][x])));
		}
	}
}

void single_channel_apply(std::function<int(int, int)> fn, png_bytep* lhs_row_pointers, png_bytep* rhs_row_pointers, int width, int height) {
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			lhs_row_pointers[y][x] = std::max(0, std::min(255, fn(lhs_row_pointers[y][x], rhs_row_pointers[y][x])));
		}
	}
}

void shrink(png_bytep* out_row_pointers, png_bytep* row_pointers, int x_in, int y_in, int width, int height) {
	if (x_in - 1 < 0) {
		out_row_pointers[y_in][x_in] = row_pointers[y_in][x_in];
		return;
	}
	if (y_in - 1 < 0) {
		out_row_pointers[y_in][x_in] = row_pointers[y_in][x_in];
		return;
	}
	if (x_in + 1 > width - 1) {
		out_row_pointers[y_in][x_in] = row_pointers[y_in][x_in];
		return;
	}
	if (y_in + 1 > height - 1) {
		out_row_pointers[y_in][x_in] = row_pointers[y_in][x_in];
		return;
	}
	int sigma = row_pointers[y_in - 1][x_in - 1] +
	            row_pointers[y_in - 1][x_in]  +
	            row_pointers[y_in - 1][x_in + 1] +
	            row_pointers[y_in][x_in - 1] +
	            row_pointers[y_in][x_in + 1] +
	            row_pointers[y_in + 1][x_in - 1] +
	            row_pointers[y_in + 1][x_in] +
	            row_pointers[y_in + 1][x_in + 1];
	if (sigma < 8) {
		out_row_pointers[y_in][x_in] = 0;
	} else {
		out_row_pointers[y_in][x_in] = row_pointers[y_in][x_in];
	}
}

void expand(png_bytep* out_row_pointers, png_bytep* row_pointers, int x_in, int y_in, int width, int height) {
	if (x_in - 1 < 0) {
		out_row_pointers[y_in][x_in] = row_pointers[y_in][x_in];
		return;
	}
	if (y_in - 1 < 0) {
		out_row_pointers[y_in][x_in] = row_pointers[y_in][x_in];
		return;
	}
	if (x_in + 1 > width - 1) {
		out_row_pointers[y_in][x_in] = row_pointers[y_in][x_in];
		return;
	}
	if (y_in + 1 > height - 1) {
		out_row_pointers[y_in][x_in] = row_pointers[y_in][x_in];
		return;
	}
	int sigma = row_pointers[y_in - 1][x_in - 1] +
	            row_pointers[y_in - 1][x_in]  +
	            row_pointers[y_in - 1][x_in + 1] +
	            row_pointers[y_in][x_in - 1] +
	            row_pointers[y_in][x_in + 1] +
	            row_pointers[y_in + 1][x_in - 1] +
	            row_pointers[y_in + 1][x_in] +
	            row_pointers[y_in + 1][x_in + 1];
	if (sigma > 0) {
		out_row_pointers[y_in][x_in] = 1;
	} else {
		out_row_pointers[y_in][x_in] = row_pointers[y_in][x_in];
	}
}

void edge(png_bytep* out_row_pointers, png_bytep* row_pointers, int x_in, int y_in, int width, int height) {
	if (x_in - 1 < 0) {
		out_row_pointers[y_in][x_in] = 0;
		return;
	}
	if (y_in - 1 < 0) {
		out_row_pointers[y_in][x_in] = 0;
		return;
	}
	if (x_in + 1 > width - 1) {
		out_row_pointers[y_in][x_in] = 0;
		return;
	}
	if (y_in + 1 > height - 1) {
		out_row_pointers[y_in][x_in] = 0;
		return;
	}
	int sigma = row_pointers[y_in - 1][x_in - 1] +
	            row_pointers[y_in - 1][x_in]  +
	            row_pointers[y_in - 1][x_in + 1] +
	            row_pointers[y_in][x_in - 1] +
	            row_pointers[y_in][x_in + 1] +
	            row_pointers[y_in + 1][x_in - 1] +
	            row_pointers[y_in + 1][x_in] +
	            row_pointers[y_in + 1][x_in + 1];
	if (sigma == 8) {
		out_row_pointers[y_in][x_in] = 0;
	} else {
		out_row_pointers[y_in][x_in] = row_pointers[y_in][x_in];
	}
}

void salt(png_bytep* out_row_pointers, png_bytep* row_pointers, int x_in, int y_in, int width, int height) {
	if (x_in - 1 < 0) {
		out_row_pointers[y_in][x_in] = row_pointers[y_in][x_in];
		return;
	}
	if (y_in - 1 < 0) {
		out_row_pointers[y_in][x_in] = row_pointers[y_in][x_in];
		return;
	}
	if (x_in + 1 > width - 1) {
		out_row_pointers[y_in][x_in] = row_pointers[y_in][x_in];
		return;
	}
	if (y_in + 1 > height - 1) {
		out_row_pointers[y_in][x_in] = row_pointers[y_in][x_in];
		return;
	}
	int sigma = row_pointers[y_in - 1][x_in - 1] +
	            row_pointers[y_in - 1][x_in]  +
	            row_pointers[y_in - 1][x_in + 1] +
	            row_pointers[y_in][x_in - 1] +
	            row_pointers[y_in][x_in + 1] +
	            row_pointers[y_in + 1][x_in - 1] +
	            row_pointers[y_in + 1][x_in] +
	            row_pointers[y_in + 1][x_in + 1];
	if (sigma == 8) {
		out_row_pointers[y_in][x_in] = 1;
	} else {
		out_row_pointers[y_in][x_in] = row_pointers[y_in][x_in];
	}
}

void pepper(png_bytep* out_row_pointers, png_bytep* row_pointers, int x_in, int y_in, int width, int height) {
	if (x_in - 1 < 0) {
		out_row_pointers[y_in][x_in] = row_pointers[y_in][x_in];
		return;
	}
	if (y_in - 1 < 0) {
		out_row_pointers[y_in][x_in] = row_pointers[y_in][x_in];
		return;
	}
	if (x_in + 1 > width - 1) {
		out_row_pointers[y_in][x_in] = row_pointers[y_in][x_in];
		return;
	}
	if (y_in + 1 > height - 1) {
		out_row_pointers[y_in][x_in] = row_pointers[y_in][x_in];
		return;
	}
	int sigma = row_pointers[y_in - 1][x_in - 1] +
	            row_pointers[y_in - 1][x_in]  +
	            row_pointers[y_in - 1][x_in + 1] +
	            row_pointers[y_in][x_in - 1] +
	            row_pointers[y_in][x_in + 1] +
	            row_pointers[y_in + 1][x_in - 1] +
	            row_pointers[y_in + 1][x_in] +
	            row_pointers[y_in + 1][x_in + 1];
	if (sigma == 8) {
		out_row_pointers[y_in][x_in] = 0;
	} else {
		out_row_pointers[y_in][x_in] = row_pointers[y_in][x_in];
	}
}

void noise(png_bytep* out_row_pointers, png_bytep* row_pointers, int x_in, int y_in, int width, int height) {
	if (x_in - 1 < 0) {
		out_row_pointers[y_in][x_in] = row_pointers[y_in][x_in];
		return;
	}
	if (y_in - 1 < 0) {
		out_row_pointers[y_in][x_in] = row_pointers[y_in][x_in];
		return;
	}
	if (x_in + 1 > width - 1) {
		out_row_pointers[y_in][x_in] = row_pointers[y_in][x_in];
		return;
	}
	if (y_in + 1 > height - 1) {
		out_row_pointers[y_in][x_in] = row_pointers[y_in][x_in];
		return;
	}
	int sigma = row_pointers[y_in - 1][x_in - 1] +
	            row_pointers[y_in - 1][x_in]  +
	            row_pointers[y_in - 1][x_in + 1] +
	            row_pointers[y_in][x_in - 1] +
	            row_pointers[y_in][x_in + 1] +
	            row_pointers[y_in + 1][x_in - 1] +
	            row_pointers[y_in + 1][x_in] +
	            row_pointers[y_in + 1][x_in + 1];
	if (sigma == 0) {
		out_row_pointers[y_in][x_in] = 0;
	} else if (sigma == 8) {
		out_row_pointers[y_in][x_in] = 1;
	} else {
		out_row_pointers[y_in][x_in] = row_pointers[y_in][x_in];
	}
}

void noize(png_bytep* out_row_pointers, png_bytep* row_pointers, int x_in, int y_in, int width, int height) {
	if (x_in - 1 < 0) {
		out_row_pointers[y_in][x_in] = row_pointers[y_in][x_in];
		return;
	}
	if (y_in - 1 < 0) {
		out_row_pointers[y_in][x_in] = row_pointers[y_in][x_in];
		return;
	}
	if (x_in + 1 > width - 1) {
		out_row_pointers[y_in][x_in] = row_pointers[y_in][x_in];
		return;
	}
	if (y_in + 1 > height - 1) {
		out_row_pointers[y_in][x_in] = row_pointers[y_in][x_in];
		return;
	}
	int sigma = row_pointers[y_in - 1][x_in - 1] +
	            row_pointers[y_in - 1][x_in]  +
	            row_pointers[y_in - 1][x_in + 1] +
	            row_pointers[y_in][x_in - 1] +
	            row_pointers[y_in][x_in + 1] +
	            row_pointers[y_in + 1][x_in - 1] +
	            row_pointers[y_in + 1][x_in] +
	            row_pointers[y_in + 1][x_in + 1];
	if (sigma < 2) {
		out_row_pointers[y_in][x_in] = 0;
	} else if (sigma > 6) {
		out_row_pointers[y_in][x_in] = 1;
	} else {
		out_row_pointers[y_in][x_in] = row_pointers[y_in][x_in];
	}
}

void grayscale_convolve(png_bytep* out_row_pointers, png_bytep* row_pointers, int x_in, int y_in, int width, int height, const float* kernel) {
	if (x_in - 1 < 0) {
		out_row_pointers[y_in][x_in] = row_pointers[y_in][x_in];
		return;
	}
	if (y_in - 1 < 0) {
		out_row_pointers[y_in][x_in] = row_pointers[y_in][x_in];
		return;
	}
	if (x_in + 1 > width - 1) {
		out_row_pointers[y_in][x_in] = row_pointers[y_in][x_in];
		return;
	}
	if (y_in + 1 > height - 1) {
		out_row_pointers[y_in][x_in] = row_pointers[y_in][x_in];
		return;
	}
	int kernel_index = 0;
	float value = 0.0f;
	for (int y = y_in - 1; y < y_in + 2; ++y) {
		for (int x = x_in - 1; x < x_in + 2; ++x) {
			 value += static_cast<float>(row_pointers[y][x]) * kernel[kernel_index++];
		}
	}
	out_row_pointers[y_in][x_in] = static_cast<int>(value);
}

void single_channel_kernel(std::function<void(png_bytep*, png_bytep*, int, int, int, int)> fn, png_bytep* out_row_pointers, png_bytep* row_pointers, int width, int height) {
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			fn(out_row_pointers, row_pointers, x, y, width, height);
		}
	}
}


void swap(png_bytep*& lhs_row_pointers, png_bytep*& rhs_row_pointers) {
	png_bytep* temp_row_pointers = lhs_row_pointers;
	lhs_row_pointers = rhs_row_pointers;
	rhs_row_pointers = temp_row_pointers;
}

int main(int argc, char** argv) {
	if (argc < 2) {
		std::cout << "Invalid number of arguments" << std::endl;
		return 1;
	}
	std::cout << "Compiled with libpng " << PNG_LIBPNG_VER_STRING << "; using libpng " << png_libpng_ver << std::endl;

	std::cout << "Loading: " << argv[1] << std::endl;
	
	int width = 0;
	int height = 0;
	png_byte color_type;
	png_byte bit_depth;
	int number_of_passes = 0;
	int stride = 0;
	png_bytep* row_pointers = nullptr;

	if (read_png_file(argv[1], width, height, color_type, bit_depth, number_of_passes, stride, row_pointers) != 0) {
		return 1;
	}

	std::cout << "Width: " << width << std::endl;
	std::cout << "Height: " << height << std::endl;
	std::cout << "Color type: " << static_cast<int>(color_type) << std::endl;
	std::cout << "Bit depth: " << static_cast<int>(bit_depth) << std::endl;
	std::cout << "Number of passes: " << number_of_passes << std::endl;
	std::cout << "Stride: " << stride << std::endl;

	// Convert to grayscale
	png_byte out_color_type = PNG_COLOR_TYPE_GRAY;
	png_bytep* out_row_pointers = reinterpret_cast<png_bytep*>(malloc(sizeof(png_bytep) * height));
	for (int y = 0; y < height; ++y) {
		out_row_pointers[y] = reinterpret_cast<png_byte*>(malloc(sizeof(png_byte) * width));
		for (int x = 0; x < width; ++x) {
			float value = 0.0f;
			for (int offset = 0; offset < stride; ++offset) {
				value += static_cast<float>(row_pointers[y][x * stride + offset]);
			}
			value /= stride;
			out_row_pointers[y][x] = static_cast<int>(value);
		}
	}

	for (int y = 0; y < height; ++y)
		free(row_pointers[y]);
	free(row_pointers);

	png_bytep* other_row_pointers = reinterpret_cast<png_bytep*>(malloc(sizeof(png_bytep) * height));
	for (int y = 0; y < height; ++y) {
		other_row_pointers[y] = reinterpret_cast<png_byte*>(malloc(sizeof(png_byte) * width));
	}
	single_channel_apply(grayscale_copy, other_row_pointers, out_row_pointers, width, height);


	// Do stuff with the image

	//print_image(row_pointers, width, height, stride);
	//single_channel_apply([](int value){return grayscale_set(value, 100);}, out_row_pointers, width, height);
	//single_channel_apply([](int value){return grayscale_brighten(value, 100);}, out_row_pointers, width, height);
	//single_channel_apply([](int value){return grayscale_stretch(value, 5, -100);}, out_row_pointers, width, height);
	//single_channel_apply(grayscale_invert, out_row_pointers, width, height);
	/*
	{
		single_channel_apply([](int value){return grayscale_threshold(value, 100);}, out_row_pointers, width, height);
		single_channel_apply(bit_display, out_row_pointers, width, height);
	}
	*/
	/*
	{
		single_channel_apply([](int value){return grayscale_threshold(value, 100);}, out_row_pointers, width, height);
		single_channel_kernel(shrink, other_row_pointers, out_row_pointers, width, height);
		swap(other_row_pointers, out_row_pointers);
		single_channel_kernel(shrink, other_row_pointers, out_row_pointers, width, height);
		swap(other_row_pointers, out_row_pointers);
		single_channel_kernel(shrink, other_row_pointers, out_row_pointers, width, height);
		single_channel_apply(bit_display, other_row_pointers, width, height);
		swap(other_row_pointers, out_row_pointers);
	}
	*/
	/*
	{
		single_channel_apply([](int value){return grayscale_threshold(value, 100);}, out_row_pointers, width, height);
		single_channel_kernel(expand, other_row_pointers, out_row_pointers, width, height);
		swap(other_row_pointers, out_row_pointers);
		single_channel_kernel(expand, other_row_pointers, out_row_pointers, width, height);
		swap(other_row_pointers, out_row_pointers);
		single_channel_kernel(expand, other_row_pointers, out_row_pointers, width, height);
		single_channel_apply(bit_display, other_row_pointers, width, height);
		swap(other_row_pointers, out_row_pointers);
	}
	*/
	/*
	{
		single_channel_apply([](int value){return grayscale_threshold(value, 100);}, out_row_pointers, width, height);
		single_channel_kernel(edge, other_row_pointers, out_row_pointers, width, height);
		swap(other_row_pointers, out_row_pointers);
		single_channel_apply(bit_display, out_row_pointers, width, height);
	}
	*/
	/*
	{
		single_channel_apply([](int value){return grayscale_threshold(value, 100);}, out_row_pointers, width, height);
		single_channel_kernel(noize, other_row_pointers, out_row_pointers, width, height);
		swap(other_row_pointers, out_row_pointers);
		single_channel_apply(bit_display, out_row_pointers, width, height);
	}
	*/
	/*
	{
		single_channel_apply([](int value){return grayscale_threshold(value, 100);}, out_row_pointers, width, height);
		single_channel_apply(bit_and, other_row_pointers, out_row_pointers, width, height);
		swap(other_row_pointers, out_row_pointers);
		single_channel_apply(bit_display, out_row_pointers, width, height);
	}
	*/
	/*
	{
		float kernel[9] = {1/9.0, 1/9.0, 1/9.0, 1/9.0, 1/9.0, 1/9.0, 1/9.0, 1/9.0, 1/9.0};
		single_channel_kernel([&](png_bytep* lhs, png_bytep* rhs, int x_in, int y_in, int width, int height){return grayscale_convolve(lhs, rhs, x_in, y_in, width, height, kernel);}, other_row_pointers, out_row_pointers, width, height);
		swap(other_row_pointers, out_row_pointers);
		single_channel_kernel([&](png_bytep* lhs, png_bytep* rhs, int x_in, int y_in, int width, int height){return grayscale_convolve(lhs, rhs, x_in, y_in, width, height, kernel);}, other_row_pointers, out_row_pointers, width, height);
		swap(other_row_pointers, out_row_pointers);
		single_channel_kernel([&](png_bytep* lhs, png_bytep* rhs, int x_in, int y_in, int width, int height){return grayscale_convolve(lhs, rhs, x_in, y_in, width, height, kernel);}, other_row_pointers, out_row_pointers, width, height);
		swap(other_row_pointers, out_row_pointers);
	}
	*/

	for (int y = 0; y < height; ++y)
		free(other_row_pointers[y]);
	free(other_row_pointers);
	
	
	if (argc > 2) {
		std::cout << "Writing output to " << argv[2] << std::endl;
		if (write_png_file(argv[2], width, height, out_color_type, bit_depth, out_row_pointers) != 0) {
			return 2;
		}
		
	}

	return 0;
}
