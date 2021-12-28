#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
#include <assert.h>

#include <jasper/jasper.h>

int main(int argc, char **argv)
{
  int fmtid; // recognized image
	int id;
	char *infile;
	jas_stream_t *instream;
	jas_image_t *image;
	int width;
	int height;
	int depth;
	int numcmpts;
	char *fmtname;
	FILE* fout;
	int x, y;
	char rgb[3];
	int cmptlut[3];

	if (jas_init()) {
		return;
	}

	infile = argv[1];

	if (!(instream = jas_stream_fopen(infile, "rb"))) {
	  fprintf(stderr, "cannot open input image file %s\n", infile);
	  exit(EXIT_FAILURE);
	} 

	if ((fmtid = jas_image_getfmt(instream)) < 0) {
		fprintf(stderr, "unknown image format\n");
	}

	/* Decode the image. */
	if (!(image = jas_image_decode(instream, fmtid, 0))) {
		fprintf(stderr, "cannot load image\n");
		return EXIT_FAILURE;
	}

	/* Close the image file. */
	jas_stream_close(instream);

	numcmpts = jas_image_numcmpts(image);
	width = jas_image_cmptwidth(image, 0);
	height = jas_image_cmptheight(image, 0);
	depth = jas_image_cmptprec(image, 0);
	if (!(fmtname = jas_image_fmttostr(fmtid))) {
		abort();
	}
	printf("%s %d %d %d %d %ld\n", fmtname, numcmpts, width, height, depth, (long) jas_image_rawsize(image));

	// convert to PPM
	if ((cmptlut[0] = jas_image_getcmptbytype(image,
	  JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_RGB_R))) < 0 ||
	  (cmptlut[1] = jas_image_getcmptbytype(image,
	  JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_RGB_G))) < 0 ||
	  (cmptlut[2] = jas_image_getcmptbytype(image,
	  JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_RGB_B))) < 0)
		goto error;

	fout = fopen("toto.ppm", "wb");
	fprintf(fout, "P6\n");
	fprintf(fout, "%d %d\n",  width, height);
	fprintf(fout, "255\n");
	for (y = 0; y < height; ++y) {
	  for (x = 0; x < width; ++x) {
	    rgb[0] = jas_image_readcmptsample(image, 0, x, y);
	    rgb[1] = jas_image_readcmptsample(image, 1, x, y);;
	    rgb[2] = jas_image_readcmptsample(image, 2, x, y);;
	    fwrite(rgb, 3, 1, fout);
	  }
	}
	fclose(fout);
 error:
	jas_image_destroy(image);
	jas_image_clearfmts();

	return EXIT_SUCCESS;
}

