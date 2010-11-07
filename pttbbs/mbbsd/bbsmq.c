#include "bbsmq.h"
#include "var.h"

#define BBSMQ_MESSAGE_PER_INVOCATION 5

void *context;
static void *write_receiver;

#define BBSMQ_DEBUG
#ifdef BBSMQ_DEBUG
static    FILE *debug;
static void _debug(const char *fmt,...)
{
    char   msg[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    fprintf(debug, "[%p] %s\n", context, msg);
    fflush(debug);
}
#else
#define _debug(format, args...)
#endif

void z_write_socket(char *buf, pid_t pid)
{
    sprintf(buf, "ipc:///tmp/write/%d", pid);
}

void boot_zmq()
{
    context = zmq_init(1);
    assert(context);

#ifdef BBSMQ_DEBUG
    char buf[128];
    sprintf(buf, "/tmp/zmq-debug.%d", getpid());
    debug = fopen(buf, "w");
#endif

    _debug("zmq boot");
}

void init_zmq()
{
    uint64_t bufsize = 1024;
    write_receiver = zmq_socket(context, ZMQ_PULL);
    zmq_setsockopt(write_receiver, ZMQ_RCVBUF, &bufsize, sizeof(bufsize));

    if(!write_receiver) {
        _debug("zmq socket fail: error: %s", zmq_strerror(errno));
        return;
    }

    char buf[128];
    z_write_socket(buf, currutmp->pid);
    int rc = zmq_bind(write_receiver, buf);
    if(rc) {
        _debug("zmq bind for %s: %d, error: %s", buf, rc, zmq_strerror(errno));
    }
}

void *get_zmq_sock(int type)
{
    _debug("zmq sock");
    return zmq_socket(context, type);
}

void process_zmq()
{
    zmq_msg_t message;
    zmq_pollitem_t items[1];

    items[0].socket = write_receiver;
    items[0].events = ZMQ_POLLIN;

    if(!context || !write_receiver)
        return;

    int rc = zmq_poll(items, 1, 0);
    _debug("process zmq: %d", rc);

    if (!rc) return;

    if (items[0].revents & ZMQ_POLLIN) {
        int i = 0;
        while(i++ < BBSMQ_MESSAGE_PER_INVOCATION) {
            zmq_msg_init(&message);
            if (zmq_recv(write_receiver, &message, ZMQ_NOBLOCK)) {
                if (errno == EAGAIN)
                    break;

                _debug("zmq_recv: %s", zmq_strerror(errno));
                break;
            }

            if( currutmp && currutmp->msgcount && !reentrant_write_request )
                write_request(1);

            zmq_msg_close(&message);
        }
    }
}

void z_sendit(char *name)
{
    void *sendit = zmq_socket(context, ZMQ_PUSH);
    int rc;
    zmq_msg_t message;

    _debug("sending to %s", name);
    int c = zmq_connect(sendit, name);
    if (c) {
        _debug("failed to connect to %s: %s", name,zmq_strerror(errno));
    }        

    zmq_msg_init_size(&message, 0);

    rc = zmq_send(sendit, &message, ZMQ_NOBLOCK);
    if (rc) {
        _debug("failed to call zmq send for %s: %s", name, zmq_strerror(errno));
    }

    zmq_msg_close(&message);

    zmq_close(sendit);
}

void destroy_zmq()
{
    if (write_receiver)
        zmq_close(write_receiver);

    if (context)
        zmq_term(context);
}
