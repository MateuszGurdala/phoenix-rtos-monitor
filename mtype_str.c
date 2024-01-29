#include <sys/msg.h>
#include "mtype_str.h"

/*
	To add own mtype_to_str function:
	I. Create new function below with two parameters: char** attr1 and m_data* attr2 and with return type of int
		- char** is a reference to pointer that will contain final string
		- m_data* is a pointer to m_data struct
		- naming convention: [mtype]_to_str
	II. Make function return size of the string
	III. Add function to switch-case in mtype_to_str()
*/

int msg_to_str(char *write_data, m_data *mdata)
{
	sprintf(write_data, "%s", mdata->data.msg);

	return sizeof(mdata->data.msg);
}

int scheduleinfo_to_str(char *write_data, m_data *mdata)
{
	sprintf(write_data, "%lu,%lu,%lu,%lu",
		mdata->data.schedule_info.pid,
		mdata->data.schedule_info.tid,
		mdata->data.schedule_info.npid,
		mdata->data.schedule_info.ntid);

	return 4 * sizeof(unsigned long) + 3 * sizeof(char) + 1;
}

int mtype_to_str(char *write_data, m_data *mdata)
{
	switch (mdata->mtype) {
		case mdt_msg:
			return msg_to_str(write_data, mdata);
		case mdt_scheduleinfo:
			return scheduleinfo_to_str(write_data, mdata);
		default:
			return 0;
	}
}

int mdata_to_str(char *write_data, m_data *mdata)
{
	int mtype_size;
	char mtype_data[RAW_MSG_LENGTH];

	mtype_size = mtype_to_str(&mtype_data, mdata);

	strncpy(write_data, "", RT_MSG_LENGTH);  // Ensure that allocating too much data will not affect the result
	sprintf(write_data, "%llu,%u,%s\n", mdata->timestamp, mdata->mtype, mtype_data);

	return RT_MSG_LENGTH - RAW_MSG_LENGTH + mtype_size;
}