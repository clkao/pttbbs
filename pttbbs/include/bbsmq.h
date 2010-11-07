#include <zmq.h>
#include "cmsys.h"	// for time4_t

void boot_zmq();
void init_zmq();
void process_zmq();
void destroy_zmq();
void z_sendit(char *name);

void z_write_socket(char *buf, pid_t pid);
