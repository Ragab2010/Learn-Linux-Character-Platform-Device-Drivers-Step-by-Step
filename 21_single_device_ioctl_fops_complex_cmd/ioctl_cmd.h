#ifndef __IOCTL_CMD_H
#define __IOCTL_CMD_H

#define MSG_MAGIC_NUMBER    0x21

#define MSG_IOCTL_GET_LENGTH    _IOR(MSG_MAGIC_NUMBER, 1, unsigned int)

#define MSG_IOCTL_CLEAR_BUFFER  _IO(MSG_MAGIC_NUMBER, 2)

#define MSG_IOCTL_FILL_BUFFER   _IOW(MSG_MAGIC_NUMBER, 3, unsigned char)

#define MSG_IOCTL_MAX_CMDS      3

#endif
