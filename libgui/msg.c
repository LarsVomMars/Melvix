// MIT License, Copyright (c) 2021 Marvin Borner

#include <assert.h>
#include <msg.h>
#include <print.h>
#include <sys.h>

static struct message msg_buf = { 0 };

int msg_send(u32 pid, enum message_type type, void *data)
{
	assert((signed)pid != -1);
	char path[32] = { 0 };
	sprintf(path, "/proc/%d/msg", pid);
	msg_buf.magic = MSG_MAGIC;
	msg_buf.src = getpid();
	msg_buf.type = type;
	msg_buf.data = data;
	return write(path, &msg_buf, 0, sizeof(msg_buf));
}

int msg_receive(struct message *msg)
{
	int ret = read("/proc/self/msg", msg, 0, sizeof(*msg));
	if (msg->magic == MSG_MAGIC && ret == sizeof(*msg))
		return ret;
	else
		return -1;
}
