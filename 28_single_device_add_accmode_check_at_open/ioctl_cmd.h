#ifndef __IOCTL_CMD_H
#define __IOCTL_CMD_H

#define SIG_MAGIC_NUMBER    0x21

#define SIG_IOCTL_SET_PID    _IOW(SIG_MAGIC_NUMBER, 1, unsigned int)

#define SIG_IOCTL_SET_SIGNAL  _IOW(SIG_MAGIC_NUMBER, 2, unsigned int)

#define SIG_IOCTL_SEND_SIGNAL   _IO(SIG_MAGIC_NUMBER, 3)


#define SIG_IOCTL_MAX_CMDS      3

#endif
