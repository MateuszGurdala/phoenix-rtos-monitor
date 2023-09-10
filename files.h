#ifndef _FILES_H
#define _FILES_H

#define O_RDONLY 0x0001
#define O_WRONLY 0x0002
#define O_RDWR   0x0004
#define O_APPEND 0x0008
#define O_CREAT  0x0100
#define O_TRUNC  0x0200

#define FILE_MAXSIZE 1024

#define DIRPATH     "/monitor"
#define DIRPATH_LEN 9
#define COMMAND     "nc 10.0.2.2:3713 <"
#define COMMAND_LEN 20

extern int _monitor_file_init();

extern int ondemand_write(void *buffer, unsigned ebuff, int size);

extern int ondemand_read(char *file_name);

#endif