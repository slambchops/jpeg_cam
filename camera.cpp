#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>

#include "camera.h"
#include "jpeg.h"

struct buf_info {
	unsigned int length;
	char *start;
};

/*
 * V4L2 capture device structure declaration
 */
struct v4l2_device_info {
    int type;
    int fd;
    enum v4l2_memory memory_mode;
    int num_buffers;
    int width;
    int height;
    char dev_name[12];
    char name[10];

    struct v4l2_buffer buf;
    struct v4l2_format fmt;
    struct buf_info *buffers;
} camera;

/*
 * V4L2 capture device initialization
 */
static int v4l2_init_device(struct v4l2_device_info *device)
{
    int ret, i, j;
    struct v4l2_requestbuffers reqbuf;
    struct v4l2_buffer buf;
    struct v4l2_format temp_fmt;
    struct buf_info *temp_buffers;
    struct v4l2_capability capability;

    /* Open the capture device */
    device->fd = open((const char *) device->dev_name, O_RDWR);
    if (device->fd <= 0) {
        printf("Cannot open %s device\n", device->dev_name);
        return -1;
    }

    printf("\n%s: Opened Channel\n", device->name);

    /* Check if the device is capable of streaming */
    if (ioctl(device->fd, VIDIOC_QUERYCAP, &capability) < 0) {
        perror("VIDIOC_QUERYCAP");
        goto ERROR;
    }

    if (capability.capabilities & V4L2_CAP_STREAMING)
        printf("%s: Capable of streaming\n", device->name);
    else {
        printf("%s: Not capable of streaming\n", device->name);
        goto ERROR;
    }

    reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    temp_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    temp_fmt.fmt.pix.width = device->width;
    temp_fmt.fmt.pix.height = device->height;
    temp_fmt.fmt.pix.pixelformat = device->fmt.fmt.pix.pixelformat;

    ret = ioctl(device->fd, VIDIOC_S_FMT, &temp_fmt);
    if (ret < 0) {
        perror("VIDIOC_S_FMT");
        goto ERROR;
    }

    device->fmt = temp_fmt;

    reqbuf.count = device->num_buffers;
    reqbuf.memory = device->memory_mode;
    ret = ioctl(device->fd, VIDIOC_REQBUFS, &reqbuf);
    if (ret < 0) {
        perror("Cannot allocate memory");
        goto ERROR;
    }
    device->num_buffers = reqbuf.count;
    printf("%s: Number of requested buffers = %u\n", device->name,
           device->num_buffers);

    temp_buffers = (struct buf_info *) malloc(sizeof(struct buf_info) *
                          device->num_buffers);
    if (!temp_buffers) {
        printf("Cannot allocate memory\n");
        goto ERROR;
    }

    for (i = 0; i < device->num_buffers; i++) {
        buf.type = reqbuf.type;
        buf.index = i;
        buf.memory = reqbuf.memory;
        ret = ioctl(device->fd, VIDIOC_QUERYBUF, &buf);
        if (ret < 0) {
            perror("VIDIOC_QUERYCAP");
            device->num_buffers = i;
            goto ERROR1;
            return -1;
        }

        temp_buffers[i].length = buf.length;
        temp_buffers[i].start = (char*)mmap(NULL, buf.length,
                     PROT_READ | PROT_WRITE,
                     MAP_SHARED, device->fd,
                     buf.m.offset);
        if (temp_buffers[i].start == MAP_FAILED) {
            printf("Cannot mmap = %d buffer\n", i);
            device->num_buffers = i;
            goto ERROR1;
        }
        printf("temp_buffers[%d].start - %x\n", i,
                (unsigned int)temp_buffers[i].start);
    }

    device->buffers = temp_buffers;

    printf("%s: Init done successfully\n", device->name);
    return 0;

ERROR1:
    for (j = 0; j < device->num_buffers; j++)
        munmap(temp_buffers[j].start,
               temp_buffers[j].length);

    free(temp_buffers);
ERROR:
    close(device->fd);

    return -1;
}

static void v4l2_exit_device(struct v4l2_device_info *device)
{
    int i;

    for (i = 0; i < device->num_buffers; i++) {
        munmap(device->buffers[i].start,
               device->buffers[i].length);
    }

    free(device->buffers);
    close(device->fd);

    return;
}


/*
 * Enable streaming for V4L2 capture device
 */
static int v4l2_stream_on(struct v4l2_device_info *device)
{
    int a, i, ret;

    for (i = 0; i < device->num_buffers; ++i) {
            struct v4l2_buffer buf;

            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.index = i;

            ret = ioctl(device->fd, VIDIOC_QBUF, &buf);
            if (ret < 0) {
                perror("VIDIOC_QBUF");
                device->num_buffers = i;
                return -1;
            }
    }

    device->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    device->buf.index = 0;
    device->buf.memory = device->memory_mode;

    a = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = ioctl(device->fd, VIDIOC_STREAMON, &a);
    if (ret < 0) {
        perror("VIDIOC_STREAMON");
        return -1;
    }
    printf("%s: Stream on\n", device->name);

    return 0;
}

/*
 * Disable streaming for V4L2 capture device
 */
static int v4l2_stream_off(struct v4l2_device_info *device)
{
    int a, ret;

    a = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = ioctl(device->fd, VIDIOC_STREAMOFF, &a);
    if (ret < 0) {
        perror("VIDIOC_STREAMOFF");
        return -1;
    }
    printf("%s: Stream off\n", device->name);

    return 0;
}

/*
 * Queue V4L2 buffer
 */
static int v4l2_queue_buffer(struct v4l2_device_info *device)
{
    int ret;
    fd_set fds;
    struct timeval tv;

    FD_ZERO(&fds);
    FD_SET(device->fd, &fds);

    /* Timeout. */
    tv.tv_sec = 10;
    tv.tv_usec = 0;

    ret = select(device->fd + 1, &fds, NULL, NULL, &tv);

    if (-1 == ret) {
        if (EINTR == errno) {
            perror( "select\n");
            return -1;
        }
    }

    if (0 == ret) {
        perror( "select timeout\n");
        return -1;
    }

    /* Queue buffer for the v4l2 capture device */
    ret = ioctl(device->fd, VIDIOC_QBUF,
            &device->buf);
    if (ret < 0) {
        perror("VIDIOC_QBUF");
        return -1;
    }

    return 0;
}

/*
 * DeQueue V4L2 buffer
 */
static int v4l2_dequeue_buffer(struct v4l2_device_info *device)
{
    int ret;
    fd_set fds;
    struct timeval tv;

    FD_ZERO(&fds);
    FD_SET(device->fd, &fds);

    /* Timeout. */
    tv.tv_sec = 10;
    tv.tv_usec = 0;

    ret = select(device->fd + 1, &fds, NULL, NULL, &tv);

    if (-1 == ret) {
        if (EINTR == errno) {
            perror( "select\n");
            return -1;
        }
    }

    if (0 == ret) {
        perror( "select timeout\n");
        return -1;
    }

    /* Dequeue buffer for the v4l2 capture device */
    ret = ioctl(device->fd, VIDIOC_DQBUF,
            &device->buf);
    if (ret < 0) {
        perror("VIDIOC_DQBUF");
        return -1;
    }

    return 0;
}

/*
 * Initializes camera for streaming
 */
int init_camera(void)
{
    /* Declare properties for camera */
    camera.memory_mode = V4L2_MEMORY_MMAP;
    camera.num_buffers = 3;
    strcpy(camera.dev_name,"/dev/video0");
    strcpy(camera.name,"Camera");
    camera.buffers = NULL;
    camera.fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    camera.width = 640;
    camera.height = 480;

    /* Initialize the v4l2 capture devices */
    if (v4l2_init_device(&camera) < 0) goto Error;

    /* Enable streaming for the v4l2 capture devices */
    if (v4l2_stream_on(&camera) < 0) goto Error;

    return 0;

Error:
    close_camera();
    return -1;
}

/*
 * Closes down the camera
 */
void close_camera(void)
{
    v4l2_stream_off(&camera);
    v4l2_exit_device(&camera);
}

/*
 * Capture v4l2 frame and save to jpeg
 */
int capture_frame(void)
{
	unsigned int index;

    /* Request a capture buffer from the driver that can be copied to framebuffer */
    v4l2_dequeue_buffer(&camera);

	index = camera.buf.index;

    jpegWrite((unsigned char *)camera.buffers[index].start,
		camera.width, camera.height);

    /* Give the buffer back to the driver so it can be filled again */
    v4l2_queue_buffer(&camera);

    return 0;
}
