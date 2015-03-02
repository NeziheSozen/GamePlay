#include "ImageUtils.h"
#include "FilepathUtils.h"
#include <png.h>
#include <zlib.h>
#include <errno.h>


namespace gameplay
{
	int ImageUtils::convertTexture2Png(const std::string& imgSource, const std::string& imgDestinationPng)
	{
#ifdef WIN32
		std::string str_cmd = ".\\convert.exe \"" + imgSource + "\" \"" + imgDestinationPng + "\"";
        return system(str_cmd.c_str());
#else
        std::string escapedSource(imgSource);
        std::string escapedDestination(imgDestinationPng);

        FilepathUtils::escapeFilePath(escapedSource);
        FilepathUtils::escapeFilePath(escapedDestination);

//        std::string str_cmd = "./convert " + escapedSource + " -colorspace sRGB " + escapedDestination;
        std::string str_cmd = "sips -s format png " + escapedSource + " --out " + escapedDestination;
        
        return system(str_cmd.c_str());
#endif
        
	}

	bool ImageUtils::isPngFile(const char* path)
	{
		FILE *fp = fopen(path, "rb");
		if (!fp)
		{
			// GP_WARNING(WARN_TEXTURE_NOT_FOUND, path, strerror(errno));
			return false;
		}
    
		int number = 7;
		Byte* header = (Byte*) malloc(sizeof(Byte)*number);
    
		fread(header, 1, number, fp);
		bool isPng = !png_sig_cmp(header, 0, number);
		fclose(fp);
		return isPng;
	}

	int ImageUtils::isPowerOfTwo(const char* path)
	{
		png_byte header[8];
    
		FILE *fp = fopen(path, "rb");
		if (fp == 0)
		{
			return 0;
		}
    
		// read the header
		fread(header, 1, 8, fp);
    
		if (png_sig_cmp(header, 0, 8))
		{
			fclose(fp);
			return 0;
		}
    
		png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
		if (!png_ptr)
		{
			fclose(fp);
			return 0;
		}
    
		// create png info struct
		png_infop info_ptr = png_create_info_struct(png_ptr);
		if (!info_ptr)
		{
			png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
			fclose(fp);
			return 0;
		}
    
		// create png info struct
		png_infop end_info = png_create_info_struct(png_ptr);
		if (!end_info)
		{
			png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);
			fclose(fp);
			return 0;
		}
    
		// the code in this if statement gets called if libpng encounters an error
		if (setjmp(png_jmpbuf(png_ptr))) {
			png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
			fclose(fp);
			return 0;
		}
    
		// init png reading
		png_init_io(png_ptr, fp);
    
		// let libpng know you already read the first 8 bytes
		png_set_sig_bytes(png_ptr, 8);
    
		// read all the info up to the image data
		png_read_info(png_ptr, info_ptr);
    
		// variables to pass to get info
		int bit_depth, color_type;
		png_uint_32 temp_width, temp_height;
    
		// get info about png
		png_get_IHDR(png_ptr, info_ptr, &temp_width, &temp_height, &bit_depth, &color_type,
					 NULL, NULL, NULL);
    
		return (ImageUtils::isPowerOfTwo(temp_width) && ImageUtils::isPowerOfTwo(temp_height));
	}

	int ImageUtils::isPowerOfTwo(unsigned int x)
	{
		return ((x != 0) && !(x & (x - 1)));
	}
}
