/*	$KAME: sctputil.c,v 1.19 2003/08/29 06:37:38 itojun Exp $	*/
/*	Header: /home/sctpBsd/netinet/sctputil.c,v 1.153 2002/04/04 16:59:01 randall Exp	*/

/*
 * Copyright (c) 2001, 2002 Cisco Systems, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Cisco Systems, Inc.
 * 4. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY CISCO SYSTEMS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL CISCO SYSTEMS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#ifndef __OpenBSD__
#include "opt_ipsec.h"
#endif
#if defined(__FreeBSD__) && __FreeBSD__ >= 3
#include "opt_compat.h"
#include "opt_inet6.h"
#include "opt_inet.h"
#include "opt_mpath.h"
#endif
#if defined(__NetBSD__)
#include "opt_inet.h"
#endif
#ifndef __OpenBSD__
#include "opt_sctp.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#if defined (__OpenBSD__)
#include <netinet/sctp_callout.h>
#else
#include <sys/callout.h>
#endif
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/sysctl.h>


#include <net/radix.h>
#include <net/route.h>

#ifdef INET6
#ifndef __OpenBSD__
#include <sys/domain.h>
#endif
#endif

#include <machine/limits.h>

#if defined(__FreeBSD__)
#include <vm/vm_zone.h>
#endif

#if defined(__NetBSD__) || defined(__OpenBSD__)
#include <sys/pool.h>
#endif

#include <net/if.h>
#include <net/if_types.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>
#include <netinet/in_var.h>
#include <netinet/ip_var.h>

#ifdef INET6
#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>

#if defined(__FreeBSD__) || (__NetBSD__)
#include <netinet6/in6_pcb.h>
#elif defined(__OpenBSD__) || (defined(__bsdi__) && _BSDI_VERSION >= 199802)
#include <netinet/in_pcb.h>
#endif

#endif /* INET6 */

#include "faith.h"

#include <netinet/sctp_pcb.h>

#ifdef IPSEC
#ifndef __OpenBSD__
#include <netinet6/ipsec.h>
#include <netkey/key.h>
#else
#undef IPSEC
#endif
#endif /* IPSEC */

#include <netinet/sctputil.h>
#include <netinet/sctp_var.h>
#include <netinet6/sctp6_var.h>
#include <netinet/sctp_header.h>
#include <netinet/sctp_output.h>
#include <netinet/sctp_hashdriver.h>
#include <netinet/sctp_uio.h>
#include <netinet/sctp_timer.h>
#ifndef SCTP_USE_ADLER32
#include <netinet/sctp_crc32.h>
#endif /* SCTP_USE_ADLER32 */

#define NUMBER_OF_MTU_SIZES 18


#ifdef SCTP_DEBUG
extern u_int32_t sctp_debug_on;
#endif

#ifdef SCTP_CWND_LOGGING
int sctp_cwnd_log_at=0;
int sctp_cwnd_log_rolled=0;
struct sctp_cwnd_log sctp_clog[SCTP_CWND_LOG_SIZE];

void sctp_log_cwnd(struct sctp_nets *net, int augment, uint8_t from)
{
	sctp_clog[sctp_cwnd_log_at].net = net;
	sctp_clog[sctp_cwnd_log_at].cwnd_new_value = net->cwnd;
	sctp_clog[sctp_cwnd_log_at].cwnd_augment = augment;
	sctp_clog[sctp_cwnd_log_at].from = from;
	sctp_cwnd_log_at++;
	if (sctp_cwnd_log_at >= SCTP_CWND_LOG_SIZE) {
		sctp_cwnd_log_at = 0;
		sctp_cwnd_log_rolled = 1;
	}
}

int
sctp_fill_cwnd_log(struct mbuf *m)
{
	struct sctp_cwnd_log_req *req;
	int size_limit, num, i, at, cnt_out=0;

	if (m == NULL)
		return (EINVAL);

	size_limit = (m->m_len - sizeof(struct sctp_cwnd_log_req));
	if (size_limit < sizeof(struct sctp_cwnd_log)) {
		return (EINVAL);
	}
	req = mtod(m, struct sctp_cwnd_log_req *);
	num = size_limit/sizeof(struct sctp_cwnd_log);
	if (sctp_cwnd_log_rolled)
		req->num_in_log = SCTP_CWND_LOG_SIZE;
	else
		req->num_in_log = sctp_cwnd_log_at;

	if ((num < SCTP_CWND_LOG_SIZE) && 
	    ((sctp_cwnd_log_rolled) || (sctp_cwnd_log_at > num))) {
		/* we can't return all of it */
		if (((req->start_at == 0) && (req->end_at == 0)) ||
		    (req->start_at >= SCTP_CWND_LOG_SIZE) ||
		    (req->end_at >= SCTP_CWND_LOG_SIZE)) {
			/* No user request or user is wacked. */
			req->num_ret = num;
			req->end_at = sctp_cwnd_log_at - 1;
			if ((sctp_cwnd_log_at - num) < 0) {
				int cc;
				cc = num - sctp_cwnd_log_at;
				req->start_at = SCTP_CWND_LOG_SIZE - cc;
			} else {
				req->start_at = sctp_cwnd_log_at - num;
			}
		} else {
			/* a user request */
			int cc;
			if (req->start_at > req->end_at) {
				cc = (SCTP_CWND_LOG_SIZE - req->start_at) +
				    (req->end_at + 1);
			} else {
				cc = req->end_at - req->start_at;
			}
			if (cc < num) {
				num = cc;
			}
			req->num_ret = num;
		}
	} else {
		/* We can return all  of it */
		if (sctp_cwnd_log_rolled) {
			req->num_ret = SCTP_CWND_LOG_SIZE;
			req->start_at = sctp_cwnd_log_at;
			req->end_at = sctp_cwnd_log_at - 1;
		} else {
			req->start_at = 0;
			req->end_at = sctp_cwnd_log_at - 1;
			req->num_ret = sctp_cwnd_log_at;
		}
	}
	for (i = 0, at = req->start_at; i < req->num_ret; i++) {
		req->log[i] = sctp_clog[at];
		cnt_out++;
		at++;
		if (at >= SCTP_CWND_LOG_SIZE)
			at = 0;
	}
	m->m_len = (cnt_out * sizeof(struct sctp_cwnd_log_req)) + sizeof(struct sctp_cwnd_log_req);
	return (0);
}

#endif

#ifdef SCTP_AUDITING_ENABLED
u_int8_t sctp_audit_data[SCTP_AUDIT_SIZE][2];
static int sctp_audit_indx=0;

static
void sctp_print_audit_report(void)
{
	int i;
	int cnt;
	cnt = 0;
	for (i=sctp_audit_indx;i<SCTP_AUDIT_SIZE;i++) {
		if ((sctp_audit_data[i][0] == 0xe0) &&
		    (sctp_audit_data[i][1] == 0x01)) {
			cnt =0;
			printf("\n");
		} else if (sctp_audit_data[i][0] == 0xf0) {
			cnt =0;
			printf("\n");
		} else if ((sctp_audit_data[i][0] == 0xc0) &&
		    (sctp_audit_data[i][1] == 0x01)) {
			printf("\n");
			cnt = 0;
		}
		printf("%2.2x%2.2x ", (u_int)sctp_audit_data[i][0],
		    (u_int)sctp_audit_data[i][1]);
		cnt++;
		if ((cnt % 14) == 0)
			printf("\n");
	}
	for (i=0;i<sctp_audit_indx;i++) {
		if ((sctp_audit_data[i][0] == 0xe0) &&
		    (sctp_audit_data[i][1] == 0x01)) {
			cnt =0;
			printf("\n");
		} else if (sctp_audit_data[i][0] == 0xf0) {
			cnt =0;
			printf("\n");
		} else if ((sctp_audit_data[i][0] == 0xc0) &&
			 (sctp_audit_data[i][1] == 0x01)) {
			printf("\n");
			cnt = 0;
		}
		printf("%2.2x%2.2x ", (u_int)sctp_audit_data[i][0],
		    (u_int)sctp_audit_data[i][1]);
		cnt++;
		if ((cnt % 14) == 0)
			printf("\n");
	}
	printf("\n");
}

void sctp_auditing(int from, struct sctp_inpcb *ep, struct sctp_tcb *tcb,
    struct sctp_nets *net)
{
	int s, resend_cnt, tot_out, rep, tot_book, tot_book_cnt;
	struct sctp_nets *lnet;
	struct sctp_tmit_chunk *chk;
#if defined(__NetBSD__) || defined(__OpenBSD__)
	s = splsoftnet();
#else
	s = splnet();
#endif
	sctp_audit_data[sctp_audit_indx][0] = 0xAA;
	sctp_audit_data[sctp_audit_indx][1] = 0x000000ff & from;
	sctp_audit_indx++;
	if (sctp_audit_indx >= SCTP_AUDIT_SIZE) {
		sctp_audit_indx = 0;
	}
	if (ep == NULL) {
		sctp_audit_data[sctp_audit_indx][0] = 0xAF;
		sctp_audit_data[sctp_audit_indx][1] = 0x01;
		sctp_audit_indx++;
		if (sctp_audit_indx >= SCTP_AUDIT_SIZE) {
			sctp_audit_indx = 0;
		}
		splx(s);
		return;
	}
	if (tcb == NULL) {
		sctp_audit_data[sctp_audit_indx][0] = 0xAF;
		sctp_audit_data[sctp_audit_indx][1] = 0x02;
		sctp_audit_indx++;
		if (sctp_audit_indx >= SCTP_AUDIT_SIZE) {
			sctp_audit_indx = 0;
		}
		splx(s);
		return;
	}
	sctp_audit_data[sctp_audit_indx][0] = 0xA1;
	sctp_audit_data[sctp_audit_indx][1] =
	    (0x000000ff & tcb->asoc.sent_queue_retran_cnt);
	sctp_audit_indx++;
	if (sctp_audit_indx >= SCTP_AUDIT_SIZE) {
		sctp_audit_indx = 0;
	}
	rep = 0;
	tot_book = tot_book_cnt = 0;
	resend_cnt = tot_out = 0;
	TAILQ_FOREACH(chk, &tcb->asoc.sent_queue, sctp_next) {
		if (chk->sent == SCTP_DATAGRAM_RESEND) {
			resend_cnt++;
		} else if (chk->sent < SCTP_DATAGRAM_RESEND) {
			tot_out += chk->send_size;
			tot_book += chk->book_size;
			tot_book_cnt++;
		}
	}
	if (resend_cnt != tcb->asoc.sent_queue_retran_cnt) {
		sctp_audit_data[sctp_audit_indx][0] = 0xAF;
		sctp_audit_data[sctp_audit_indx][1] = 0xA1;
		sctp_audit_indx++;
		if (sctp_audit_indx >= SCTP_AUDIT_SIZE) {
			sctp_audit_indx = 0;
		}
		printf("resend_cnt:%d asoc-tot:%d\n",
		    resend_cnt, tcb->asoc.sent_queue_retran_cnt);
		rep = 1;
		tcb->asoc.sent_queue_retran_cnt = resend_cnt;
		sctp_audit_data[sctp_audit_indx][0] = 0xA2;
		sctp_audit_data[sctp_audit_indx][1] =
		    (0x000000ff & tcb->asoc.sent_queue_retran_cnt);
		sctp_audit_indx++;
		if (sctp_audit_indx >= SCTP_AUDIT_SIZE) {
			sctp_audit_indx = 0;
		}
	}
	if (tot_out != tcb->asoc.total_flight) {
		sctp_audit_data[sctp_audit_indx][0] = 0xAF;
		sctp_audit_data[sctp_audit_indx][1] = 0xA2;
		sctp_audit_indx++;
		if (sctp_audit_indx >= SCTP_AUDIT_SIZE) {
			sctp_audit_indx = 0;
		}
		rep = 1;
		printf("tot_flt:%d asoc_tot:%d\n", tot_out,
		    (int)tcb->asoc.total_flight);
		tcb->asoc.total_flight = tot_out;
	}
	if (tot_book != tcb->asoc.total_flight_book) {
		sctp_audit_data[sctp_audit_indx][0] = 0xAF;
		sctp_audit_data[sctp_audit_indx][1] = 0xA4;
		sctp_audit_indx++;
		if (sctp_audit_indx >= SCTP_AUDIT_SIZE) {
			sctp_audit_indx = 0;
		}
		rep = 1;
		printf("tot_flt_book:%d asoc_tot:%d\n", tot_book,
		    (int)tcb->asoc.total_flight_book);
		tcb->asoc.total_flight_book = tot_book;
	}
	if (tot_book_cnt != tcb->asoc.total_flight_count) {
		sctp_audit_data[sctp_audit_indx][0] = 0xAF;
		sctp_audit_data[sctp_audit_indx][1] = 0xA5;
		sctp_audit_indx++;
		if (sctp_audit_indx >= SCTP_AUDIT_SIZE) {
			sctp_audit_indx = 0;
		}
		rep = 1;
		printf("tot_flt_book:%d asoc_tot:%d\n", tot_book,
		    (int)tcb->asoc.total_flight_book);
		tcb->asoc.total_flight_count = tot_book_cnt;
	}
	tot_out = 0;
	TAILQ_FOREACH(lnet,&tcb->asoc.nets, sctp_next) {
		tot_out += lnet->flight_size;
	}
	if (tot_out != tcb->asoc.total_flight) {
		sctp_audit_data[sctp_audit_indx][0] = 0xAF;
		sctp_audit_data[sctp_audit_indx][1] = 0xA3;
		sctp_audit_indx++;
		if (sctp_audit_indx >= SCTP_AUDIT_SIZE) {
			sctp_audit_indx = 0;
		}
		rep = 1;
		printf("real flight:%d net total was %d\n",
		    tcb->asoc.total_flight, tot_out);
		/* now corrective action */
		TAILQ_FOREACH(lnet,&tcb->asoc.nets, sctp_next) {
			tot_out = 0;
			TAILQ_FOREACH(chk, &tcb->asoc.sent_queue, sctp_next) {
				if ((chk->whoTo == lnet) &&
				    (chk->sent < SCTP_DATAGRAM_RESEND)) {
					tot_out += chk->send_size;
				}
			}
			if (lnet->flight_size != tot_out) {
				printf("net:%x flight was %d corrected to %d\n",
				    (u_int)lnet, lnet->flight_size, tot_out);
				lnet->flight_size = tot_out;
			}

		}
	}
	
	if (rep) {
		sctp_print_audit_report();
	}
	splx(s);
}

void
sctp_audit_log(u_int8_t ev, u_int8_t fd)
{
	int s;
#if defined(__NetBSD__) || defined(__OpenBSD__)
	s = splsoftnet();
#else
	s = splnet();
#endif
	sctp_audit_data[sctp_audit_indx][0] = ev;
	sctp_audit_data[sctp_audit_indx][1] = fd;
	sctp_audit_indx++;
	if (sctp_audit_indx >= SCTP_AUDIT_SIZE) {
		sctp_audit_indx = 0;
	}
	splx(s);
}

#endif

/*
 * a list of sizes based on typical mtu's, used only if next hop
 * size not returned.
 */
static int sctp_mtu_sizes[] = {
	68,
	296,
	508,
	512,
	544,
	576,
	1006,
	1492,
	1500,
	1536,
	2002,
	2048,
	4352,
	4464,
	8166,
	17914,
	32000,
	65535
};

int
find_next_best_mtu(int totsz)
{
	int i, perfer;
	/*
	 * if we are in here we must find the next best fit based on the
	 * size of the dg that failed to be sent.
	 */
	perfer = 0;
	for (i = 0; i < NUMBER_OF_MTU_SIZES; i++) {
		if (totsz < sctp_mtu_sizes[i]) {
			perfer = i - 1;
			if (perfer < 0)
				perfer = 0;
			break;
		}
	}
	return (sctp_mtu_sizes[perfer]);
}

void
sctp_fill_random_store(struct sctp_pcb *m)
{
	/*
	 * Here we use the MD5/SHA-1 to hash with our good randomNumbers
	 * and our counter. The result becomes our good random numbers and
	 * we then setup to give these out.
	 */
	m->store_at = 0;
	sctp_hash_digest((char *)m->random_numbers, sizeof(m->random_numbers),
			 (char *)&m->random_counter, sizeof(m->random_counter),
			 (char *)m->random_store);
	m->random_counter++;
}

u_int32_t sctp_select_initial_TSN(struct sctp_pcb *m)
{
	/*
	 * A true implementation should use random selection process to
	 * get the initial stream sequence number, using RFC1750 as a
	 * good guideline
	 */
	u_long x, *xp;
	unsigned char *p;

	if (m->initial_sequence_debug != 0) {
		u_int32_t ret;
		ret = m->initial_sequence_debug;
		m->initial_sequence_debug++;
		return (ret);
	}
	if ((m->store_at+sizeof(u_long)) > SCTP_SIGNATURE_SIZE) {
		/* Refill the random store */
		sctp_fill_random_store(m);
	}
	p = &m->random_store[(int)m->store_at];
	xp = (u_long *)p;
	x = *xp;
	m->store_at += sizeof(u_long);
	return (x);
}

u_int32_t sctp_select_a_tag(struct sctp_inpcb *m)
{
	u_long x, not_done;
	struct timeval now;

	SCTP_GETTIME_TIMEVAL(&now);
	not_done = 1;
	while (not_done) {
		x = sctp_select_initial_TSN(&m->sctp_ep);
		if (sctp_is_vtag_good(m, x, &now)) {
			not_done = 0;
		}
	}
	return (x);
}


int
sctp_init_asoc(struct sctp_inpcb *m, struct sctp_association *asoc,
	       int for_a_init)
{
	/*
	 * Anything set to zero is taken care of by the allocation
	 * routine's bzero
	 */

	/* Up front select what scoping to apply on addresses I tell my peer
	 * Not sure what to do with these right now, we will need to come up
	 * with a way to set them. We may need to pass them through from the
	 * caller in the sctp_aloc_assoc() function.
	 */
	int i;
	/* init all variables to a known value.*/
	asoc->max_burst = m->sctp_ep.max_burst;
	asoc->heart_beat_delay = m->sctp_ep.sctp_timeoutticks[SCTP_TIMER_HEARTBEAT];
	asoc->cookie_life = m->sctp_ep.def_cookie_life;

	asoc->my_vtag = sctp_select_a_tag(m);
	asoc->asconf_seq_out = asoc->init_seq_number = asoc->sending_seq =
		sctp_select_initial_TSN(&m->sctp_ep);
	asoc->t3timeout_highest_marked = asoc->asconf_seq_out;
	asoc->peer_supports_asconf = 1;
	asoc->peer_supports_asconf_setprim = 1;

	asoc->sent_queue_retran_cnt = 0;
	/* This will need to be adjusted */
	asoc->last_cwr_tsn = asoc->init_seq_number - 1;
	asoc->last_acked_seq = asoc->init_seq_number - 1;
	asoc->advanced_peer_ack_point = asoc->last_acked_seq;
	asoc->asconf_seq_in = asoc->last_acked_seq;
	asoc->initial_init_rto_max = m->sctp_ep.initial_init_rto_max;
	asoc->initial_rto = m->sctp_ep.initial_rto;

	asoc->max_init_times = m->sctp_ep.max_init_times;
	asoc->max_send_times = m->sctp_ep.max_send_times;
	asoc->def_net_failure = m->sctp_ep.def_net_failure;

	if (m->sctp_flags & SCTP_PCB_FLAGS_BOUND_V6) {
		struct in6pcb *inp6;


		/* Its a V6 socket */
		inp6 = (struct in6pcb *)m;
		asoc->ipv6_addr_legal = 1;
		/* Now look at the binding flag to see if V4 will be legal */
	if (
#if defined(__OpenBSD__)
		(0) /* we always do dual bind */
#elif defined (__NetBSD__)
		(inp6->in6p_flags & IN6P_IPV6_V6ONLY)
#else
		(inp6->inp_flags & IN6P_IPV6_V6ONLY)
#endif
	     == 0) {
			asoc->ipv4_addr_legal = 1;
		} else {
			/* V4 addresses are NOT legal on the association */
			asoc->ipv4_addr_legal = 0;
		}
	} else {
		/* Its a V4 socket, no - V6 */
		asoc->ipv4_addr_legal = 1;
		asoc->ipv6_addr_legal = 0;
	}


	asoc->my_rwnd = max(m->sctp_socket->so_rcv.sb_hiwat, SCTP_MINIMAL_RWND);
	asoc->peers_rwnd = m->sctp_socket->so_rcv.sb_hiwat;

	asoc->smallest_mtu = m->sctp_frag_point;

	LIST_INIT(&asoc->sctp_local_addr_list);
	TAILQ_INIT(&asoc->nets);
	asoc->last_asconf_ack_sent = NULL;
	/* Setup to fill the hb random cache at first HB */
	asoc->hb_random_idx = 4;

	asoc->sctp_autoclose_ticks = m->sctp_ep.auto_close_time;

	/*
	 * Now the stream parameters, here we allocate space for all
	 * streams that we request by default.
	 */
	asoc->streamoutcnt = asoc->pre_open_streams =
	    m->sctp_ep.pre_open_stream_count;
	asoc->strmout = malloc((asoc->streamoutcnt *
	    sizeof(struct sctp_stream_out)), M_PCB, M_NOWAIT);
	if (asoc->strmout == NULL) {
		/* big trouble no memory */
		return (ENOMEM);
	}
	for (i = 0; i < asoc->streamoutcnt; i++) {
		/*
		 * inbound side must be set to 0xffff,
		 * also NOTE when we get the INIT-ACK back (for INIT sender)
		 * we MUST reduce the count (streamoutcnt) but first check
		 * if we sent to any of the upper streams that were dropped
		 * (if some were). Those that were dropped must be notified
		 * to the upper layer as failed to send.
		 */
		asoc->strmout[i].next_sequence_sent = 0x0;
		TAILQ_INIT(&asoc->strmout[i].outqueue);
		asoc->strmout[i].stream_no = i;
		asoc->strmout[i].next_spoke.tqe_next = 0;
		asoc->strmout[i].next_spoke.tqe_prev = 0;
	}
	/* Now the mapping array */
	asoc->mapping_array_size = SCTP_INITIAL_MAPPING_ARRAY;
	asoc->mapping_array = malloc(asoc->mapping_array_size, M_PCB, M_NOWAIT);
	if (asoc->mapping_array == NULL) {
		free(asoc->strmout, M_PCB);
		return (ENOMEM);
	}
	memset(asoc->mapping_array, 0, asoc->mapping_array_size);
	/* Now the init of the other outqueues */
	TAILQ_INIT(&asoc->out_wheel);
	TAILQ_INIT(&asoc->control_send_queue);
	TAILQ_INIT(&asoc->send_queue);
	TAILQ_INIT(&asoc->sent_queue);
	TAILQ_INIT(&asoc->reasmqueue);
	TAILQ_INIT(&asoc->delivery_queue);
	asoc->max_inbound_streams = m->sctp_ep.max_open_streams_intome;

	TAILQ_INIT(&asoc->asconf_queue);
	return (0);
}

int
sctp_expand_mapping_array(struct sctp_association *asoc)
{
	/* mapping array needs to grow */
	u_int8_t *new_array;
	uint16_t new_size;
	
	new_size = asoc->mapping_array_size + SCTP_MAPPING_ARRAY_INCR;
	new_array = malloc(new_size, M_PCB, M_NOWAIT);
	if (new_array == NULL) {
		/* can't get more, forget it */
		printf("No memory for expansion of SCTP mapping array %d\n",
		       new_size);
		return (-1);
	}
	memset(new_array, 0, new_size);
	memcpy(new_array, asoc->mapping_array, asoc->mapping_array_size);
	free(asoc->mapping_array, M_PCB);
	asoc->mapping_array = new_array;
	asoc->mapping_array_size = new_size;
	return (0);
}


static void
sctp_timeout_handler(void *t)
{
	struct sctp_inpcb *ep;
	struct sctp_tcb *tcb;
	struct sctp_nets *net;
	struct sctp_timer *tmr;
	int s, did_output, typ;

#if defined(__NetBSD__) || defined(__OpenBSD__)
	s = splsoftnet();
#else
	s = splnet();
#endif
	tmr = (struct sctp_timer *)t;
	ep = (struct sctp_inpcb *)tmr->ep;
	tcb = (struct sctp_tcb *)tmr->tcb;
	net = (struct sctp_nets *)tmr->net;
	did_output = 1;

#ifdef SCTP_AUDITING_ENABLED
	sctp_audit_log(0xF0, (u_int8_t)tmr->type);
	sctp_auditing(3, ep, tcb, net);
#endif
	sctp_pegs[SCTP_TIMERS_EXP]++;
	if (ep) {
		if (ep->sctp_socket == 0) {
			splx(s);
			return;
		}
	}
	if (tcb) {
		if (tcb->asoc.state == 0) {
			splx(s);
			return;
		}
	}
#ifdef SCTP_DEBUG
	if (ep) {
		if (ep->sctp_flags & SCTP_PCB_FLAGS_SOCKET_GONE) {
			if (LIST_FIRST(&ep->sctp_asoc_list) == NULL) {
				printf("Timer type %d fires on GONE enpoint:%x\n",
				    tmr->type,(u_int)ep);
				if (tcb)
					printf("tcb:%x\n",(u_int)tcb);

				printf("Hmm, all assoc's are gone?\n");
			}
		}
	}
#endif
#ifdef SCTP_DEBUG
	if (sctp_debug_on & SCTP_DEBUG_TIMER1) {
		printf("Timer type %d goes off\n", tmr->type);
	}
#endif /* SCTP_DEBUG */
	if (!callout_active(&tmr->timer)) {
		splx(s);
		return;
	}
	typ = tmr->type;
	switch(tmr->type) {
		/* call the handler for the appropriate timer type */
	case SCTP_TIMER_TYPE_SEND:
		sctp_pegs[SCTP_TMIT_TIMER]++;
		sctp_t3rxt_timer(ep, tcb, net);
#ifdef SCTP_AUDITING_ENABLED
		sctp_auditing(4, ep, tcb, net);
#endif
		sctp_chunk_output(ep, tcb, 1);
		break;
	case SCTP_TIMER_TYPE_INIT:
		sctp_t1init_timer(ep, tcb, net);
		/* We do output but not here */
		did_output = 0;
		break;
	case SCTP_TIMER_TYPE_RECV:
		sctp_pegs[SCTP_RECV_TIMER]++;
		sctp_send_sack(tcb);
#ifdef SCTP_AUDITING_ENABLED
		sctp_auditing(4, ep, tcb, net);
#endif
		sctp_chunk_output(ep, tcb, 4);
		break;
	case SCTP_TIMER_TYPE_SHUTDOWN:
		sctp_shutdown_timer(ep, tcb, net);
#ifdef SCTP_AUDITING_ENABLED
		sctp_auditing(4, ep, tcb, net);
#endif
		sctp_chunk_output(ep, tcb, 5);
		break;
	case SCTP_TIMER_TYPE_HEARTBEAT:
		sctp_heartbeat_timer(ep, tcb, net);
#ifdef SCTP_AUDITING_ENABLED
		sctp_auditing(4, ep, tcb, net);
#endif
		sctp_chunk_output(ep, tcb, 6);
		break;
	case SCTP_TIMER_TYPE_COOKIE:
		sctp_cookie_timer(ep, tcb, net);
#ifdef SCTP_AUDITING_ENABLED
		sctp_auditing(4, ep, tcb, net);
#endif
		sctp_chunk_output(ep, tcb, 1);
		break;
	case SCTP_TIMER_TYPE_NEWCOOKIE:
	{
		struct timeval tv;
		int i, secret;
		SCTP_GETTIME_TIMEVAL(&tv);
		ep->sctp_ep.time_of_secret_change = tv.tv_sec;
		ep->sctp_ep.last_secret_number =
		    ep->sctp_ep.current_secret_number;
		ep->sctp_ep.current_secret_number++;
		if (ep->sctp_ep.current_secret_number >=
		    SCTP_HOW_MANY_SECRETS) {
			ep->sctp_ep.current_secret_number = 0;
		}
		secret = (int)ep->sctp_ep.current_secret_number;
		for (i = 0; i < SCTP_NUMBER_OF_SECRETS; i++) {
			ep->sctp_ep.secret_key[secret][i] =
			    sctp_select_initial_TSN(&ep->sctp_ep);
		}
		sctp_timer_start(SCTP_TIMER_TYPE_NEWCOOKIE, ep, tcb, net);
	}
	did_output = 0;
	break;
	case SCTP_TIMER_TYPE_PATHMTURAISE:
		sctp_pathmtu_timer(ep, tcb, net);
		did_output = 0;
		break;
	case SCTP_TIMER_TYPE_SHUTDOWNACK:
		sctp_shutdownack_timer(ep, tcb, net);
#ifdef SCTP_AUDITING_ENABLED
		sctp_auditing(4, ep, tcb, net);
#endif
		sctp_chunk_output(ep, tcb, 7);
		break;
	case SCTP_TIMER_TYPE_SHUTDOWNGUARD:
		sctp_abort_an_association(ep, tcb,
					  SCTP_SHUTDOWN_GUARD_EXPIRES, NULL);
		if (ep->sctp_flags & SCTP_PCB_FLAGS_SOCKET_GONE) {
			/* Yes, so can we purge ourself now */
			if (LIST_FIRST(&ep->sctp_asoc_list) == NULL) {
				/* finish the job now */
				sctp_inpcb_free(ep,1);
			}
		}
		did_output = 0;
		break;
	case SCTP_TIMER_TYPE_ASCONF:
		sctp_asconf_timer(ep, tcb, net);
#ifdef SCTP_AUDITING_ENABLED
		sctp_auditing(4, ep, tcb, net);
#endif
		sctp_chunk_output(ep, tcb, 8);
		break;
	case SCTP_TIMER_TYPE_AUTOCLOSE:
		sctp_autoclose_timer(ep, tcb, net);
		sctp_chunk_output(ep, tcb, 10);
		did_output = 0;
		break;
	default:
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_TIMER1) {
			printf("sctp_timeout_handler:unknown timer %d\n",
			       tmr->type);
		}
#endif /* SCTP_DEBUG */
		break;
	};
#ifdef SCTP_AUDITING_ENABLED
	sctp_audit_log(0xF1,(u_int8_t)tmr->type);
	sctp_auditing(5, ep, tcb, net);
#endif
	if (did_output) {
		/*
		 * Now we need to clean up the control chunk chain if an
		 * ECNE is on it. It must be marked as UNSENT again so next
		 * call will continue to send it until such time that we get
		 * a CWR, to remove it. It is, however, less likely that we
		 * will find a ecn echo on the chain though.
		 */
		sctp_fix_ecn_echo(&tcb->asoc);
	}
#ifdef SCTP_DEBUG
	if (sctp_debug_on & SCTP_DEBUG_TIMER1) {
		printf("Timer now complete (type %d)\n", typ);
	}
#endif /* SCTP_DEBUG */
	splx(s);
}

int
sctp_timer_start(int t_type, struct sctp_inpcb *ep, struct sctp_tcb *tcb,
    struct sctp_nets *net)
{
	int to_ticks;
	struct sctp_timer *tmr;

	if (ep == NULL)
		return (EFAULT);

	to_ticks = 0;

	tmr = NULL;
	switch (t_type) {
	case SCTP_TIMER_TYPE_SEND:
		/* Here we use the RTO timer */
	{
		int rto_val;
		if ((tcb == NULL) || (net == NULL)) {
			return (EFAULT);
		}
		tmr = &net->rxt_timer;
		if (net->RTO == 0) {
			rto_val = tcb->asoc.initial_rto;
		} else {
			rto_val = net->RTO;
		}
		to_ticks = (rto_val * hz)/1000;
	}
	break;
	case SCTP_TIMER_TYPE_INIT:
		/*
		 * Here we use the INIT timer default
		 * usually about 1 minute.
		 */
		if ((tcb == NULL) || (net == NULL)) {
			return (EFAULT);
		}
		tmr = &net->rxt_timer;
		if (net->RTO == 0) {
			to_ticks = (tcb->asoc.initial_rto * hz)/1000;
		} else {
			to_ticks = (net->RTO * hz)/1000;
		}
		break;
	case SCTP_TIMER_TYPE_RECV:
		/*
		 * Here we use the Delayed-Ack timer value from the ep
		 * ususually about 200ms.
		 */
		if (tcb == NULL) {
			return (EFAULT);
		}
		tmr = &tcb->asoc.dack_timer;
		to_ticks = ep->sctp_ep.sctp_timeoutticks[SCTP_TIMER_RECV];
		break;
	case SCTP_TIMER_TYPE_SHUTDOWN:
		/* Here we use the RTO of the destination. */
		if ((tcb == NULL) || (net == NULL)) {
			return (EFAULT);
		}

		if (net->RTO == 0) {
			to_ticks = (tcb->asoc.initial_rto * hz)/1000;
		} else {
			to_ticks = (net->RTO * hz)/1000;
		}
		tmr = &net->rxt_timer;
		break;
	case SCTP_TIMER_TYPE_HEARTBEAT:
		/*
		 * the net is used here so that we can add in the RTO.
		 * Even though we use a different timer. We also add the
		 * HB timer PLUS a random jitter.
		 */
		if (tcb == NULL) {
			return (EFAULT);
		}
		{
			u_int rndval;
			u_int8_t this_random;
			if (tcb->asoc.hb_random_idx > 3) {
				
				rndval = sctp_select_initial_TSN(&ep->sctp_ep);
				memcpy(tcb->asoc.hb_random_values, &rndval,
				       sizeof(tcb->asoc.hb_random_values));
				this_random = tcb->asoc.hb_random_values[0];
				tcb->asoc.hb_random_idx = 0;
				tcb->asoc.hb_ect_randombit = 0;
			} else {
				this_random = tcb->asoc.hb_random_values[tcb->asoc.hb_random_idx];
				tcb->asoc.hb_random_idx++;
				tcb->asoc.hb_ect_randombit = 0;
			}
			/* this_random will be 0 - 256 ms 
			 * RTO is in ms.
			 */
			if (tcb->asoc.heart_beat_delay == 0) {
				/* no HB on this ep */
				return (0);
			}
			if (net) {
				struct sctp_nets *lnet;
				int delay;
				delay = tcb->asoc.heart_beat_delay;
				TAILQ_FOREACH(lnet, &tcb->asoc.nets, sctp_next) {
					if ((lnet->dest_state & SCTP_ADDR_UNCONFIRMED) &&
					    ((lnet->dest_state & SCTP_ADDR_OUT_OF_SCOPE) == 0) && 
					    (lnet->dest_state & SCTP_ADDR_REACHABLE)) {
					    delay = 0;
					}
				}
				if (net->RTO == 0) {
					/* Never been checked */
					to_ticks = this_random + tcb->asoc.initial_rto + delay;
				} else {
					/* set rto_val to the ms */
					to_ticks = delay + net->RTO + this_random;
				}
			} else {
				to_ticks = tcb->asoc.heart_beat_delay + this_random + tcb->asoc.initial_rto;
			}
			/* Now we must convert the to_ticks that are now in ms to
			 * ticks.
			 */
			to_ticks *= hz;
			to_ticks /= 1000;
			tmr = &tcb->asoc.hb_timer;
		}
		break;
	case SCTP_TIMER_TYPE_COOKIE:
		/*
		 * Here we can use the RTO timer from the network since
		 * one RTT was compelete. If a retran happened then we will
		 * be using the RTO initial value.
		 */
		if ((tcb == NULL) || (net == NULL)) {
			return (EFAULT);
		}
		if (net->RTO == 0) {
			to_ticks = (tcb->asoc.initial_rto * hz)/1000;
		} else {
			to_ticks = (net->RTO * hz)/1000;
		}
		tmr = &net->rxt_timer;
		break;
	case SCTP_TIMER_TYPE_NEWCOOKIE:
		/*
		 * nothing needed but the endpoint here
		 * ususually about 60 minutes.
		 */
		tmr = &ep->sctp_ep.signature_change;
		to_ticks = ep->sctp_ep.sctp_timeoutticks[SCTP_TIMER_SIGNATURE];
		break;
	case SCTP_TIMER_TYPE_PATHMTURAISE:
		/*
		 * Here we use the value found in the EP for PMTU
		 * ususually about 10 minutes.
		 */
		if (tcb == NULL) {
			return (EFAULT);
		}
		to_ticks = ep->sctp_ep.sctp_timeoutticks[SCTP_TIMER_PMTU];
		tmr = &tcb->asoc.pmtu;
		break;
	case SCTP_TIMER_TYPE_SHUTDOWNACK:
		/* Here we use the RTO of the destination */
		if ((tcb == NULL) || (net == NULL)) {
			return (EFAULT);
		}
		if (net->RTO == 0) {
			to_ticks = (tcb->asoc.initial_rto * hz)/1000;
		} else {
			to_ticks = (net->RTO * hz)/1000;
		}
		tmr = &net->rxt_timer;
		break;
	case SCTP_TIMER_TYPE_SHUTDOWNGUARD:
		/*
		 * Here we use the endpoints shutdown guard timer
		 * usually about 3 minutes.
		 */
		if (tcb == NULL) {
			return (EFAULT);
		}
		to_ticks = ep->sctp_ep.sctp_timeoutticks[SCTP_TIMER_MAXSHUTDOWN];
		tmr = &tcb->asoc.shut_guard_timer;
		break;
	case SCTP_TIMER_TYPE_ASCONF:
		/*
		 * Here the timer comes from the ep
		 * but its value is from the RTO.
		 */
		if ((tcb == NULL) && (net == NULL)) {
			return (EFAULT);
		}
		if (net->RTO == 0) {
			to_ticks = (tcb->asoc.initial_rto * hz)/1000;
		} else {
			to_ticks = (net->RTO * hz)/1000;
		}
		tmr = &tcb->asoc.asconf_timer;
		break;
	case SCTP_TIMER_TYPE_AUTOCLOSE:
		if (tcb == NULL) {
			return (EFAULT);
		}
		if (tcb->asoc.sctp_autoclose_ticks == 0) {
			/* Really an error since tcb is NOT set to autoclose */
			return (0);
		}
		to_ticks = tcb->asoc.sctp_autoclose_ticks;
		tmr = &tcb->asoc.autoclose_timer;
		break;
	default:
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_TIMER1) {
			printf("sctp_timer_start:Unknown timer type %d\n",
			       t_type);
		}
#endif /* SCTP_DEBUG */
		return (EFAULT);
		break;
	};
	if (callout_pending(&tmr->timer)) {
		/*
		 * we do NOT allow you to have it already running.
		 * if it is we leave the current one up unchanged
		 */
		return (EALREADY);
	}
	if ((to_ticks <= 0) || (tmr == NULL)) {
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_TIMER1) {
			printf("sctp_timer_start:%d:software error to_ticks:%d tmr:%x not set ??\n",
			       t_type, to_ticks, (u_int)tmr);
		}
#endif /* SCTP_DEBUG */
		return (EFAULT);
	}
	/* At this point we can proceed */
	tmr->type = t_type;
	tmr->ep = (void *)ep;
	tmr->tcb = (void *)tcb;
	tmr->net = (void *)net;
	callout_reset(&tmr->timer, to_ticks, sctp_timeout_handler, tmr);
	return (0);
}

int
sctp_timer_stop(int t_type,
		struct sctp_inpcb *ep,
		struct sctp_tcb *tcb,
		struct sctp_nets *net)
{
	struct sctp_timer *tmr;

	if (ep == NULL)
		return (EFAULT);

	tmr = NULL;
	switch (t_type) {
	case SCTP_TIMER_TYPE_SEND:
		if ((tcb == NULL) || (net == NULL)) {
			return (EFAULT);
		}
		tmr = &net->rxt_timer;
		break;
	case SCTP_TIMER_TYPE_INIT:
		if ((tcb == NULL) || (net == NULL)) {
			return (EFAULT);
		}
		tmr = &net->rxt_timer;
		break;
	case SCTP_TIMER_TYPE_RECV:
		if (tcb == NULL) {
			return (EFAULT);
		}
		tmr = &tcb->asoc.dack_timer;
		break;
	case SCTP_TIMER_TYPE_SHUTDOWN:
		if ((tcb == NULL) || (net == NULL)) {
			return (EFAULT);
		}
		tmr = &net->rxt_timer;
		break;
	case SCTP_TIMER_TYPE_HEARTBEAT:
		if (tcb == NULL) {
			return (EFAULT);
		}
		tmr = &tcb->asoc.hb_timer;
		break;
	case SCTP_TIMER_TYPE_COOKIE:
		if ((tcb == NULL) || (net == NULL)) {
			return (EFAULT);
		}
		tmr = &net->rxt_timer;
		break;
	case SCTP_TIMER_TYPE_NEWCOOKIE:
		/* nothing needed but the endpoint here */
		tmr = &ep->sctp_ep.signature_change;
		break;
	case SCTP_TIMER_TYPE_PATHMTURAISE:
		if (tcb == NULL) {
			return (EFAULT);
		}
		tmr = &tcb->asoc.pmtu;
		break;
	case SCTP_TIMER_TYPE_SHUTDOWNACK:
		if ((tcb == NULL) || (net == NULL)) {
			return (EFAULT);
		}
		tmr = &net->rxt_timer;
		break;
	case SCTP_TIMER_TYPE_SHUTDOWNGUARD:
		if (tcb == NULL) {
			return (EFAULT);
		}
		tmr = &tcb->asoc.shut_guard_timer;
		break;
	case SCTP_TIMER_TYPE_ASCONF:
		if (tcb == NULL) {
			return (EFAULT);
		}
		tmr = &tcb->asoc.asconf_timer;
		break;
	case SCTP_TIMER_TYPE_AUTOCLOSE:
		if (tcb == NULL) {
			return (EFAULT);
		}
		tmr = &tcb->asoc.autoclose_timer;
		break;
	default:
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_TIMER1) {
			printf("sctp_timer_stop:Unknown timer type %d\n",
			       t_type);
		}
#endif /* SCTP_DEBUG */
		break;
	};
	if (tmr->type != t_type) {
		/* Ok we have a timer that is under
		 * joint use. Cookie timer per chance with
		 * the SEND timer. We therefore are NOT
		 * running the timer that the caller wants
		 * stopped. so just return.
		 */
		return (0);
	}
	if (tmr == NULL)
		return (EFAULT);

	callout_stop(&tmr->timer);
	return (0);
}

#ifdef SCTP_USE_ADLER32
static
unsigned int update_adler32(u_int32_t adler,
			    u_int8_t *buf,
			    int len)
{
	u_int32_t s1 = adler & 0xffff;
	u_int32_t s2 = (adler >> 16) & 0xffff;
	int n;

	for (n = 0; n < len; n++, buf++) {
		/* s1 = (s1 + buf[n]) % BASE */
		/* first we add */
		s1 = (s1 + *buf);
		/*
		 * now if we need to, we do a mod by subtracting. It seems
		 * a bit faster since I really will only ever do one subtract
		 * at the MOST, since buf[n] is a max of 255.
		 */
		if (s1 >= SCTP_ADLER32_BASE) {
			s1 -= SCTP_ADLER32_BASE;
		}
		/* s2 = (s2 + s1) % BASE */
		/* first we add */
		s2 = (s2 + s1);
		/*
		 * again, it is more efficent (it seems) to subtract since
		 * the most s2 will ever be is (BASE-1 + BASE-1) in the worse
		 * case. This would then be (2 * BASE) - 2, which will still
		 * only do one subtract. On Intel this is much better to do
		 * this way and avoid the divide. Have not -pg'd on sparc.
		 */
		if (s2 >= SCTP_ADLER32_BASE) {
			s2 -= SCTP_ADLER32_BASE;
		}
	}
	/* Return the adler32 of the bytes buf[0..len-1] */
	return ((s2 << 16) + s1);
}
#endif /* SCTP_USE_ADLER32 */

u_int32_t
sctp_calculate_sum(m, pktlen, offset)
     struct mbuf *m;
     int32_t *pktlen;
     u_int32_t offset;

{
	/*
	 * given a mbuf chain with a packetheader offset by 'offset'
	 * pointing at a sctphdr (with csum set to 0) go through
	 * the chain of m_next's and calculate the SCTP checksum.
	 * This is currently Adler32 but will change to CRC32x
	 * soon. Also has a side bonus calculate the total length
	 * of the mbuf chain.
	 * Note: if offset is greater than the total mbuf length,
	 * checksum=1, pktlen=0 is returned (ie. no real error code)
	 */
	register int32_t tlen=0;
#ifdef SCTP_USE_ADLER32
	register unsigned int base = 1L;
#else
	register u_int32_t base = 0xffffffff;
#endif /* SCTP_USE_ADLER32 */
	register struct mbuf *at;
	at = m;
	/* find the correct mbuf and offset into mbuf */
	while ((at != NULL) && (offset > at->m_len)) {
		offset -= at->m_len;	/* update remaining offset left */
		at = at->m_next;
	}

	while (at != NULL) {
#ifdef SCTP_USE_ADLER32
		base = update_adler32(base, at->m_data + offset,
		    at->m_len - offset);
#else
		base = update_crc32(base, at->m_data + offset,
		    at->m_len - offset);
#endif /* SCTP_USE_ADLER32 */
		tlen += at->m_len - offset;
		/* we only offset once into the first mbuf */
		if (offset) {
			offset = 0;
		}
		at = at->m_next;
	}
	if (pktlen != NULL) {
		*pktlen = tlen;
	}
#ifdef SCTP_USE_ADLER32
	/* Adler32 */
	base = htonl(base);
#else
	/* CRC-32c */
	base = sctp_csum_finalize(base);
#endif
	return (base);
}

void
sctp_mtu_size_reset(struct sctp_inpcb *ep,
		    struct sctp_association *asoc, u_long mtu)
{
	/*
	 * Reset the P-MTU size on this association, this involves changing
	 * the asoc MTU, going through ANY chunk+overhead larger than mtu
	 * to allow the DF flag to be cleared.
	 */
	struct sctp_tmit_chunk *chk;
	struct sctp_stream_out *strm;
	int eff_mtu;
	asoc->smallest_mtu = mtu;
	if (ep->sctp_frag_point > mtu)
		ep->sctp_frag_point = mtu;

	eff_mtu = mtu - SCTP_MAX_OVERHEAD;

	/* Now mark any chunks that need to let IP fragment */
	TAILQ_FOREACH(strm, &asoc->out_wheel, next_spoke) {
		TAILQ_FOREACH(chk, &strm->outqueue, sctp_next) {
			if (chk->send_size > eff_mtu) {
				chk->flags &= SCTP_DONT_FRAGMENT;
				chk->flags |= CHUNK_FLAGS_FRAGMENT_OK;
			}
		}
	}
	TAILQ_FOREACH(chk, &asoc->send_queue, sctp_next) {
		if (chk->send_size > eff_mtu) {
			chk->flags &= SCTP_DONT_FRAGMENT;
			chk->flags |= CHUNK_FLAGS_FRAGMENT_OK;
		}
	}
	TAILQ_FOREACH(chk, &asoc->sent_queue, sctp_next) {
		if (chk->send_size > eff_mtu) {
			chk->flags &= SCTP_DONT_FRAGMENT;
			chk->flags |= CHUNK_FLAGS_FRAGMENT_OK;
		}
	}
}


/*
 * given an association and starting time of the current RTT period
 * return RTO in number of usecs
 * net should point to the current network
 */
u_int32_t
sctp_calculate_rto(struct sctp_tcb *stcb,
		   struct sctp_association *assoc,
		   struct sctp_nets *net,
		   struct timeval *old)
{
	/*
	 * given an association and the starting time of the current RTT
	 * period (in value1/value2) return RTO in number of usecs.
	 */
	int calc_time = 0;
	int o_calctime;
	int new_rto = 0;
	struct timeval now;

	/************************/
	/* 1. calculate new RTT */
	/************************/
	/* get the current time */
	SCTP_GETTIME_TIMEVAL(&now);
	/* compute the RTT value */
	if ((u_long)now.tv_sec > (u_long)old->tv_sec) {
		calc_time = ((u_long)now.tv_sec - (u_long)old->tv_sec) * 1000;
		if ((u_long)now.tv_usec > (u_long)old->tv_usec) {
			calc_time += (((u_long)now.tv_usec -
				       (u_long)old->tv_usec)/1000);
		} else if ((u_long)now.tv_usec < (u_long)old->tv_usec) {
			/* Borrow 1,000ms from current calculation */
			calc_time -= 1000;
			/* Add in the slop over */
			calc_time += ((int)now.tv_usec/1000);
			/* Add in the pre-second ms's */
			calc_time += (((int)1000000 - (int)old->tv_usec)/1000);
		}
	} else if ((u_long)now.tv_sec == (u_long)old->tv_sec) {
		if ((u_long)now.tv_usec > (u_long)old->tv_usec) {
			calc_time = ((u_long)now.tv_usec -
				     (u_long)old->tv_usec)/1000;
		} else if ((u_long)now.tv_usec < (u_long)old->tv_usec) {
			/* impossible .. garbage in nothing out */
			return (((net->lastsa >> 2) + net->lastsv) >> 1);
		} else {
			/* impossible .. garbage in nothing out */
			return (((net->lastsa >> 2) + net->lastsv) >> 1);
		}
	} else {
		/* Clock wrapped? */
		return (((net->lastsa >> 2) + net->lastsv) >> 1);
	}
	/***************************/
	/* 2. update RTTVAR & SRTT */
	/***************************/
	/*	if (net->lastsv || net->lastsa) {*/
	/* per Section 5.3.1 C3 in SCTP */
	/*		net->lastsv = (int) 	*//* RTTVAR */
	/*			(((double)(1.0 - 0.25) * (double)net->lastsv) +
				(double)(0.25 * (double)abs(net->lastsa - calc_time)));
				net->lastsa = (int) */	/* SRTT */
	/*(((double)(1.0 - 0.125) * (double)net->lastsa) +
	  (double)(0.125 * (double)calc_time));
	  } else {
	*//* the first RTT calculation, per C2 Section 5.3.1 */
	/*		net->lastsa = calc_time;	*//* SRTT */
	/*		net->lastsv = calc_time / 2;	*//* RTTVAR */
	/*	}*/
	/* if RTTVAR goes to 0 you set to clock grainularity */
	/*	if (net->lastsv == 0) {
		net->lastsv = SCTP_CLOCK_GRANULARITY;
		}
		new_rto = net->lastsa + 4 * net->lastsv;
	*/
	o_calctime = calc_time;
	/* this is Van Jacobson's integer version */
	if (net->RTO) {
		calc_time -= (net->lastsa >> 3);
		net->lastsa += calc_time;
		if (calc_time < 0) {
			calc_time = -calc_time;
		}
		calc_time -= (net->lastsv >> 2);
		net->lastsv += calc_time;
		if (net->lastsv == 0) {
			net->lastsv = SCTP_CLOCK_GRANULARITY;
		}
	} else {
		/* First RTO measurment */
		net->lastsa = calc_time;
		net->lastsv = calc_time >> 1;
	}
	new_rto = ((net->lastsa >> 2) + net->lastsv) >> 1;
	if (new_rto > SCTP_SAT_NETWORK_MIN) {
		stcb->asoc.sat_network = 1;
	}
	/* bound it, per C6/C7 in Section 5.3.1 */
	if (new_rto < stcb->sctp_ep->sctp_ep.sctp_minrto) {
		new_rto = stcb->sctp_ep->sctp_ep.sctp_minrto;
	}
	if (new_rto > stcb->sctp_ep->sctp_ep.sctp_maxrto) {
		new_rto = stcb->sctp_ep->sctp_ep.sctp_maxrto;
	}
	/* we are now returning the RTT Smoothed */
	return ((u_int32_t)new_rto);
}


/*
 * return a pointer to a contiguous piece of data from the given
 * mbuf chain starting at 'off' for 'len' bytes.  If the desired
 * piece spans more than one mbuf, a copy is made at 'ptr'.
 * caller must ensure that the buffer size is >= 'len'
 * returns NULL if there there isn't 'len' bytes in the chain.
 */
caddr_t
sctp_m_getptr(struct mbuf *m, int off, int len, u_int8_t *in_ptr)
{
	u_int count;
	u_int8_t *ptr;
	ptr = in_ptr;
	if ((off < 0) || (len <= 0))
		return (NULL);

	/* find the desired start location */
	while ((m != NULL) && (off > 0)) {
		if (off < m->m_len)
			break;
		off -= m->m_len;
		m = m->m_next;
	}
	if (m == NULL)
		return (NULL);

	/* is the current mbuf large enough (eg. contiguous)? */
	if ((m->m_len - off) >= len) {
		return (mtod(m, caddr_t) + off);
	} else {
		/* else, it spans more than one mbuf, so save a temp copy... */
		while ((m != NULL) && (len > 0)) {
			count = min(m->m_len - off, len);
			bcopy(mtod(m, caddr_t) + off, ptr, count);
			len -= count;
			ptr += count;
			off = 0;
			m = m->m_next;
		}
		if ((m == NULL) && (len > 0))
			return (NULL);
		else
			return ((caddr_t)in_ptr);
	}
}


struct sctp_paramhdr *
sctp_get_next_param(struct mbuf *m,
		    int offset,
		    struct sctp_paramhdr *pull,
		    int pull_limit)
{
	/* This just provides a typed signature to Peter's Pull routine */
	return ((struct sctp_paramhdr *)sctp_m_getptr(m, offset, pull_limit,
    	    (u_int8_t *)pull));
}


int
sctp_add_pad_tombuf(struct mbuf *m, int padlen)
{
	/*
	 * add padlen bytes of 0 filled padding to the end of the mbuf.
	 * If padlen is > 3 this routine will fail.
	 */
	u_int8_t *dp;
	int i;
	if (padlen > 3) {
		return (ENOBUFS);
	}
	if (M_TRAILINGSPACE(m)) {
		/*
		 * The easy way.
		 * We hope the majority of the time we hit here :)
		 */
		dp = (u_int8_t *)(mtod(m, caddr_t) + m->m_len);
		m->m_len += padlen;
	} else {
		/* Hard way we must grow the mbuf */
		struct mbuf *tmp;
		MGET(tmp, M_DONTWAIT, MT_DATA);
		if (tmp == NULL) {
			/* Out of space GAK! we are in big trouble. */
			return (ENOSPC);
		}
		/* setup and insert in middle */
		tmp->m_next = m->m_next;
		tmp->m_len = padlen;
		m->m_next = tmp;
		dp = mtod(tmp, u_int8_t *);
	}
	/* zero out the pad */
	for (i=  0; i < padlen; i++) {
		*dp = 0;
		dp++;
	}
	return (0);
}

int
sctp_pad_lastmbuf(struct mbuf *m, int padval)
{
	/* find the last mbuf in chain and pad it */
	struct mbuf *m_at;
	m_at = m;
	while (m_at) {
		if (m_at->m_next == NULL) {
			return (sctp_add_pad_tombuf(m_at, padval));
		}
		m_at = m_at->m_next;
	}
	return (EFAULT);
}

#ifndef __FreeBSD__
/*
 * Don't know why but without this I get an unknown reference when
 * compiling NetBSD... hmm
 */
extern void in6_sin_2_v4mapsin6(struct sockaddr_in *, struct sockaddr_in6 *);
#endif

extern int sctp_deliver_data(struct sctp_tcb *, struct sctp_association *,
	struct sctp_tmit_chunk *);

static void
sctp_notify_assoc_change(u_int32_t event, struct sctp_tcb *stcb,
    u_int32_t error)
{
	struct mbuf *m_notify;
	struct sctp_assoc_change *sac;
	struct sockaddr *to;
	struct sockaddr_in6 sin6, lsa6;

	 /* First if we are are going down dump everything we
	  * can to the socket rcv queue.
	  */
	if ((event == SCTP_SHUTDOWN_COMP) ||
	   (event == SCTP_COMM_LOST)) {
		sctp_deliver_data(stcb, &stcb->asoc, NULL);
	}

#ifdef SCTP_TCP_MODEL_SUPPORT
	/*
	 * For TCP model AND UDP connected sockets we will send
	 * an error up when an ABORT comes in.
	 */
	if (((stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_TCPTYPE) ||
	     (stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_IN_TCPPOOL)) &&
	    (event == SCTP_COMM_LOST)) {
		stcb->sctp_socket->so_error = ECONNRESET;
		/* Wake ANY sleepers */
		sowwakeup(stcb->sctp_socket);
		sorwakeup(stcb->sctp_socket);
	}
#if 0
	if ((event == SCTP_COMM_UP) && 
	    (stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_TCPTYPE) &&
 	    (stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_CONNECTED)) {
		 soisconnected(stcb->sctp_socket);
	}
#endif
#endif /* SCTP_TCP_MODEL_SUPPORT */

	if (!(stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_RECVASSOCEVNT)) {
		/* event not enabled */
		return;
	}
	MGETHDR(m_notify, M_DONTWAIT, MT_DATA);
	if (m_notify == NULL)
		/* no space left */
		return;
	sac = mtod(m_notify, struct sctp_assoc_change *);
	sac->sac_type = SCTP_ASSOC_CHANGE;
	sac->sac_flags = 0;
	sac->sac_length = sizeof(struct sctp_assoc_change);
	sac->sac_state = event;
	sac->sac_error = error;
	/* XXX verify these stream counts */
	sac->sac_outbound_streams = stcb->asoc.streamoutcnt;
	sac->sac_inbound_streams = stcb->asoc.streamincnt;
	sac->sac_assoc_id = (sctp_assoc_t)stcb;

	m_notify->m_flags |= M_EOR | M_NOTIFICATION;
	m_notify->m_pkthdr.len = sizeof(struct sctp_assoc_change);
	m_notify->m_pkthdr.rcvif = 0;
	m_notify->m_len = sizeof(struct sctp_assoc_change);
	m_notify->m_next = NULL;

	/* append to socket */
	to = (struct sockaddr *)&stcb->asoc.primary_destination->ra._l_addr;
	if ((stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_NEEDS_MAPPED_V4) &&
	    (to->sa_family == AF_INET)) {
		in6_sin_2_v4mapsin6((struct sockaddr_in *)to, &sin6);
		to = (struct sockaddr *)&sin6;
	}
	/* check and strip embedded scope junk */
	to = (struct sockaddr *)sctp_recover_scope((struct sockaddr_in6 *)to,
						   &lsa6);
	/*
	 * We need to always notify comm changes.
	 * if (sctp_sbspace(&stcb->sctp_socket->so_rcv) < m_notify->m_len) {
	 * 	m_freem(m_notify);
	 *	return;
	 * }
	*/
	if (stcb->sctp_ep->sctp_vtag_last == 0) {
		stcb->sctp_ep->sctp_vtag_last = stcb->asoc.my_vtag;
	}
	if (sbappendaddr_nocheck(&stcb->sctp_socket->so_rcv,
	    to, m_notify, NULL, stcb->asoc.my_vtag) == 0) {
		/* not enough room */
		m_freem(m_notify);
		return;
	}
	if (sctp_add_to_socket_q(stcb->sctp_ep, stcb)) {
		stcb->asoc.my_rwnd_control_len += sizeof(struct mbuf);
	}
	/* Wake up any sleeper */
	sctp_sorwakeup(stcb->sctp_ep, stcb->sctp_socket);
	sctp_sowwakeup(stcb->sctp_ep, stcb->sctp_socket);
}

static void
sctp_notify_peer_addr_change(struct sctp_tcb *stcb, uint32_t state,
    struct sockaddr *sa, uint32_t error)
{
	struct mbuf *m_notify;
	struct sctp_paddr_change *spc;
	struct sockaddr *to;
	struct sockaddr_in6 sin6, lsa6;

	if (!(stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_RECVPADDREVNT))
		/* event not enabled */
		return;

	MGETHDR(m_notify, M_DONTWAIT, MT_DATA);
	if (m_notify == NULL)
		return;

	spc = mtod(m_notify, struct sctp_paddr_change *);
	spc->spc_type = SCTP_PEER_ADDR_CHANGE;
	spc->spc_flags = 0;
	spc->spc_length = sizeof(struct sctp_paddr_change);
	if (sa->sa_family == AF_INET) {
		memcpy(&spc->spc_aaddr, sa, sizeof(struct sockaddr_in));
	} else {
		memcpy(&spc->spc_aaddr, sa, sizeof(struct sockaddr_in6));
	}
	spc->spc_state = state;
	spc->spc_error = error;
	spc->spc_assoc_id = (sctp_assoc_t)stcb;

	m_notify->m_flags |= M_EOR | M_NOTIFICATION;
	m_notify->m_pkthdr.len = sizeof(struct sctp_paddr_change);
	m_notify->m_pkthdr.rcvif = 0;
	m_notify->m_len = sizeof(struct sctp_paddr_change);
	m_notify->m_next = NULL;

	to = (struct sockaddr *)(struct sockaddr *)&stcb->asoc.primary_destination->ra._l_addr;
	if ((stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_NEEDS_MAPPED_V4) &&
	    (to->sa_family == AF_INET)) {
		in6_sin_2_v4mapsin6((struct sockaddr_in *)to, &sin6);
		to = (struct sockaddr *)&sin6;
	}
	/* check and strip embedded scope junk */
	to = (struct sockaddr *)sctp_recover_scope((struct sockaddr_in6 *)to,
						   &lsa6);

	if (sctp_sbspace(&stcb->sctp_socket->so_rcv) < m_notify->m_len) {
		m_freem(m_notify);
		return;
	}
	/* append to socket */
	if (stcb->sctp_ep->sctp_vtag_last == 0) {
		stcb->sctp_ep->sctp_vtag_last = stcb->asoc.my_vtag;
	}
	if (!sbappendaddr_nocheck(&stcb->sctp_socket->so_rcv, to,
	    m_notify, NULL, stcb->asoc.my_vtag)) {
		/* not enough room */
		m_freem(m_notify);
		return;
	}
	if (sctp_add_to_socket_q(stcb->sctp_ep, stcb)) {
		stcb->asoc.my_rwnd_control_len += sizeof(struct mbuf);
	}
	sctp_sorwakeup(stcb->sctp_ep, stcb->sctp_socket);
}


static void
sctp_notify_send_failed(struct sctp_tcb *stcb, u_int32_t error,
			struct sctp_tmit_chunk *chk)
{
	struct mbuf *m_notify;
	struct sctp_send_failed *ssf;
	struct sockaddr_in6 sin6, lsa6;
	struct sockaddr *to;
	int length;

	if (!(stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_RECVSENDFAILEVNT))
		/* event not enabled */
		return;

	length = sizeof(struct sctp_send_failed) + chk->send_size;
	MGETHDR(m_notify, M_DONTWAIT, MT_DATA);
	if (m_notify == NULL)
		/* no space left */
		return;
	ssf = mtod(m_notify, struct sctp_send_failed *);
	ssf->ssf_type = SCTP_SEND_FAILED;
	if (error == SCTP_NOTIFY_DATAGRAM_UNSENT)
		ssf->ssf_flags = SCTP_DATA_UNSENT;
	else
		ssf->ssf_flags = SCTP_DATA_SENT;
	ssf->ssf_length = length;
	ssf->ssf_error = error;
	/* not exactly what the user sent in, but should be close :) */
	ssf->ssf_info.sinfo_stream = chk->rec.data.stream_number;
	ssf->ssf_info.sinfo_ssn = chk->rec.data.stream_seq;
	ssf->ssf_info.sinfo_flags = chk->rec.data.rcv_flags;
	ssf->ssf_info.sinfo_ppid = chk->rec.data.payloadtype;
	ssf->ssf_info.sinfo_context = chk->rec.data.context;
	ssf->ssf_info.sinfo_assoc_id = (sctp_assoc_t)stcb;
	ssf->ssf_assoc_id = (sctp_assoc_t)stcb;
	m_notify->m_next = chk->data;
	if (m_notify->m_next == NULL)
		m_notify->m_flags |= M_EOR | M_NOTIFICATION;
	else {
		struct mbuf *m;
		m_notify->m_flags |= M_NOTIFICATION;
		m = m_notify;
		while (m->m_next != NULL)
			m = m->m_next;
		m->m_flags |= M_EOR;
	}
	m_notify->m_pkthdr.len = length;
	m_notify->m_pkthdr.rcvif = 0;
	m_notify->m_len = sizeof(struct sctp_send_failed);

	/* Steal off the mbuf */
	chk->data = NULL;
	to = (struct sockaddr *)(struct sockaddr *)&stcb->asoc.primary_destination->ra._l_addr;
	if ((stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_NEEDS_MAPPED_V4) &&
	    (to->sa_family == AF_INET)) {
		in6_sin_2_v4mapsin6((struct sockaddr_in *)to, &sin6);
		to = (struct sockaddr *)&sin6;
	}
	/* check and strip embedded scope junk */
	to = (struct sockaddr *)sctp_recover_scope((struct sockaddr_in6 *)to,
						   &lsa6);

	if (sctp_sbspace(&stcb->sctp_socket->so_rcv) < m_notify->m_len) {
		m_freem(m_notify);
		return;
	}

	/* append to socket */
	if (stcb->sctp_ep->sctp_vtag_last == 0) {
		stcb->sctp_ep->sctp_vtag_last = stcb->asoc.my_vtag;
	}

	if (!sbappendaddr_nocheck(&stcb->sctp_socket->so_rcv, to,
	    m_notify, NULL, stcb->asoc.my_vtag)) {
		/* not enough room */
		m_freem(m_notify);
		return;
	}
	if (sctp_add_to_socket_q(stcb->sctp_ep, stcb)) {
		stcb->asoc.my_rwnd_control_len += sizeof(struct mbuf);
	}
	sctp_sorwakeup(stcb->sctp_ep, stcb->sctp_socket);
}

static void
sctp_notify_adaption_layer(struct sctp_tcb *stcb,
			   u_int32_t error)
{
	struct mbuf *m_notify;
	struct sctp_adaption_event *sai;
	struct sockaddr_in6 sin6, lsa6;
	struct sockaddr *to;

	if (!(stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_ADAPTIONEVNT))
		/* event not enabled */
		return;

	MGETHDR(m_notify, M_DONTWAIT, MT_DATA);
	if (m_notify == NULL)
		/* no space left */
		return;
	sai = mtod(m_notify, struct sctp_adaption_event *);
	sai->sai_type = SCTP_ADAPTION_INDICATION;
	sai->sai_flags = 0;
	sai->sai_length = sizeof(struct sctp_adaption_event);
	sai->sai_adaption_ind = error;
	sai->sai_assoc_id = (sctp_assoc_t)stcb;

	m_notify->m_flags |= M_EOR | M_NOTIFICATION;
	m_notify->m_pkthdr.len = sizeof(struct sctp_adaption_event);
	m_notify->m_pkthdr.rcvif = 0;
	m_notify->m_len = sizeof(struct sctp_adaption_event);
	m_notify->m_next = NULL;

	to = (struct sockaddr *)(struct sockaddr *)&stcb->asoc.primary_destination->ra._l_addr;
	if ((stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_NEEDS_MAPPED_V4) &&
	    (to->sa_family == AF_INET)) {
		in6_sin_2_v4mapsin6((struct sockaddr_in *)to, &sin6);
		to = (struct sockaddr *)&sin6;
	}
	/* check and strip embedded scope junk */
	to = (struct sockaddr *)sctp_recover_scope((struct sockaddr_in6 *)to,
						   &lsa6);
	if (sctp_sbspace(&stcb->sctp_socket->so_rcv) < m_notify->m_len) {
		m_freem(m_notify);
		return;
	}
	/* append to socket */
	if (stcb->sctp_ep->sctp_vtag_last == 0) {
		stcb->sctp_ep->sctp_vtag_last = stcb->asoc.my_vtag;
	}

	if (!sbappendaddr_nocheck(&stcb->sctp_socket->so_rcv, to,
	    m_notify, NULL, stcb->asoc.my_vtag)) {
		/* not enough room */
		m_freem(m_notify);
		return;
	}
	if (sctp_add_to_socket_q(stcb->sctp_ep, stcb)) {
		stcb->asoc.my_rwnd_control_len += sizeof(struct mbuf);
	}
	sctp_sorwakeup(stcb->sctp_ep, stcb->sctp_socket);
}

static void
sctp_notify_partial_delivery_indication(struct sctp_tcb *stcb,
					u_int32_t error)
{
	struct mbuf *m_notify;
	struct sctp_pdapi_event *pdapi;
	struct sockaddr_in6 sin6, lsa6;
	struct sockaddr *to;

	if (!(stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_PDAPIEVNT))
		/* event not enabled */
		return;

	MGETHDR(m_notify, M_DONTWAIT, MT_DATA);
	if (m_notify == NULL)
		/* no space left */
		return;
	pdapi = mtod(m_notify, struct sctp_pdapi_event *);
	pdapi->pdapi_type = SCTP_PARTIAL_DELIVERY_EVENT;
	pdapi->pdapi_flags = 0;
	pdapi->pdapi_length = sizeof(struct sctp_pdapi_event);
	pdapi->pdapi_indication = error;
	pdapi->pdapi_assoc_id = (sctp_assoc_t)stcb;

	m_notify->m_flags |= M_EOR | M_NOTIFICATION;
	m_notify->m_pkthdr.len = sizeof(struct sctp_pdapi_event);
	m_notify->m_pkthdr.rcvif = 0;
	m_notify->m_len = sizeof(struct sctp_pdapi_event);
	m_notify->m_next = NULL;

	to = (struct sockaddr *)(struct sockaddr *)&stcb->asoc.primary_destination->ra._l_addr;
	if ((stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_NEEDS_MAPPED_V4) &&
	    (to->sa_family == AF_INET)) {
		in6_sin_2_v4mapsin6((struct sockaddr_in *)to, &sin6);
		to = (struct sockaddr *)&sin6;
	}
	/* check and strip embedded scope junk */
	to = (struct sockaddr *)sctp_recover_scope((struct sockaddr_in6 *)to,
						   &lsa6);
	if (sctp_sbspace(&stcb->sctp_socket->so_rcv) < m_notify->m_len) {
		m_freem(m_notify);
		return;
	}
	/* append to socket */
	if (stcb->sctp_ep->sctp_vtag_last == 0) {
		stcb->sctp_ep->sctp_vtag_last = stcb->asoc.my_vtag;
	}

	if (!sbappendaddr_nocheck(&stcb->sctp_socket->so_rcv, to,
	    m_notify, NULL, stcb->asoc.my_vtag)) {
		/* not enough room */
		m_freem(m_notify);
		return;
	}
	if (sctp_add_to_socket_q(stcb->sctp_ep, stcb)) {
		stcb->asoc.my_rwnd_control_len += sizeof(struct mbuf);
	}
	sctp_sorwakeup(stcb->sctp_ep, stcb->sctp_socket);
}

static void
sctp_notify_shutdown_event(struct sctp_tcb *stcb)
{
	struct mbuf *m_notify;
	struct sctp_shutdown_event *sse;
	struct sockaddr_in6 sin6, lsa6;
	struct sockaddr *to;

#ifdef SCTP_TCP_MODEL_SUPPORT
	/*
	 * For TCP model AND UDP connected sockets we will send
	 * an error up when an SHUTDOWN completes
	 */
	if ((stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_TCPTYPE) ||
	    (stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_IN_TCPPOOL)) {
		/* mark socket closed for read/write and wakeup! */
		socantrcvmore(stcb->sctp_socket);
		socantsendmore(stcb->sctp_socket);
	}
#endif /* SCTP_TCP_MODEL_SUPPORT */

	if (!(stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_RECVSHUTDOWNEVNT))
		/* event not enabled */
		return;

	MGETHDR(m_notify, M_DONTWAIT, MT_DATA);
	if (m_notify == NULL)
		/* no space left */
		return;
	sse = mtod(m_notify, struct sctp_shutdown_event *);
	sse->sse_type = SCTP_SHUTDOWN_EVENT;
	sse->sse_flags = 0;
	sse->sse_length = sizeof(struct sctp_shutdown_event);
	sse->sse_assoc_id = (sctp_assoc_t)stcb;

	m_notify->m_flags |= M_EOR | M_NOTIFICATION;
	m_notify->m_pkthdr.len = sizeof(struct sctp_shutdown_event);
	m_notify->m_pkthdr.rcvif = 0;
	m_notify->m_len = sizeof(struct sctp_shutdown_event);
	m_notify->m_next = NULL;

	to = (struct sockaddr *)(struct sockaddr *)&stcb->asoc.primary_destination->ra._l_addr;
	if ((stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_NEEDS_MAPPED_V4) &&
	    (to->sa_family == AF_INET)) {
		in6_sin_2_v4mapsin6((struct sockaddr_in *)to, &sin6);
		to = (struct sockaddr *)&sin6;
	}
	/* check and strip embedded scope junk */
	to = (struct sockaddr *)sctp_recover_scope((struct sockaddr_in6 *)to,
						   &lsa6);
	if (sctp_sbspace(&stcb->sctp_socket->so_rcv) < m_notify->m_len) {
		m_freem(m_notify);
		return;
	}
	/* append to socket */
	if (stcb->sctp_ep->sctp_vtag_last == 0) {
		stcb->sctp_ep->sctp_vtag_last = stcb->asoc.my_vtag;
	}

	if (!sbappendaddr_nocheck(&stcb->sctp_socket->so_rcv, to,
	    m_notify, NULL, stcb->asoc.my_vtag)) {
		/* not enough room */
		m_freem(m_notify);
		return;
	}
	if (sctp_add_to_socket_q(stcb->sctp_ep, stcb)) {
		stcb->asoc.my_rwnd_control_len += sizeof(struct mbuf);
	}
	sctp_sorwakeup(stcb->sctp_ep, stcb->sctp_socket);
}

void
sctp_ulp_notify(u_int32_t notification, struct sctp_tcb *stcb,
		u_int32_t error, void *data)
{
	if (stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_SOCKET_GONE) {
		/* No notifications up when we are in a no socket state */
		return;
	}
	switch (notification) {
	case SCTP_NOTIFY_ASSOC_UP:
		sctp_notify_assoc_change(SCTP_COMM_UP, stcb, error);
		break;
	case SCTP_NOTIFY_ASSOC_DOWN:
		sctp_notify_assoc_change(SCTP_SHUTDOWN_COMP, stcb, error);
		break;
	case SCTP_NOTIFY_INTERFACE_DOWN:
	{
		struct sctp_nets *net;
		net = (struct sctp_nets *)data;
		sctp_notify_peer_addr_change(stcb, SCTP_ADDR_UNREACHABLE,
		    (struct sockaddr *)&net->ra._l_addr, error);
		break;
	}
	case SCTP_NOTIFY_INTERFACE_UP:
	{
		struct sctp_nets *net;
		net = (struct sctp_nets *)data;
		sctp_notify_peer_addr_change(stcb, SCTP_ADDR_REACHABLE,
		    (struct sockaddr *)&net->ra._l_addr, error);
		break;
	}
	case SCTP_NOTIFY_INTERFACE_CONFIRMED:
	{
		struct sctp_nets *net;
		net = (struct sctp_nets *)data;
		sctp_notify_peer_addr_change(stcb, SCTP_ADDR_CONFIRMED,
		    (struct sockaddr *)&net->ra._l_addr, error);
		break;
	}
	case SCTP_NOTIFY_DG_FAIL:
		sctp_notify_send_failed(stcb, error,
		    (struct sctp_tmit_chunk *)data);
		break;
	case SCTP_NOTIFY_ADAPTION_INDICATION:
		/* Here the error is the adaption indication */
		sctp_notify_adaption_layer(stcb, error);
		break;
	case SCTP_NOTIFY_PARTIAL_DELVIERY_INDICATION:
		sctp_notify_partial_delivery_indication(stcb, error);
		break;
	case SCTP_NOTIFY_STRDATA_ERR:
		break;
	case SCTP_NOTIFY_ASSOC_ABORTED:
		sctp_notify_assoc_change(SCTP_COMM_LOST, stcb, error);
		break;
	case SCTP_NOTIFY_PEER_OPENED_STREAM:
		break;
	case SCTP_NOTIFY_STREAM_OPENED_OK:
		break;
	case SCTP_NOTIFY_ASSOC_RESTART:
		sctp_notify_assoc_change(SCTP_RESTART, stcb, error);
		break;
	case SCTP_NOTIFY_HB_RESP:
		break;
	case SCTP_NOTIFY_ASCONF_ADD_IP:
		sctp_notify_peer_addr_change(stcb, SCTP_ADDR_ADDED, data,
		    error);
		break;
	case SCTP_NOTIFY_ASCONF_DELETE_IP:
		sctp_notify_peer_addr_change(stcb, SCTP_ADDR_REMOVED, data,
		    error);
		break;
	case SCTP_NOTIFY_ASCONF_SET_PRIMARY:
		sctp_notify_peer_addr_change(stcb, SCTP_ADDR_MADE_PRIM, data,
		    error);
		break;
	case SCTP_NOTIFY_ASCONF_SUCCESS:
		break;
	case SCTP_NOTIFY_ASCONF_FAILED:
		break;
	case SCTP_NOTIFY_PEER_SHUTDOWN:
		sctp_notify_shutdown_event(stcb);
		break;
	default:
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_UTIL1) {
			printf("NOTIFY: unknown notification %xh (%u)\n",
			    notification, notification);
		}
#endif /* SCTP_DEBUG */
		break;
	} /* end switch */
}

extern struct sctp_epinfo sctppcbinfo;

void
sctp_report_all_outbound(struct sctp_tcb *stcb)
{
	struct sctp_association *asoc;
	struct sctp_stream_out *outs;
	struct sctp_tmit_chunk *chk;

	asoc = &stcb->asoc;

	if (stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_SOCKET_GONE) {
		return;
	}
	/* now through all the gunk freeing chunks */
	TAILQ_FOREACH(outs, &asoc->out_wheel, next_spoke) {
		/* now clean up any chunks here */
		chk = TAILQ_FIRST(&outs->outqueue);
		while (chk) {
			TAILQ_REMOVE(&outs->outqueue, chk, sctp_next);
			sctp_ulp_notify(SCTP_NOTIFY_DG_FAIL, stcb,
			    SCTP_NOTIFY_DATAGRAM_UNSENT, chk);
			if (chk->data) {
				m_freem(chk->data);
				chk->data = NULL;
			}
			if (chk->whoTo)
				sctp_free_remote_addr(chk->whoTo);
			chk->whoTo = NULL;
			chk->asoc = NULL;
			/* Free the chunk */
#if defined(__FreeBSD__)
			zfreei(sctppcbinfo.ipi_zone_chunk, chk);
#endif
#if defined(__NetBSD__) || defined(__OpenBSD__)
			pool_put(&sctppcbinfo.ipi_zone_chunk, chk);
#endif
			sctppcbinfo.ipi_count_chunk--;
			if ((int)sctppcbinfo.ipi_count_chunk < 0) {
				panic("Chunk count is negative");
			}
			sctppcbinfo.ipi_gencnt_chunk++;
			chk = TAILQ_FIRST(&outs->outqueue);
		}
	}
	/* pending send queue SHOULD be empty */
	if (!TAILQ_EMPTY(&asoc->send_queue)) {
		chk = TAILQ_FIRST(&asoc->send_queue);
		while (chk) {
			TAILQ_REMOVE(&asoc->send_queue, chk, sctp_next);
			sctp_ulp_notify(SCTP_NOTIFY_DG_FAIL, stcb, SCTP_NOTIFY_DATAGRAM_UNSENT, chk);
			if (chk->data) {
				m_freem(chk->data);
				chk->data = NULL;
			}
			if (chk->whoTo)
				sctp_free_remote_addr(chk->whoTo);
			chk->whoTo = NULL;
#if defined(__FreeBSD__)
			zfreei(sctppcbinfo.ipi_zone_chunk, chk);
#endif
#if defined(__NetBSD__) || defined(__OpenBSD__)
			pool_put(&sctppcbinfo.ipi_zone_chunk, chk);
#endif
			sctppcbinfo.ipi_count_chunk--;
			if ((int)sctppcbinfo.ipi_count_chunk < 0) {
				panic("Chunk count is negative");
			}
			sctppcbinfo.ipi_gencnt_chunk++;
			chk = TAILQ_FIRST(&asoc->send_queue);
		}
	}
	/* sent queue SHOULD be empty */
	if (!TAILQ_EMPTY(&asoc->sent_queue)) {
		chk = TAILQ_FIRST(&asoc->sent_queue);
		while (chk) {
			TAILQ_REMOVE(&asoc->sent_queue, chk, sctp_next);
			sctp_ulp_notify(SCTP_NOTIFY_DG_FAIL, stcb,
			    SCTP_NOTIFY_DATAGRAM_SENT, chk);
			if (chk->data) {
				m_freem(chk->data);
				chk->data = NULL;
			}
			if (chk->whoTo)
				sctp_free_remote_addr(chk->whoTo);
			chk->whoTo = NULL;
#if defined(__FreeBSD__)
			zfreei(sctppcbinfo.ipi_zone_chunk, chk);
#endif
#if defined(__NetBSD__) || defined(__OpenBSD__)
			pool_put(&sctppcbinfo.ipi_zone_chunk, chk);
#endif
			sctppcbinfo.ipi_count_chunk--;
			if ((int)sctppcbinfo.ipi_count_chunk < 0) {
				panic("Chunk count is negative");
			}
			sctppcbinfo.ipi_gencnt_chunk++;
			chk = TAILQ_FIRST(&asoc->sent_queue);
		}
	}
}

void
sctp_abort_notification(struct sctp_tcb *stcb, int error)
{
	if (stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_SOCKET_GONE) {
	    return;
	}
	/* Tell them we lost the asoc */
	sctp_ulp_notify(SCTP_NOTIFY_ASSOC_ABORTED, stcb, error, NULL);
	sctp_report_all_outbound(stcb);
}

void
sctp_abort_association(struct sctp_inpcb *inp, struct sctp_tcb *stcb,
    struct mbuf *m, int iphlen, struct mbuf *operr)
{
	struct ip *iph;
	struct ip6_hdr *ip6;
	struct sctphdr *sh;
	u_int32_t vtag;

	iph = mtod(m, struct ip *);
	sh = (struct sctphdr *)((caddr_t)iph + iphlen);
	vtag = 0;
	if (stcb != NULL) {
		/* We have a TCB to abort, send notification too */
		vtag = stcb->asoc.peer_vtag;
		sctp_abort_notification(stcb, 0);
	}
	if (iph->ip_v == IPVERSION) {
		sctp_send_abort(m, iph, sh, iphlen, vtag, operr);
	} else {
		ip6 = mtod(m, struct ip6_hdr *);
		sctp6_send_abort(m, ip6, sh, iphlen, vtag, operr);
	}
	if (stcb != NULL) {
		/* Ok, now lets free it */
		sctp_free_assoc(inp, stcb);
	}
}

void
sctp_abort_an_association(struct sctp_inpcb *inp, struct sctp_tcb *stcb,
    int error, struct mbuf *operr)
{
	u_int32_t vtag;

	if (stcb == NULL) {
		/* Got to have a TCB */
		return;
	}
	vtag = stcb->asoc.peer_vtag;
	/* notify the ulp */
	if ((inp->sctp_flags & SCTP_PCB_FLAGS_SOCKET_GONE) == 0)
		sctp_abort_notification(stcb, error);
	/* notify the peer */
	sctp_send_abort_tcb(stcb, operr);
	/* now free the asoc */
	sctp_free_assoc(inp, stcb);
	if (inp->sctp_flags & SCTP_PCB_FLAGS_SOCKET_GONE) {
		sctp_inpcb_free(inp,0);
	}
}

void
sctp_handle_ootb(struct sctp_inpcb *ep, struct mbuf *m, int iphlen, int offset,
    int length, struct mbuf *operr)
{
	struct sctp_chunkhdr *ch;
	struct sctphdr *sctphdr;
	struct sockaddr_in sin;
	struct sockaddr_in6 sin6;
	struct sockaddr *to;

	int ch_len;
	struct ip *iph;
	struct ip6_hdr *ip6h;

	u_int8_t chunk_buf[128];
	/* Generate a TO address for future reference */
	sctphdr = (struct sctphdr *)(mtod(m, caddr_t) + iphlen);
	iph = mtod(m, struct ip *);
	ip6h = mtod(m, struct ip6_hdr *);
	if (iph->ip_v == IPVERSION) {
		/* form a sockaddr_in to send to. */
		to = (struct sockaddr *)&sin;
		sin.sin_len = sizeof(sin);
		sin.sin_family = AF_INET;
		sin.sin_port = sctphdr->src_port;
		sin.sin_addr = iph->ip_src;
	} else {
		/* form a sockaddr_in6 to send to. */
		to = (struct sockaddr *)&sin6;
		sin6.sin6_len = sizeof(sin6);
		sin6.sin6_family = AF_INET6;
		sin6.sin6_port = sctphdr->src_port;
		sin6.sin6_addr = ip6h->ip6_src;
	}
	ch = (struct sctp_chunkhdr *)sctp_m_getptr(m, offset,
	    sizeof(struct sctp_chunkhdr), chunk_buf);
	while (ch != NULL) {
		ch_len = ntohs(ch->chunk_length);
		if ((ch_len < sizeof(*ch)) || (ch_len > length)) {
			/* break to abort land */
			break;
		}
		switch (ch->chunk_type) {
		case SCTP_PACKET_DROPPED:
			/* we don't respond to pkt-dropped */
			return;
		case SCTP_ABORT_ASSOCIATION:
			/* we don't respond with an ABORT to an ABORT */
			return;
		case SCTP_SHUTDOWN_COMPLETE:
			/*
			 * we ignore it since we are not waiting for it
			 * and peer is gone
			 */
			return;
		case SCTP_SHUTDOWN_ACK:
			sctp_send_shutdown_complete2(ep, to, sctphdr->v_tag);
			return;
		default:
			break;
		}
		length -= SCTP_SIZE32(ch_len);
		if (length < sizeof(*ch)) {
			/* no more data left in the mbuf chain */
			break;
		}
		offset += SCTP_SIZE32(ch_len);
		ch = (struct sctp_chunkhdr *)sctp_m_getptr(m, offset,
		    sizeof(struct sctp_chunkhdr), chunk_buf);
	}
	if (to->sa_family == AF_INET) {
		sctp_send_abort(m, iph, sctphdr, iphlen, 0, operr);
	} else {
		sctp6_send_abort(m, ip6h, sctphdr, iphlen, 0, operr);
	}
}


int
sctp_is_there_an_abort_here(struct mbuf *m, int off, int *vtagfill)
{
	/*
	 * check the inbound datagram to make sure there is not an abort
	 * inside it, if there is return 1, else return 0.
	 */
	struct sctp_chunkhdr desc;
	int at, x;

	at = off + sizeof(struct sctphdr);
	while ((at+sizeof(struct sctp_chunkhdr)) <= m->m_pkthdr.len) {
		m_copydata(m, at, sizeof(struct sctp_chunkhdr),
		    (caddr_t)&desc);
		x = desc.chunk_length;
		NTOHS(x);
		/* Is it to small? */
		if (x < sizeof(struct sctp_chunkhdr)) {
			/* packet is probably corrupt */
			break;
		}

		/* is it to large? */
		if ((x + at) > m->m_pkthdr.len) {
			/* packet is probably corrupt */
			break;
		}
		/* we seem to be ok, is it an abort? */
		if (desc.chunk_type == SCTP_ABORT_ASSOCIATION) {
			/* yep, tell them */
			return (1);
		}
		if (desc.chunk_type == SCTP_INITIATION) {
			/* need to update the Vtag */
			if ((at+sizeof(struct sctp_init_chunk)) >=
			    m->m_pkthdr.len) {
				/* there is a INIT here */
				struct sctp_init_chunk ic;
				m_copydata(m, at,
				    sizeof(struct sctp_init_chunk),
				    (caddr_t)&ic);
				/* Update the vtag */
				*vtagfill = ntohl(ic.init.initiate_tag);
			}
		}
		/* Nope, move to the next chunk */
		at += x;
	}
	return (0);
}


/*
 * currently (2/02), ifa_addr embeds scope_id's and don't
 * have sin6_scope_id set (i.e. it's 0)
 * so, create this function to compare link local scopes
 */
uint32_t
sctp_is_same_scope(struct sockaddr_in6 *addr1, struct sockaddr_in6 *addr2)
{
	struct sockaddr_in6 a, b;
	/* save copies */
	a = *addr1;
	b = *addr2;

	if (a.sin6_scope_id == 0)
		if (in6_recoverscope(&a, &a.sin6_addr, NULL)) {
			/* can't get scope, so can't match */
			return (0);
		}
	if (b.sin6_scope_id == 0)
		if (in6_recoverscope(&b, &b.sin6_addr, NULL)) {
			/* can't get scope, so can't match */
			return (0);
		}
	if (a.sin6_scope_id != b.sin6_scope_id)
		return (0);

	return (1);
}

/*
 * returns a sockaddr_in6 with embedded scope recovered and removed
 */
struct sockaddr_in6 *
sctp_recover_scope(struct sockaddr_in6 *addr, struct sockaddr_in6 *store)
{
	/* check and strip embedded scope junk */
	if (addr->sin6_family == AF_INET6) {
		if (IN6_IS_SCOPE_LINKLOCAL(&addr->sin6_addr)) {
			if (addr->sin6_scope_id == 0) {
				*store = *addr;
				if (!in6_recoverscope(store, &store->sin6_addr,
				    NULL)) {
					/* use the recovered scope */
					addr = store;
				}
				/* else, return the original "to" addr */
			}
		}
	}
	return (addr);
}

/*
 * are the two addresses the same?  currently a "scopeless" check
 * returns: 1 if same, 0 if not
 */
int
sctp_cmpaddr(struct sockaddr *sa1, struct sockaddr *sa2) {
	/* must be valid */
	if ((sa1 == NULL) || (sa2 == NULL))
		return (0);

	/* must be the same family */
	if (sa1->sa_family != sa2->sa_family)
		return (0);

	if (sa1->sa_family == AF_INET6) {
		/* IPv6 addresses */
		struct sockaddr_in6 *sin6_1, *sin6_2;

		sin6_1 = (struct sockaddr_in6 *)sa1;
		sin6_2 = (struct sockaddr_in6 *)sa2;
		return (SCTP6_ARE_ADDR_EQUAL(&sin6_1->sin6_addr,
		    &sin6_2->sin6_addr));
	} else if (sa1->sa_family == AF_INET) {
		/* IPv4 addresses */
		struct sockaddr_in *sin_1, *sin_2;

		sin_1 = (struct sockaddr_in *)sa1;
		sin_2 = (struct sockaddr_in *)sa2;
		return (sin_1->sin_addr.s_addr == sin_2->sin_addr.s_addr);
	} else {
		/* we don't do these... */
		return (0);
	}
}


/*
 * ntop() routines
 */
#define SPRINTF(x)	((size_t)sprintf x)
#define NS_INT16SZ    2       /* #/bytes of data in a u_int16_t */
#define NS_IN6ADDRSZ  16      /* IPv6 T_AAAA */

const char *
sctp_ntop4(const u_char *src, char *dst, size_t size) {
	char tmp[sizeof("255.255.255.255")];

	if (SPRINTF((tmp, "%u.%u.%u.%u", src[0], src[1], src[2], src[3])) >
	    size) {
		return (NULL);
	}
	strcpy(dst, tmp);
	return (dst);
}

const char *
sctp_ntop6(const u_char *src, char *dst, size_t size) {
	/*
	 * Note that int32_t and int16_t need only be "at least" large enough
	 * to contain a value of the specified size.  On some systems, like
	 * Crays, there is no such thing as an integer variable with 16 bits.
	 * Keep this in mind if you think this function should have been coded
	 * to use pointer overlays.  All the world's not a VAX.
	 */
	char tmp[sizeof "ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255"], *tp;
	struct { int base, len; } best, cur;
	u_int words[NS_IN6ADDRSZ / NS_INT16SZ];
	int i;

	/*
	 * Preprocess:
	 *      Copy the input (bytewise) array into a wordwise array.
	 *      Find the longest run of 0x00's in src[] for :: shorthanding.
	 */
	memset(words, '\0', sizeof words);
	for (i = 0; i < NS_IN6ADDRSZ; i++)
		words[i / 2] |= (src[i] << ((1 - (i % 2)) << 3));
	best.base = -1;
	cur.base = -1;
	for (i = 0; i < (NS_IN6ADDRSZ / NS_INT16SZ); i++) {
		if (words[i] == 0) {
			if (cur.base == -1)
				cur.base = i, cur.len = 1;
			else
				cur.len++;
		} else {
			if (cur.base != -1) {
				if (best.base == -1 || cur.len > best.len)
					best = cur;
				cur.base = -1;
			}
		}
	}
	if (cur.base != -1) {
		if (best.base == -1 || cur.len > best.len)
			best = cur;
	}
	if (best.base != -1 && best.len < 2)
		best.base = -1;

	/*
	 * Format the result.
	 */
	tp = tmp;
	for (i = 0; i < (NS_IN6ADDRSZ / NS_INT16SZ); i++) {
		/* Are we inside the best run of 0x00's? */
		if (best.base != -1 && i >= best.base &&
		    i < (best.base + best.len)) {
			if (i == best.base)
				*tp++ = ':';
			continue;
		}
		/* Are we following an initial run of 0x00s or any real hex? */
		if (i != 0)
			*tp++ = ':';
		/* Is this address an encapsulated IPv4? */
		if (i == 6 && best.base == 0 &&
		    (best.len == 6 || (best.len == 5 && words[5] == 0xffff))) {
			if (!sctp_ntop4(src+12, tp, sizeof tmp - (tp - tmp)))
				return (NULL);
			tp += strlen(tp);
			break;
		}
		tp += SPRINTF((tp, "%x", words[i]));
	}
	/* Was it a trailing run of 0x00's? */
	if (best.base != -1 && (best.base + best.len) ==
	    (NS_IN6ADDRSZ / NS_INT16SZ))
		*tp++ = ':';
	*tp++ = '\0';

	/*
	 * Check for overflow, copy, and we're done.
	 */
	if ((size_t)(tp - tmp) > size) {
		return (NULL);
	}
	strcpy(dst, tmp);
	return (dst);
}

void
sctp_print_address(struct sockaddr *sa)
{
	char buf[128];

	if (sa->sa_family == AF_INET6) {
		struct sockaddr_in6 *sin6;
		sin6 = (struct sockaddr_in6 *)sa;
		sctp_ntop6((char *)&sin6->sin6_addr, buf, sizeof(buf));
		printf("IPv6 address: %s:%d scope:%u\n", buf,
		       ntohs(sin6->sin6_port),
		       sin6->sin6_scope_id);
	} else if (sa->sa_family == AF_INET) {
		struct sockaddr_in *sin;
		sin = (struct sockaddr_in *)sa;
		sctp_ntop4((char *)&sin->sin_addr, buf, sizeof(buf));
		printf("IPv4 address: %s:%d\n", buf,
		       ntohs(sin->sin_port));
	} else {
		printf("?\n");
	}
}

int
sbappendaddr_nocheck(sb, asa, m0, control, tag)
	struct sockbuf *sb;
	struct sockaddr *asa;
	struct mbuf *m0, *control;
	u_int32_t tag;
{
#ifdef __NetBSD__
	struct mbuf *m, *n;
	int space = asa->sa_len;

	if (m0 && (m0->m_flags & M_PKTHDR) == 0)
		panic("sbappendaddr_nocheck");
	if (m0)
		space += m0->m_pkthdr.len;

	m0->m_pkthdr.csum_data = tag;

	for (n = control; n; n = n->m_next) {
		space += n->m_len;
		if (n->m_next == 0)	/* keep pointer to last control buf */
			break;
	}
	MGET(m, M_DONTWAIT, MT_SONAME);
	if (m == 0)
		return (0);
	if (asa->sa_len > MLEN) {
		MEXTMALLOC(m, asa->sa_len, M_NOWAIT);
		if ((m->m_flags & M_EXT) == 0) {
			m_free(m);
			return (0);
		}
	}
	m->m_len = asa->sa_len;
	memcpy(mtod(m, caddr_t), (caddr_t)asa, asa->sa_len);
	if (n)
		n->m_next = m0;		/* concatenate data to control */
	else
		control = m0;
	m->m_next = control;
	for (n = m; n; n = n->m_next)
		sballoc(sb, n);
	if ((n = sb->sb_mb) != NULL) {
		while (n->m_nextpkt)
			n = n->m_nextpkt;
		n->m_nextpkt = m;
	} else
		sb->sb_mb = m;
	return (1);
#endif
#ifdef __FreeBSD__
	register struct mbuf *m, *n;
	int space = asa->sa_len;
	int cnt=0;

	if (m0 && (m0->m_flags & M_PKTHDR) == 0)
		panic("sbappendaddr_nocheck");
	if (m0)
		space += m0->m_pkthdr.len;
	m0->m_pkthdr.csum_data = (int)tag;
	for (n = control; n; n = n->m_next) {
		space += n->m_len;
		if (n->m_next == 0)	/* keep pointer to last control buf */
			break;
	}
	if (asa->sa_len > MLEN)
		return (0);
 try_again:
	MGET(m, M_DONTWAIT, MT_SONAME);
	if (m == 0)
		return (0);
        /* safety */
	if (m == m0) {
		printf("Duplicate mbuf allocated %x in and mget returned %x?\n",
		       (u_int)m0,
		       (u_int)m);
		if (cnt) {
		        panic("more than once");
		}
		cnt++;
		goto try_again;
	}
	m->m_len = asa->sa_len;
	bcopy((caddr_t)asa, mtod(m, caddr_t), asa->sa_len);
	if (n)
		n->m_next = m0;		/* concatenate data to control */
	else
		control = m0;
	m->m_next = control;
	for (n = m; n; n = n->m_next)
		sballoc(sb, n);
	n = sb->sb_mb;
	if (n) {
		while (n->m_nextpkt)
			n = n->m_nextpkt;
		n->m_nextpkt = m;
	} else
		sb->sb_mb = m;
	return (1);
#endif
#ifdef __OpenBSD__
	register struct mbuf *m, *n;
	int space = asa->sa_len;

	if (m0 && (m0->m_flags & M_PKTHDR) == 0)
		panic("sbappendaddr_nocheck");
	if (m0)
		space += m0->m_pkthdr.len;
	m0->m_pkthdr.csum = (int)tag;
	for (n = control; n; n = n->m_next) {
		space += n->m_len;
		if (n->m_next == 0)	/* keep pointer to last control buf */
			break;
	}
	if (asa->sa_len > MLEN)
		return (0);
	MGET(m, M_DONTWAIT, MT_SONAME);
	if (m == 0)
		return (0);
	m->m_len = asa->sa_len;
	bcopy((caddr_t)asa, mtod(m, caddr_t), asa->sa_len);
	if (n)
		n->m_next = m0;		/* concatenate data to control */
	else
		control = m0;
	m->m_next = control;
	for (n = m; n; n = n->m_next)
		sballoc(sb, n);
	if ((n = sb->sb_mb) != NULL) {
		while (n->m_nextpkt)
			n = n->m_nextpkt;
		n->m_nextpkt = m;
	} else
		sb->sb_mb = m;
	return (1);
#endif
}

#ifdef SCTP_ALTERNATE_ROUTE

#if defined(__NetBSD__) || defined(__OpenBSD__)
#define rn_offset rn_u.rn_node.rn_Off
#define rn_bit rn_b
#define rn_parent rn_p
#define rn_left rn_u.rn_node.rn_L
#endif

static int
sctp_is_interior_node(struct radix_node *node)
{
	if (node->rn_bit < 0) {
		/*
		 * Negative numbers are exterior/leaf nodes
		 * i.e. the actual pointer to the info
		 */
		return (0);
	} else {
		/*
		 * postive numbers are interior (tree structure) 
		 * nodes.
		 */
	return (1);
	}
}



static struct rtentry *
rtfinalize_route(struct sockaddr *dst, struct rtentry *rt)
{
	/*
	 * We handle cloning in this module (if needed). 
	 * This could probably be put inline but I don't want
	 * to clone it multiple times :>
	 */
	struct rt_addrinfo info;
	struct rtentry *newrt;
	int err;
	if (rt->rt_flags & (RTF_CLONING
#ifdef __FreeBSD__
			    | RTF_PRCLONING
#endif
		)) {
		newrt = rt;
		bzero((caddr_t)&info, sizeof(info));
		err = rtrequest(RTM_RESOLVE, dst, (struct sockaddr *)0,
				(struct sockaddr *)0, 0, &newrt);
		if (err) {
			info.rti_info[RTAX_DST] = dst;
			rt_missmsg(RTM_MISS, &info, 0, err);
			rt->rt_refcnt++;
			return (rt);
		} else {
			rt = newrt;
			if (rt->rt_flags & RTF_XRESOLVE) {
				info.rti_info[RTAX_DST] = dst;
				rt_missmsg(RTM_RESOLVE, &info, 0, err);
				return (rt);
			}
			/* Inform listeners of the new route */
			info.rti_info[RTAX_DST] = rt_key(rt);
			info.rti_info[RTAX_NETMASK] = rt_mask(rt);
			info.rti_info[RTAX_GATEWAY] = rt->rt_gateway;
		}
	} else {
		/* No cloning needed */
		rt->rt_refcnt++;
	}
	return (rt);
}

static int
sctp_rn_are_keys_same(struct radix_node *exist,
		      struct radix_node *cmp)
{
	caddr_t e, c, cplim;
	int len;
	if (sctp_is_interior_node(exist) || sctp_is_interior_node(cmp)) {
		/* can't compare interior nodes */
		return (0);
	}

	if (exist->rn_key == cmp->rn_key) {
		/* Mask holds same pointer. Must be same */
		return (1);
	}
	if ((exist->rn_key == NULL) || (cmp->rn_key == NULL)) {
		/*
		 * One is null (host route) the other is not. Can't be same.
		 */
		return (0);
	}
	e = exist->rn_key;
	c = cmp->rn_key;
	len = (int)*((u_char *)e);
	cplim = e + len;
	while (e < cplim) {
		if (*e != *c) {
			return (0);
		}
		e++;
		c++;
 	}
	/* so far the keys are the same */
	if (exist->rn_mask != cmp->rn_mask) {
		/* different masks */
		return (0);
	}
	/* They are the same :-) */
	return (1);
}


static int
sctp_are_gw_the_same(struct rtentry *rt1, struct rtentry *rt2)
{
	if (!rt1 || !rt2)
		return (0);

	if (rt1 == rt2)
		return (1);

	if (rt1->rt_gateway == rt2->rt_gateway)
		return (1);
	if (rt1->rt_gateway->sa_len == rt2->rt_gateway->sa_len) {
		/*
		 * I think we don't actually need to do this
		 * since I believe pointers would match first, but
		 * to be cautious I will put this in for
		 * now and we can rip it out later. This would
		 * optimize the search if we remove the memcmp.
		 */
		int ret;
		ret = memcmp(rt1->rt_gateway, rt2->rt_gateway,
		    rt1->rt_gateway->sa_len);
		if (ret == 0) {
			return (1);
		}
	}
	return (0);
}

static struct rtentry *
sctp_rt_scan_dups(struct sockaddr *dst, struct rtentry *existing, int finalize)
{
	struct radix_node *exist, *cmp, *prev;
	struct rtentry *dupped;
	exist = (struct radix_node *)existing;

	if (sctp_is_interior_node(exist)) {
		/* Can't look at a interior node */
		return (NULL);
	}
	/* get the dupedkey from this one */
	cmp = exist->rn_dupedkey;
	while (cmp != NULL) {
		if (sctp_is_interior_node(cmp)) {
			return (NULL);
		}
		dupped = (struct rtentry *)cmp;
		if (dupped->rt_flags &  
#ifdef __FreeBSD__
		   RTF_WASCLONED
#else
		   RTF_CLONED
#endif
			) {
			/* We don't consider a cloned route
			 * as via-able. We use the base route
			 * only and then finalize from that.
			 */
 			return (NULL);
		}
		if ((!sctp_are_gw_the_same(dupped, existing)) &&
		    sctp_rn_are_keys_same(exist, cmp)) {
			/* Keys are no longer the same */
			if (finalize) {
				dupped = rtfinalize_route(dst,(struct rtentry *)cmp);
				return (dupped);
			} else {
				return ((struct rtentry *)cmp);
			}
		} else {
			cmp = cmp->rn_dupedkey;
		}
	}
	/* If we ran down to the end of the chain then
	 * lets go from the beginning of the chain 
	 * and run forward through the list.
	 */
	cmp = exist->rn_parent;
	prev = exist;
	while (cmp) {
		/* We move back until we hit a interior node
		 * or the RNF_ROOT flag (if we are on the default
		 * chain). This gets us to the point where
		 * prev points to the top of the chain.
		 */
		if (sctp_is_interior_node(cmp) ||
		   (cmp->rn_flags & RNF_ROOT)) {
			/* we expect this to break at the 
			 * interior parent or in the case of
			 * the default we will find a exterior
			 * node marked as ROOT.
			 */
			break;
		}
		prev = cmp;
		cmp = cmp->rn_parent;
	}
	if (cmp == NULL) {
		return (NULL);
	}
	/* now set to prev one */
	cmp = prev;
	/* now move forward until we see exist */
	while (cmp != exist) {
		dupped = (struct rtentry *)cmp;
		if (dupped->rt_flags &  
#ifdef __FreeBSD__
		   RTF_WASCLONED
#else
		   RTF_CLONED
#endif
			) {
			/* We don't consider a cloned route
			 * as via-able. We use the base route
			 * only and then finalize from that.
			 */
 			return (NULL);
		}
		if ((!sctp_are_gw_the_same(dupped, existing)) &&
		    sctp_rn_are_keys_same(exist, cmp)) {
			/* Keys are no longer the same */
			if (finalize) {
				dupped = rtfinalize_route(dst,(struct rtentry *)cmp);
				return (dupped);
			} else {
				return ((struct rtentry *)cmp);
			}
		} else {
			cmp = cmp->rn_dupedkey;
		}
	}
	return (NULL);
}


static struct rtentry *
sctp_find_cloned_in(struct radix_node *entry, struct rtentry *rt)
{

	if (entry->rn_flags & RNF_ROOT) {
		/* Huh, can't search root */
		return (NULL);
	}
	if (sctp_is_interior_node(entry)) {
		/* Here we hunt the left side only. Left is
		 * 0 bit match and we need to get less specific
		 * i.e. when at 10.1.1.1 we look for 10.1.1.0,
		 * 10.1.0.0 and 10.0.0.0... we don't want to
		 * find 10.1.3.0, which we would if we looked
		 * right.
		 */
		if (entry->rn_left) {
			struct rtentry *res;
			res = sctp_find_cloned_in(entry->rn_left, rt);
			if (res) {
				return (res);
			}
		}
	} else {
		/* Exterior node, compare the gateways */
		while (entry) {
			if (((struct rtentry *)(entry))->rt_flags & 
			   (RTF_CLONING
#ifdef __FreeBSD__
|RTF_PRCLONING
#endif
)
				) {
				/* only with these flags can it be a candidate */
				if (sctp_are_gw_the_same(rt,((struct rtentry *)(entry)))) {
					return (((struct rtentry *)(entry)));
				}
			}
			entry = entry->rn_dupedkey;
		}
	}
	return (NULL);
}


static struct rtentry *
sctp_find_cloned_from(struct sockaddr *dst,
		      struct rtentry *rt)
{
	/* This a very very tricky. We must hunt
	 * the radix_node tree looking for where
	 * we were cloned from. This must be a
	 * node that has the RTF_CLONING or
	 * RTF_PRCLONING flags set on it and
	 * the corresponding gateway matches that
	 * of rt.
	 */
	struct radix_node_head *rnh;
	struct radix_node *base, *cmp, *prev, *top,*topleft,*def;
	struct rtentry *result;

	base = (struct radix_node *)rt;
	if (sctp_is_interior_node(base)) {
		return (NULL);
	}
	if ((base->rn_flags & RNF_ACTIVE) == 0) {
		/* Our route is bad */
		return (NULL);
	}
	/* First we must look from the table up. 
	 * Most likely a clone of the default but
	 * we don't optimize for that otherwise we
	 * reverse the order of the tree :<
	 */

	prev = base;
	cmp = base->rn_parent;
	while (cmp &&
	      (!sctp_is_interior_node(cmp))) {
		/* find the first interior node up from my base */
		prev = cmp;
		cmp = cmp->rn_parent;
	}
	/* Now traverse the tree */
	while (cmp && ((cmp->rn_flags & RNF_ROOT) == 0)) {
		/* Here we hunt the left side only. Left is
		 * 0 bit match and we need to get less specific
		 * i.e. when at 10.1.1.1 we look for 10.1.1.0,
		 * 10.1.0.0 and 10.0.0.0... we don't want to
		 * find 10.1.3.0, which we would if we looked
		 * right. Note that we optimize by verifying that
		 * we are not looking at ourselve... i.e. if we
		 * were cloned from the default, we may find that
		 * there is nothing to the left but our node.
		 */
		if (prev != cmp->rn_left) {
			result = sctp_find_cloned_in(cmp->rn_left, rt);
			if (result) {
				/* found him */
				return (result);
			}
		}
		/* We have now checked rt's parents left side, 
		 * now we must go up looking for our cloning source
		 * for rt.
		 */

		/* save were we looked in prev, so we don't
		 * check where we already looked.
		 */
		prev = cmp;
		/* move up to parent */
		cmp = cmp->rn_parent;
	}
	/* 
	 * Now lets go look at the default.
	 */
	def = NULL;
	/* get the top of the tree */
	rnh = rt_tables[dst->sa_family];
	if (rnh) {
		top = rnh->rnh_treetop;
		/* valdiate that the top is an interior node */
		if (sctp_is_interior_node(top)) {
			/* move left as far as possible to
			 * get to the ROOT node that is 
			 * the default edge.
			 */
			topleft = top->rn_left;
			while (topleft && sctp_is_interior_node(topleft)) {
				topleft = topleft->rn_left;
			}
			if (topleft &&
			   (topleft->rn_flags & RNF_ROOT) &&
			   (sctp_is_interior_node(topleft) == 0)
				) {
				/* copy the dupedky .. it may be NULL. */
				def = topleft->rn_dupedkey;
			}
		} else {
			return (NULL);
		}
	}
	if (def) {
		/* we have a default, was it cloned from that? */
		result = sctp_find_cloned_in(def, rt);
		if (result) {
			return (result);
		}
	}
	return (NULL);
}

static struct rtentry *
sctp_check_cloned_route(struct sockaddr *dst,
			struct rtentry *rt)

{
    /* Given a cloned route, find the route that
     * it was cloned from in the routing table and
     * then use the base to scan for valid alternates.
     */
    struct rtentry *cloned_from,*ent,*sameone;
    struct radix_node *base, *prev;	

    /* Note: that cloned_from and rt SHOULD have
     * the same gateway as the existing route
     * or else we would have used an alternate
     * and not gotten into checking the cloned
     * routes.
     */
    cloned_from = sctp_find_cloned_from(dst, rt);
    if (cloned_from) {
	ent = sctp_rt_scan_dups(dst, cloned_from,0);
	if (ent == NULL) {
		return (ent);
	}
	/* if it does not require cloning just finalize */
	if ((ent->rt_flags & (RTF_CLONING
#ifdef __FreeBSD__
			    | RTF_PRCLONING
#endif
		    )) == 0) {
		ent->rt_refcnt++;
		return (ent);
	}
	/* Ok if the route requires cloning we must 
	 * validate that we have not already cloned it
	 * to the original list in rt.. if so we return
	 * that otherwise we proceed with cloning by calling
	 * rtfinalize.
	 */
	base = ((struct radix_node *)rt)->rn_dupedkey;
	while (base != NULL) {
		if (sctp_is_interior_node(base)) {
			/* Gak, a interior node? */
			return (NULL);
		}
		sameone = (struct rtentry *)base;
		if ((sctp_are_gw_the_same(sameone, ent)) &&
		    sctp_rn_are_keys_same(((struct radix_node *)rt), base)) {
			/* found it */
			sameone->rt_refcnt++;
			return (sameone);
		} else {
			base = base->rn_dupedkey;
		}
	}
	/* Went to end now must go back to beginning */
	base = ((struct radix_node *)rt)->rn_parent;
	prev = (struct radix_node *)rt;
	while (base) {
		/* We move back until we hit a interior node
		 * or the RNF_ROOT flag (if we are on the default
		 * chain). This gets us to the point where
		 * prev points to the top of the chain.
		 */
		if (sctp_is_interior_node(base) ||
		   (base->rn_flags & RNF_ROOT)) {
			/* we expect this to break at the 
			 * interior parent or in the case of
			 * the default we will find a exterior
			 * node marked as ROOT.
			 */
			break;
		}
		prev = base;
		base = base->rn_parent;
	}
	if (base == NULL) {
		/* punt */
		return (rtfinalize_route(dst, ent));
	}

	/* now set to prev one */
	base = prev;
	/* now move forward until we see exist */
	while (base != (struct radix_node *)rt) {
		if (sctp_is_interior_node(base)) {
			/* Gak, a interior node? */
			return (NULL);
		}
		sameone = (struct rtentry *)base;
		if ((sctp_are_gw_the_same(sameone, ent)) &&
		    sctp_rn_are_keys_same((struct radix_node *)rt, base)) {
			/* found it */
			sameone->rt_refcnt++;
			return (sameone);
		} else {
			base = base->rn_dupedkey;
		}
	}
	/* finalize what came back */
	return (rtfinalize_route(dst, ent));
    } else
	return (NULL);
}

/*
 * Look up the route that matches the address given
 * Or, at least try.. Create a cloned route i, *tmp2;
 */
static struct rtentry *
sctp_rtalloc_alternate(struct sockaddr *dst,
		       struct rtentry *existing,
		       int peer_dest_route)
{
	int cursalen, s;
	struct rtentry *base_rt,*tmp2;
	struct radix_node *base, *base_nxt;
	struct sockaddr_storage s_store;
	struct sockaddr *sa;
	static void *my_xx1,*my_xx2;
#if defined(__NetBSD__) || defined(__OpenBSD__)
	s = splsoftnet();
#else
	s = splnet();
#endif
	/* setup fence and boundary stuff */
	memset(&s_store, 0, sizeof(s_store));
	my_xx1 = &cursalen;
	my_xx2 = &sa;

	/* now on with the show */
	if (existing == NULL) {
		/* No existing route, we just to rtalloc1() */
		goto noexisting;
	}
	if ((existing->rt_nodes[0].rn_flags & RNF_ACTIVE) == 0) {
		goto noexisting;
	}


	/* We must do the following:
	 *  - Using the existing route look at each duplicate that
	 *    may be found on the duped list. If we find a different
	 *    gateway, we are done.
	 *  - For each cloned route that is in my current dupedkey
	 *    chain (including existing) I must search back and
	 *    find the route that was cloned, then look at its
	 *    dupped chain. 
	 */

	/* 1: first look up and down the chain for alternates at our level
	 *    note, that this will NOT find cloned routes, but will allow
	 *    us to see added host routes at our level in the tree.
	 */
	tmp2 = sctp_rt_scan_dups(dst, existing, 1);
	if (tmp2) {
		 splx(s);
		 return (tmp2);
	 }
	/* First step un-successful, so now lets re-examine each
	 * route that scan_dups looked at and see if the WAS_CLONED
	 * flag was set on the route. If so we must find the base
	 * route and do a scan_dups on that as well.
	 */
	base_rt = existing;
	while (base_rt) {
	    if (base_rt->rt_flags &
#ifdef __FreeBSD__
		   RTF_WASCLONED
#else
		   RTF_CLONED
#endif

		) {
		/* yep this one was cloned 
		 * check it.
		 */
		tmp2 = sctp_check_cloned_route(dst, base_rt);
		if (tmp2) {
		    splx(s);
		    return (tmp2);
		}
	    }
	    /* now advance to next node */
	    base = (struct radix_node *)base_rt;
	    if (base->rn_dupedkey) {
		/* check to make sure it is an exterior node */
		if (sctp_is_interior_node(base->rn_dupedkey)) {
		    break;
		}
	    }
	    base_rt = (struct rtentry *)base->rn_dupedkey;
	}
	/* Now we must back UP from existing */
	base = (struct radix_node *)existing;
	/* get previous */
	base_nxt = base->rn_parent;
	while ((base_nxt != NULL) &&
	      (sctp_is_interior_node(base_nxt) == 0) &&
	      ((base_nxt->rn_flags & RNF_ROOT) == 0)) {
	    base_rt = (struct rtentry *)base_nxt;
	    if (base_rt->rt_flags &
#ifdef __FreeBSD__
		   RTF_WASCLONED
#else
		   RTF_CLONED
#endif
		) {
		/* yep this one was cloned 
		 * check it.
		 */
		tmp2 = sctp_check_cloned_route(dst, base_rt);
		if (tmp2) {
		    splx(s);
		    return (tmp2);
		}
	    }
	    /* back up again */
	    base_nxt = base_nxt->rn_parent;
	}
	/* If we fall out here, we have checkd all and
	 * failed, give them back a normal rtalloc1() which
	 * will yeild the same result.
	 */

 noexisting:
#ifdef __FreeBSD__
	tmp2 = rtalloc1(dst, 1, 0);
#else
	tmp2 = rtalloc1(dst, 1);
#endif
	splx(s);
	return (tmp2);
}

#endif

struct rtentry *
rtalloc_alternate (struct sockaddr *dst, struct rtentry *old,
		   int peer_dest_route)
{
#if defined(SCTP_ALTERNATE_ROUTE) && defined(RADIX_MPATH)
	/* In order for this routine to work the KAME RADIX_MPATH option in
	 * order for this to work. Right now this is only supported under
	 * netbsd.
	 */
	return (sctp_rtalloc_alternate(dst, old, peer_dest_route));
#else
#ifdef __FreeBSD__
	return (rtalloc1(dst, 1, 0UL));
#else
	return (rtalloc1(dst, 1));
#endif
#endif
}

struct mbuf *
sctp_generate_invmanparam(int err)
{
	/* Return a MBUF with a invalid mandatory parameter */
	struct mbuf *m;
	MGET(m, M_DONTWAIT, MT_DATA);
	if (m) {
		struct sctp_paramhdr *ph;
		m->m_len = sizeof(struct sctp_paramhdr);
		ph = mtod(m, struct sctp_paramhdr *);
		ph->param_length = htons(sizeof(struct sctp_paramhdr));
		ph->param_type = htons(err);
	}
	return (m);
}

static int
sctp_should_be_moved(struct mbuf *this, struct sctp_association *asoc)
{
	struct mbuf *m;
	/* given a mbuf chain, look through it finding
	 * the M_PKTHDR and return 1 if it belongs to
	 * the association given. We tell this by
	 * a kludge where we stuff the my_vtag of the assoc
	 * into the m->m_pkthdr.csum_data/csum field.
	 */
	m = this;
	while (m) {
		if (m->m_flags & M_PKTHDR) {
			/* check it */
			if (
#if defined(__FreeBSD__) || defined(__NetBSD__)
				(u_int32_t)m->m_pkthdr.csum_data
#else
/* OpenBSD */
				(u_int32_t)m->m_pkthdr.csum
#endif
				== asoc->my_vtag) {
				/* Yep */
				return (1);
			}
		}
		m = m->m_next;
	}
	return (0);
}

void
sctp_grub_through_socket_buffer(struct sctp_inpcb *inp,
				struct socket *old,
				struct socket *new,
				struct sctp_tcb *tcb)
{
	struct mbuf **put,**take,*next,*this;
	struct sockbuf *old_sb,*new_sb;	
	struct sctp_association *asoc;
	int moved_top = 0;

	asoc = &tcb->asoc;
	old_sb = &old->so_rcv;
	new_sb = &new->so_rcv;
	if (old_sb->sb_mb == NULL) {
		/* Nothing to move */
		return;
	}
	if (inp->sctp_vtag_last == asoc->my_vtag) {
		/* First one must be moved */
		struct mbuf *mm;
		for (mm = old_sb->sb_mb; mm; mm = mm->m_next) {
			/* Go down the chain and fix
			 * the space allocation of the
			 * two sockets.
			 */
			sbfree(old_sb, mm);
			sballoc(new_sb, mm);
		}
		new_sb->sb_mb = old_sb->sb_mb;
		old_sb->sb_mb = new_sb->sb_mb->m_nextpkt;
		new_sb->sb_mb->m_nextpkt = NULL;
		put = &new_sb->sb_mb->m_nextpkt;
		moved_top = 1;
	} else {
		put = &new_sb->sb_mb;
	}

	take = &old_sb->sb_mb;
	next = old_sb->sb_mb;
	while (next) {
		this = next;
		/* postion for next one */
		next = this->m_nextpkt;
		/* check the tag of this packet */
		if (sctp_should_be_moved(this, asoc)) {
			/* yes this needs to be moved */
			struct mbuf *mm;
			*take = this->m_nextpkt;
			this->m_nextpkt = NULL;
			*put = this;
			for (mm = this; mm; mm = mm->m_next) {
				/* Go down the chain and fix
				 * the space allocation of the
				 * two sockets.
				 */
				sbfree(old_sb, mm);
				sballoc(new_sb, mm);
			}
			put = &this->m_nextpkt;
			
		} else {
			/* no advance our take point. */
			take = &this->m_nextpkt;
		}
	} 
	if (moved_top) {
		/* Ok so now we must re-postion vtag_last to
		 * match the new first one since we moved the
		 * mbuf at the top.
		 */
		inp->sctp_vtag_last = 0;
		this = old_sb->sb_mb;
		while (this) {
			if (this->m_flags & M_PKTHDR) {
				/* check it */
				if (
#if defined(__FreeBSD__) || defined(__NetBSD__)
					(u_int32_t)this->m_pkthdr.csum_data
#else
/* OpenBSD */
					(u_int32_t)this->m_pkthdr.csum
#endif
					!= 0) {
					/* its the one */
					inp->sctp_vtag_last =
#if defined(__FreeBSD__) || defined(__NetBSD__)
					(u_int32_t)this->m_pkthdr.csum_data
#else
/* OpenBSD */
					(u_int32_t)this->m_pkthdr.csum
#endif
						;
					break;
				}

			}
			this = this->m_nextpkt;
		}

	}
}

void
sctp_free_bufspace(struct sctp_tcb *stcb,
		   struct sctp_association *asoc,
		   struct sctp_tmit_chunk *tp1)
{
	struct mbuf *mm;
	int mbcnt=0;
	int num_mb=0;
	int num_mbext=0;

	if (tp1->data == NULL) {
		return;
	}
	/* The book_size accounts for all 
	 * of the actual data size, so instead here
	 * we need to go through and sum up
	 * the MBUF/M_EXT useage for subtraction.
	 */
	for (mm = tp1->data; mm; mm = mm->m_next) {
		num_mb++;
		mbcnt += MSIZE;
		if (mm->m_flags & M_EXT) {
			num_mbext++;
			mbcnt += mm->m_ext.ext_size;
		}
	}
	/* We release the book_size and mbcnt */
	if (asoc->total_output_queue_size >= tp1->book_size) {
		asoc->total_output_queue_size -= tp1->book_size;
	} else {
		asoc->total_output_queue_size = 0;
	}

	/* Now free the mbuf */
	if (asoc->total_output_mbuf_queue_size >= mbcnt) {
		asoc->total_output_mbuf_queue_size -= mbcnt;
	} else {
		asoc->total_output_mbuf_queue_size = 0;
	}
#ifdef  SCTP_TCP_MODEL_SUPPORT
	if ((stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_TCPTYPE) ||
	   (stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_IN_TCPPOOL)) {
		if (stcb->sctp_socket->so_snd.sb_cc >= tp1->book_size) {
			stcb->sctp_socket->so_snd.sb_cc -= tp1->book_size;
		} else {
			stcb->sctp_socket->so_snd.sb_cc = 0;

		}
		if (stcb->sctp_socket->so_snd.sb_mbcnt >= mbcnt) {
			stcb->sctp_socket->so_snd.sb_mbcnt -= mbcnt;
		} else {
			stcb->sctp_socket->so_snd.sb_mbcnt = 0;
		}
	}
#endif

}

int
sctp_release_pr_sctp_chunk(struct sctp_tcb *tcb, struct sctp_tmit_chunk *tp1, 
    int reason, struct sctpchunk_listhead *queue)	
{
	int ret_sz = 0;
	int notdone;
	uint8_t foundeom = 0;

	do {
		printf("We are dropping TSN:%x frag state:%x\n",
		    (u_int)tp1->rec.data.TSN_seq, tp1->rec.data.rcv_flags);
		ret_sz += tp1->book_size;
		tp1->sent = SCTP_FORWARD_TSN_SKIP;
		if (tp1->data) {
			sctp_free_bufspace(tcb, &tcb->asoc, tp1);
			sctp_ulp_notify(SCTP_NOTIFY_DG_FAIL, tcb, reason, tp1);
			m_freem(tp1->data);
			tp1->data = NULL;
			sctp_sowwakeup(tcb->sctp_ep, tcb->sctp_socket);
		}
		if (tp1->flags & SCTP_PR_SCTP_BUFFER) {
			tcb->asoc.sent_queue_cnt_removeable--;
		}
		if (queue == &tcb->asoc.send_queue) {
			TAILQ_REMOVE(&tcb->asoc.send_queue, tp1, sctp_next);
			/* on to the sent queue */
			TAILQ_INSERT_TAIL(&tcb->asoc.sent_queue, tp1,
			    sctp_next);
			tcb->asoc.sent_queue_cnt++;
		}
		if ((tp1->rec.data.rcv_flags & SCTP_DATA_NOT_FRAG) == SCTP_DATA_NOT_FRAG) {
			/* not frag'ed we ae done   */
			notdone = 0;
			foundeom = 1;
		} else if (tp1->rec.data.rcv_flags & SCTP_DATA_LAST_FRAG) {
			/* end of frag, we are done */
			notdone = 0;
			foundeom = 1;
		} else {
			/* Its a begin or middle piece, we must mark all of it */
			notdone = 1;
			tp1 = TAILQ_NEXT(tp1, sctp_next);
		}
	} while (tp1 && notdone);
	if ((foundeom == 0) && (queue == &tcb->asoc.sent_queue)) {
		/* The multi-part message was scattered 
		 * across the send and sent queue.
		 */
		tp1 = TAILQ_FIRST(&tcb->asoc.send_queue);
		/* recurse throught the send_queue too, starting at the
		 * beginning.
		 */
		if (tp1) {
			printf("Recursing to the send_queue now\n");
			ret_sz += sctp_release_pr_sctp_chunk(tcb, tp1, reason,
			    &tcb->asoc.send_queue);
		} else {
			printf("hmm, nothing on the send queue and no EOM?\n");
		}
	}
	return (ret_sz);
}
