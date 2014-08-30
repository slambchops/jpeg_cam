#ifndef CAMERA_H
#define CAMERA_H

int init_camera(void);
void close_camera(void);
int capture_frame(char *jpegFileName);

#endif // CAMERA_H
