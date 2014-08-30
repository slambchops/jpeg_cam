#include <malloc.h>
#include <sys/mman.h>
#include <jpeglib.h>

void jpeg_write(char *jpegFilename, unsigned char* yuv, int width, int height)
{
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;

    unsigned char *img = (unsigned char *)malloc(width*height*3*sizeof(char));

    JSAMPROW row_pointer[1];
    FILE *outfile = fopen( jpegFilename, "wb" );

    /* Open file for saving output */
    if (!outfile) {
		printf("Can't open file!\n");
    }

    /* Create jpeg data */
    cinfo.err = jpeg_std_error( &jerr );
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, outfile);

    /* Set image parameters */
    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_YCbCr;

    /* Set jpeg compression parameters to default */
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, 90, TRUE);

    /* Start compression */
    jpeg_start_compress(&cinfo, TRUE);

    /* Feed in data. YUV422 is converted to YUV444 */
    row_pointer[0] = img;
    while (cinfo.next_scanline < cinfo.image_height) {
        unsigned i, j;
        unsigned offset = cinfo.next_scanline * cinfo.image_width * 2;
        for (i = 0, j = 0; i < cinfo.image_width*2; i += 4, j += 6) {
            img[j + 0] = yuv[offset + i + 0]; // Y
            img[j + 1] = yuv[offset + i + 1]; // U
            img[j + 2] = yuv[offset + i + 3]; // V
            img[j + 3] = yuv[offset + i + 2]; // Y
            img[j + 4] = yuv[offset + i + 1]; // U
            img[j + 5] = yuv[offset + i + 3]; // V
        }
        jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    /* Finish compression */
    jpeg_finish_compress(&cinfo);

    /* Destroy jpeg data */
    jpeg_destroy_compress(&cinfo);

    /* Close output file */
    fclose(outfile);

    munmap(img, width*height*3*sizeof(char));

    return;
}
