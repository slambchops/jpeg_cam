#include <stdio.h>
#include <unistd.h>
#include "camera.h"

int main(int argc, char *argv[])
{
	char out[] = "camera.jpg";

	init_camera();

	capture_frame(out);
	
	close_camera();

	return 0;
}
