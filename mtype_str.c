#include <sys/msg.h>
#include "mtype_str.h"

/*
	To add own mtype_to_str function:
	I. Create new function below with two parameters: char** attr1 and m_data* attr2 and with return type of int
		- char** is a reference to pointer that will contain final string
		- m_data* is a pointer to m_data struct
		- naming convention: [mtype]_to_str
	II. Allocate enough memory at *attr1 and write a string in it
	III. Make function return size of the string
	IV. Add function to switch-case in mtype_to_str() 
*/

int msg_to_str(char **write_data, m_data *mdata)
{
	*write_data = malloc(sizeof(mdata->data.msg));
	sprintf(*write_data, "%s", mdata->data.msg);

	return sizeof(mdata->data.msg);
}

int scheduleinfo_to_str(char **write_data, m_data *mdata)
{
	int data_length = 4 * sizeof(unsigned long) + 3 * sizeof(char) + 1;
	*write_data = malloc(data_length);
	sprintf(*write_data, "%lu|%lu-%lu|%lu", mdata->data.schedule_info.pid,
		mdata->data.schedule_info.tid,
		mdata->data.schedule_info.npid,
		mdata->data.schedule_info.ntid);
	return data_length;
}

int mtype_to_str(char **write_data, m_data *mdata)
{
	int data_length = 0;
	switch (mdata->mtype) {
		case mdt_msg:
			data_length = msg_to_str(write_data, mdata);
			break;
		case mdt_scheduleinfo:
			data_length = scheduleinfo_to_str(write_data, mdata);
			break;
		default:
			break;
	}
	return data_length;
}

int mdata_to_str(char **write_data, m_data *mdata)
{
	char *mtype_data = NULL;
	int mtype_size;
	int write_length;

	mtype_size = mtype_to_str(&mtype_data, mdata);

	write_length = mtype_size + sizeof(unsigned) + sizeof(unsigned long long) + sizeof(char) * 3 + 1;
	*write_data = malloc(sizeof(char) * write_length);
	strncpy(*write_data, "", write_length);  // Ensure that allocating too much data will not affect the result
	sprintf(*write_data, "%llu,%u,%s\n", mdata->timestamp, mdata->mtype, mtype_data);

	if (mtype_data != NULL) {
		free(mtype_data);
	}

	return write_length;
}