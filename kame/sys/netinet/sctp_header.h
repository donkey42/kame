/*    $KAME: sctp_header.h,v 1.5 2002/09/18 01:00:25 itojun Exp $     */
/*	Header: /home/sctpBsd/netinet/sctp_header.h,v 1.34 2002/04/03 21:10:19 lei Exp	*/

#ifndef __sctp_header_h__
#define __sctp_header_h__

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
#include <netinet/sctp.h>
#include <netinet/sctp_constants.h>
/*
 * Parameter structures
 */
struct sctp_ipv4addr_param {
	struct sctp_paramhdr ph;	/* type=SCTP_IPV4_PARAM_TYPE, len=8 */
	u_int32_t addr;			/* IPV4 address */
};

struct sctp_ipv6addr_param {
	struct sctp_paramhdr ph;	/* type=SCTP_IPV6_PARAM_TYPE, len=20 */
	u_int8_t  addr[16];		/* IPV6 address */
};

/* Cookie Preservative */
struct sctp_cookie_perserve_param {
	struct sctp_paramhdr ph;	/* type=SCTP_COOKIE_PRESERVE, len=8 */
	u_int32_t time;			/* time in ms to extend cookie */
};

/* Host Name Address */
struct sctp_host_name_param {
	struct sctp_paramhdr ph;	/* type=SCTP_HOSTNAME_ADDRESS */
	char name[1];			/* host name */
};

/* supported address type */
struct sctp_supported_addr_param {
	struct sctp_paramhdr ph;	/* type=SCTP_SUPPORTED_ADDRTYPE */
	u_int16_t addr_type[1];		/* array of supported address types */
};


/* heartbeat info parameter */
struct sctp_heartbeat_info_param {
	struct sctp_paramhdr ph;
	u_int32_t time_value_1;
	u_int32_t time_value_2;
	u_int16_t user_req;
        u_int8_t addr_family;
        u_int8_t addr_len;
	char address[SCTP_ADDRMAX];
};


/* draft-ietf-tsvwg-usctp */
/* unreliable streams parameter */
struct sctp_unrel_streams {
	u_int16_t start;		/* starting unrel stream id in range */
	u_int16_t end;			/* ending unrel stream id in range */
};

struct sctp_unrel_streams_param {
	struct sctp_paramhdr ph;
	struct sctp_unrel_streams unrel[0];	/* list of ranges */
};

struct sctp_strseq {
	u_int16_t stream;
	u_int16_t sequence;
};


/* draft-ietf-tsvwg-addip-sctp */
struct sctp_asconf_paramhdr {		/* an ASCONF "parameter" */
	struct sctp_paramhdr ph;	/* a SCTP parameter header */
	u_int32_t correlation_id;	/* correlation id for this param */
};

struct sctp_asconf_addr_param {		/* an ASCONF address parameter */
	struct sctp_asconf_paramhdr aph;	/* asconf "parameter" */
	struct sctp_ipv6addr_param  addrp;	/* max storage size */
};


/*
 * Structures for DATA chunks
 */
struct sctp_data {
	u_int32_t tsn;
	u_int16_t stream_id;
	u_int16_t stream_sequence;
	u_int32_t protocol_id;
	/* user data follows */
};

struct sctp_data_chunk {
	struct sctp_chunkhdr ch;
	struct sctp_data dp;
};

/*
 * Structures for the control chunks
 */

/* Initiate (INIT)/Initiate Ack (INIT ACK) */
struct sctp_init {
	u_int32_t initiate_tag;		/* initiate tag */
	u_int32_t a_rwnd;		/* a_rwnd */
	u_int16_t num_outbound_streams;	/* OS */
	u_int16_t num_inbound_streams;	/* MIS */
	u_int32_t initial_tsn;		/* I-TSN */
	/* optional param's follow */
};

/* state cookie header */
struct sctp_state_cookie {		/* this is our definition... */
	u_int32_t cookie_life;		/* life I will award this cookie */
        u_int32_t tie_tag_my_vtag;	/* my tag in old association */
        u_int32_t tie_tag_peer_vtag;    /* peers tag in old association */
        u_int32_t peers_vtag;		/* peers tag in INIT (for quick ref) */
	struct timeval time_entered;	/* the time I built cookie */
	u_int32_t address[4];		/* 4 ints/128 bits */
	u_int32_t addr_type;		/* address type */
	u_int32_t laddress[4];          /* my local from address */
	u_int32_t laddr_type;           /* my local from address type */
	u_int32_t scope_id;		/* v6 scope id for link-locals */
        u_int8_t ipv4_addr_legal;	/* Are V4 addr legal? */
        u_int8_t ipv6_addr_legal;	/* Are V6 addr legal? */
	u_int8_t local_scope;		/* IPv6 local scope flag */
	u_int8_t site_scope;		/* IPv6 site scope flag */
        u_int8_t ipv4_scope;		/* IPv4 private addr scope */
        u_int8_t loopback_scope;	/* loopback scope information */
        u_int16_t reserved;
	/*
	 * at the end is tacked on the INIT chunk and the
	 * INIT-ACK chunk (minus the cookie).
	 */
};

struct sctp_inv_mandatory_param {
	u_int16_t cause;
	u_int16_t length;
	u_int32_t num_param;
	u_int16_t param;
	/*
	 * We include this to 0 it since only a missing cookie
	 * will cause this error.
	 */
	u_int16_t resv;
};

struct sctp_unresolv_addr {
	u_int16_t cause;
	u_int16_t length;
	u_int16_t addr_type;
	u_int16_t reserved;	/* Only one invalid addr type */
};

/* state cookie parameter */
struct sctp_state_cookie_param {
	struct sctp_paramhdr ph;
	struct sctp_state_cookie cookie;
};

struct sctp_init_chunk {
	struct sctp_chunkhdr ch;
	struct sctp_init init;
};

struct sctp_init_msg {
	struct sctphdr sh;
	struct sctp_init_chunk msg;
};
/* ... used for both INIT and INIT ACK */
#define sctp_init_ack		sctp_init
#define sctp_init_ack_chunk	sctp_init_chunk
#define sctp_init_ack_msg	sctp_init_msg


/* Selective Ack (SACK) */
struct sctp_gap_ack_block {
	u_int16_t start;		/* Gap Ack block start */
	u_int16_t end;			/* Gap Ack block end */
};

struct sctp_sack {
	u_int32_t cum_tsn_ack;		/* cumulative TSN Ack */
	u_int32_t a_rwnd;		/* updated a_rwnd of sender */
	u_int16_t num_gap_ack_blks;	/* number of Gap Ack blocks */
	u_int16_t num_dup_tsns;		/* number of duplicate TSNs */
	/* struct sctp_gap_ack_block's follow */
	/* u_int32_t duplicate_tsn's follow */
};

struct sctp_sack_chunk {
	struct sctp_chunkhdr ch;
	struct sctp_sack sack;
};


/* Heartbeat Request (HEARTBEAT) */
struct sctp_heartbeat {
	struct sctp_heartbeat_info_param hb_info;
};

struct sctp_heartbeat_chunk {
	struct sctp_chunkhdr ch;
	struct sctp_heartbeat heartbeat;
};
/* ... used for Heartbeat Ack (HEARTBEAT ACK) */
#define sctp_heartbeat_ack		sctp_heartbeat
#define sctp_heartbeat_ack_chunk	sctp_heartbeat_chunk


/* Abort Asssociation (ABORT) */
struct sctp_abort_chunk {
	struct sctp_chunkhdr ch;
	/* optional error cause may follow */
};

struct sctp_abort_msg {
	struct sctphdr sh;
	struct sctp_abort_chunk msg;
};


/* Shutdown Association (SHUTDOWN) */
struct sctp_shutdown_chunk {
	struct sctp_chunkhdr ch;
	u_int32_t cumulative_tsn_ack;
};


/* Shutdown Acknowledgment (SHUTDOWN ACK) */
struct sctp_shutdown_ack_chunk {
	struct sctp_chunkhdr ch;
};


/* Operation Error (ERROR) */
struct sctp_error_chunk {
	struct sctp_chunkhdr ch;
	/* optional error causes follow */
};


/* Cookie Echo (COOKIE ECHO) */
struct sctp_cookie_echo_chunk {
	struct sctp_chunkhdr ch;
	struct sctp_state_cookie cookie;
};

/* Cookie Acknowledgment (COOKIE ACK) */
struct sctp_cookie_ack_chunk {
	struct sctp_chunkhdr ch;
};

/* Explicit Congestion Notification Echo (ECNE) */
struct sctp_ecne_chunk {
	struct sctp_chunkhdr ch;
	u_int32_t tsn;
};

/* Congestion Window Reduced (CWR) */
struct sctp_cwr_chunk {
	struct sctp_chunkhdr ch;
	u_int32_t tsn;
};

/* Shutdown Complete (SHUTDOWN COMPLETE) */
struct sctp_shutdown_complete_chunk {
	struct sctp_chunkhdr ch;
};

/* Oper error holding a stale cookie */
struct sctp_stale_cookie_msg {
	struct sctphdr sh;
	struct sctp_chunkhdr ch;
	struct sctp_paramhdr ph;	/* really an error cause */
	u_int32_t time_usec;
};

struct sctp_adaption_layer_indication {
	struct sctp_paramhdr ph;
	u_int32_t indication;
};

struct sctp_cookie_while_shutting_down {
	struct sctphdr sh;
	struct sctp_chunkhdr ch;
	struct sctp_paramhdr ph;	/* really an error cause */

};

struct sctp_shutdown_complete_msg {
        struct sctphdr sh;
        struct sctp_shutdown_complete_chunk shut_cmp;
};

/* draft-ietf-tsvwg-addip-sctp */
/* Address/Stream Configuration Change (ASCONF) */
struct sctp_asconf_chunk {
	struct sctp_chunkhdr ch;
	u_int32_t serial_number;
	/* lookup address parameter (mandatory) */
	/* asconf parameters follow */
};

/* Address/Stream Configuration Acknowledge (ASCONF ACK) */
struct sctp_asconf_ack_chunk {
	struct sctp_chunkhdr ch;
	u_int32_t serial_number;
	/* asconf parameters follow */
};

/* draft-ietf-tsvwg-usctp */
/* Forward Cumulative TSN (FORWARD TSN) */
struct sctp_forward_tsn_chunk {
	struct sctp_chunkhdr ch;
	u_int32_t new_cumulative_tsn;
};

struct sctp_forward_tsn_msg {
	struct sctphdr sh;
	struct sctp_forward_tsn_chunk msg;
};

/*
 * we pre-reserve enough room for a ECNE or CWR AND a SACK with no
 * missing pieces. If ENCE is missing we could have a couple of blocks.
 * This way we optimize so we MOST likely can bundle a SACK/ECN with
 * the smallest size data chunk I will split into. We could increase
 * throughput slightly by taking out these two but the  24-sack/8-CWR
 * i.e. 32 bytes I pre-reserve I feel is worth it for now.
 */
#ifndef SCTP_MAX_OVERHEAD
#ifdef AF_INET6
#define SCTP_MAX_OVERHEAD (sizeof(struct sctp_data_chunk) + \
			   sizeof(struct sctphdr) + \
			   sizeof(struct sctp_ecne_chunk) + \
			   sizeof(struct sctp_sack_chunk) + \
			   sizeof(struct ip6_hdr))

#define SCTP_MIN_OVERHEAD (sizeof(struct ip6_hdr) + \
                           sizeof(struct sctphdr))

#else
#define SCTP_MAX_OVERHEAD (sizeof(struct sctp_data_chunk) + \
			   sizeof(struct sctphdr) + \
			   sizeof(struct sctp_ecne_chunk) + \
			   sizeof(struct sctp_sack_chunk) + \
			   sizeof(struct ip))

#define SCTP_MIN_OVERHEAD (sizeof(struct ip) + \
                           sizeof(struct sctphdr))

#endif /* AF_INET6 */
#endif /* !SCTP_MAX_OVERHEAD */

#endif /* !__sctp_header_h__ */