#define TINYEXR_USE_MINIZ 0
#define TINYEXR_USE_STB_ZLIB 1
#define TINYEXR_IMPLEMENTATION
#include <tinyexr.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <iostream>

int 
main(int argc, char* argv[]) {
	int width, height;
	float* image;
    const char* error;

	int ret = LoadEXR(&image, &width, &height, argv[1], &error);
	if (ret == TINYEXR_SUCCESS) {
 	    free(image);
    } else {
        std::cout << error << std::endl;
        FreeEXRErrorMessage(error);
    }
    return 0;
}