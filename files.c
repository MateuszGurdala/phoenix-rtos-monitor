#include <stdlib.h>
#include <sys/msg.h>
#include <phoenix/monitor.h>
#include <phoenix/errno.h>
#include "files.h"
#include "mtype_str.h"
#include "buffer_files_conf.h"

char *_get_buffer_file(unsigned ebuff)
{
	switch (ebuff) {
#define BUFFILE(BUFFER_NAME, FILE) \
	case mbuff_##BUFFER_NAME: return FILE;
		BUFFER_FILES_CONF()
#undef BUFFILE
		default: return NULL;
	}
}
char *_get_file_path(char *file_name)
{
	char *file_path = NULL;

	// Alloc memory space and concat file path
	if ((file_path = malloc(sizeof(char) * (DIRPATH_LEN + FILE_MAX_LENGTH + 1))) == NULL) {  // +1 for /
		return NULL;
	}

	sprintf(file_path, "%s/%s", DIRPATH, file_name);

	return file_path;
}

int ondemand_write(void *buffer, unsigned ebuff, int size)
{
	// MG: Support for multithreading
	int fildes;
	int data_length = 0;
	char *file_path = NULL;
	char *write_data = NULL;

	if ((file_path = _get_file_path(_get_buffer_file(ebuff))) == NULL) {
		return -ENOMEM;
	}

	// If file exists open it in APPEND mode, otherwise create it
	if ((fildes = sys_open(file_path, O_APPEND)) < 0) {
		sys_close(sys_open(file_path, O_CREAT));
		if ((fildes = sys_open(file_path, O_APPEND)) < 0) {
			return fildes;
		}
	}

	for (size_t i = 0; i < size; ++i) {
		data_length = mdata_to_str(&write_data, &((m_data *)buffer)[i]);
		if (write_data != NULL && data_length > 0) {
			sys_write(fildes, write_data, data_length);
			free(write_data);
		}
		else {
			sys_close(fildes);
			free(file_path);
			return -ENOWRDT;
		}
	}

	// Cleanup
	sys_close(fildes);
	free(file_path);

	return EOK;
}

int ondemand_read(char *file_name)
{
	char *file_path = NULL;
	char *command = NULL;

	if ((file_path = _get_file_path(file_name)) == NULL) {
		return -ENOMEM;
	}

	// Create command string
	command = malloc(sizeof(char) * (DIRPATH_LEN + FILE_MAX_LENGTH + COMMAND_LEN + 1));
	if (command == NULL) {
		return -ENOMEM;
	}
	sprintf(command, "%s %s", COMMAND, file_path);

	// Run netcat file transfer
	system(command);

	// Cleanup
	free(command);
	free(file_path);

	return EOK;
}

int _monitor_file_init()
{
	// Create directory for all monitoring data files
	mkdir(DIRPATH, 0777);
	return EOK;
}