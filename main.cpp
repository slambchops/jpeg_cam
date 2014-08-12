#include <unistd.h>
#include <stdio.h>
#include "camera.h"
#include "jpeg.h"

int main(int argc, char *argv[])
{
	char temp[] = "image.jpg";
	char out[] = "camera.jpg";

    init_camera();
	while (1) {
		capture_frame();
		rename(temp, out);
		sleep(3);
	}
    close_camera();

    return 0;
}
