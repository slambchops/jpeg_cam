#include <unistd.h>
#include "camera.h"
#include "jpeg.h"

int main(int argc, char *argv[])
{
    init_camera();
    capture_frame();
    close_camera();

    return 0;
}
