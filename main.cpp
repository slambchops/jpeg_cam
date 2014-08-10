#include <unistd.h>
#include "camera.h"
#include "jpeg.h"

int main(int argc, char *argv[])
{
    init_camera();
//	while (1) {
    	capture_frame();
		usleep(100000);
//	}
    close_camera();

    return 0;
}
