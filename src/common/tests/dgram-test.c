#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <murphy/common.h>

#define TAG_END     ((uint16_t)0x0)
#define TAG_SEQ     ((uint16_t)0x1)
#define TAG_FOO     ((uint16_t)0x2)
#define TAG_BAR     ((uint16_t)0x3)
#define TAG_MSG     ((uint16_t)0x4)
#define TAG_RPL     ((uint16_t)0x5)


typedef struct {
    mrp_mainloop_t  *ml;
    mrp_transport_t *t;
    int              server;
    char            *saddr;
    char            *caddr;
    int              sock;
    mrp_io_watch_t  *iow;
    mrp_timer_t     *timer;
} context_t;


void recv_evt(mrp_transport_t *t, mrp_msg_t *msg, void *user_data)
{
    context_t       *c = (context_t *)user_data;
    mrp_msg_field_t *f;
    uint32_t         seq;
    char             buf[256];
    
    mrp_log_info("received a message");
    mrp_msg_dump(msg, stdout);
    
    if (c->server) {
	seq = 0;
	if ((f = mrp_msg_find(msg, TAG_SEQ)) != NULL) {
	    if (f->type == MRP_MSG_FIELD_UINT32)
		seq = f->u32;
	}
	    
	snprintf(buf, sizeof(buf), "reply to message #%u", seq);
	
	if (!mrp_msg_append(msg, TAG_RPL, MRP_MSG_FIELD_STRING, buf,
			    TAG_END)) {
	    mrp_log_info("failed to append to received message");
	    exit(1);
	}
			   
	if (mrp_transport_send(t, msg))
	    mrp_log_info("reply successfully sent");
	else
	    mrp_log_error("failed to send reply");

	/* message unreffed by transport layer */
    }
}


void recvfrom_evt(mrp_transport_t *t, mrp_msg_t *msg, mrp_sockaddr_t *addr,
		  socklen_t addrlen, void *user_data)
{
    context_t       *c = (context_t *)user_data;
    mrp_msg_field_t *f;
    uint32_t         seq;
    char             buf[256];
    
    mrp_log_info("received a message");
    mrp_msg_dump(msg, stdout);
    
    if (c->server) {
	seq = 0;
	if ((f = mrp_msg_find(msg, TAG_SEQ)) != NULL) {
	    if (f->type == MRP_MSG_FIELD_UINT32)
		seq = f->u32;
	}
	    
	snprintf(buf, sizeof(buf), "reply to message #%u", seq);
	
	if (!mrp_msg_append(msg, TAG_RPL, MRP_MSG_FIELD_STRING, buf,
			    TAG_END)) {
	    mrp_log_info("failed to append to received message");
	    exit(1);
	}
			   
	if (mrp_transport_sendto(t, msg, addr, addrlen))
	    mrp_log_info("reply successfully sent");
	else
	    mrp_log_error("failed to send reply");

	/* message unreffed by transport layer */
    }
}


void closed_evt(mrp_transport_t *t, int error, void *user_data)
{
    context_t *c = (context_t *)user_data;

    MRP_UNUSED(t);
    MRP_UNUSED(c);

    if (error) {
	mrp_log_error("Connection closed with error %d (%s).", error,
		      strerror(error));
	exit(1);
    }
    else {
	mrp_log_info("Peer has closed the connection.");
	exit(0);
    }
}


void server_init(context_t *c)
{
    static mrp_transport_evt_t evt = {
	.closed   = closed_evt,
	.recv     = NULL,
	.recvfrom = recvfrom_evt,
    };
    mrp_sockaddr_t  addr;
    socklen_t       len;
    const char     *type;

    len = sizeof(addr);
    len = mrp_transport_resolve(c->t, c->saddr, &addr, len, &type);

    if (len > 0) {
	c->t = mrp_transport_create(c->ml, type, &evt, c, 0);
    
	if (c->t == NULL) {
	    mrp_log_error("Failed to create new transport.");
	    exit(1);
	}

	if (!mrp_transport_bind(c->t, &addr, len)) {
	    mrp_log_error("Failed to bind to %s.", c->saddr);
	    exit(1);
	}

	mrp_log_info("Waiting for messages on %s...", c->saddr);
    }
    else {
	mrp_log_error("Failed to resolve address '%s'.", c->saddr);
	exit(1);
    }
}


void send_cb(mrp_mainloop_t *ml, mrp_timer_t *t, void *user_data)
{
    static uint32_t seqno = 1;
    
    context_t *c = (context_t *)user_data;
    mrp_msg_t *msg;
    uint32_t   seq;
    char       buf[256];
    uint32_t   len;

    MRP_UNUSED(ml);
    MRP_UNUSED(t);

    
    seq = seqno++;
    len = snprintf(buf, sizeof(buf), "This is message %u.", (unsigned int)seq);
    if ((msg = mrp_msg_create(TAG_SEQ, MRP_MSG_FIELD_UINT32, seq,
			      TAG_FOO, MRP_MSG_FIELD_STRING, "foo",
			      TAG_BAR, MRP_MSG_FIELD_STRING, "bar",
			      TAG_MSG, MRP_MSG_FIELD_BLOB  , len, buf,
			      TAG_END)) == NULL) {
	mrp_log_error("Failed to create new message.");
	exit(1);
    }

    if (!mrp_transport_send(c->t, msg)) {
	mrp_log_error("Failed to send message #%d.", seq);
	exit(1);
    }
    else
	mrp_log_info("Message #%d succesfully sent.", seq);
    
    mrp_msg_unref(msg);
}


void client_init(context_t *c)
{
    static mrp_transport_evt_t evt = {
	.closed   = closed_evt,
	.recv     = recv_evt,
	.recvfrom = NULL,
    };

    mrp_sockaddr_t  sa, ca;
    socklen_t       sl, cl;
    const char     *type;

    sl = mrp_transport_resolve(NULL, c->saddr, &sa, sizeof(sa), &type);

    if (sl <= 0) {
	mrp_log_error("Failed resolve transport address '%s'.", c->saddr);
	exit(1);
    }

    c->t = mrp_transport_create(c->ml, type, &evt, c, 0);
    
    if (c->t == NULL) {
	mrp_log_error("Failed to create new transport.");
	exit(1);
    }

    if (c->caddr) {
	cl = mrp_transport_resolve(NULL, c->caddr, &ca, sizeof(ca), &type);
	
	if (cl <= 0) {
	    mrp_log_error("Failed resolve transport address '%s'.", c->caddr);
	    exit(1);
	}

	if (!mrp_transport_bind(c->t, &ca, cl)) {
	    mrp_log_error("Failed to bind to %s.", c->caddr);
	    exit(1);
	}
	else
	    mrp_log_info("Bound local endpoint to '%s'...", c->caddr);
    }
    
    if (!mrp_transport_connect(c->t, &sa, sl)) {
	mrp_log_error("Failed to connect to %s.", c->saddr);
	exit(1);
    }

    c->timer = mrp_add_timer(c->ml, 1000, send_cb, c);

    if (c->timer == NULL) {
	mrp_log_error("Failed to create send timer.");
	exit(1);
    }
}


int main(int argc, char *argv[])
{
    context_t c;
    int       i;

    mrp_clear(&c);

    mrp_log_set_mask(MRP_LOG_UPTO(MRP_LOG_DEBUG));
    mrp_log_set_target(MRP_LOG_TO_STDOUT);

    for (i = 1; i < argc; i++) {
	if (!strcmp(argv[i], "-s") || !strcmp(argv[i], "--server"))
	    c.server = TRUE;
	else if (!strcmp(argv[i], "-c") || !strcmp(argv[i], "--client"))
	    c.server = FALSE;
	else {
	    if (c.saddr == NULL)
		c.saddr = argv[i];
	    else if (c.caddr == NULL)
		c.caddr = argv[i];
	    else {
		mrp_log_error("Unrecognized argument '%s'.", argv[i]);
		goto invalid_cmdline;
	    }
	}
    }

    if (c.server)
	mrp_log_info("Running as server, using address '%s'...", c.saddr);
    else
	mrp_log_info("Running as client, server is at '%s'...", c.saddr);
    
    if (c.caddr)
	mrp_log_info("Going to bind client side-socket to '%s'...", c.caddr);
    

    c.ml = mrp_mainloop_create();

    if (c.server)
	server_init(&c);
    else
	client_init(&c);
    
    mrp_mainloop_run(c.ml);

    return 0;

 invalid_cmdline:
    mrp_log_error("invalid command line arguments");
    mrp_log_error("usage: %s [-s|-c] <server> [<client>]", argv[0]);
    exit(1);
}