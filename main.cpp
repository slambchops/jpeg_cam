#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "camera.h"

int main(int argc, char *argv[])
{
	char out[32];	

	if (argv[1] == '\0')
		strcpy(out, "image.jpg");
	else
		strcpy(out, argv[1]);

	init_camera();

	capture_frame(out);
	
	close_camera();

	return 0;
}
