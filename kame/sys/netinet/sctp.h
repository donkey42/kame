/*    $KAME: sctp.h,v 1.13 2003/08/29 06:37:38 itojun Exp $    */
/*	Header: /home/sctpBsd/netinet/sctp.h,v 1.43 2002/04/02 15:34:44 lei Exp	*/


#ifndef _NETINET_SCTP_H_
#define _NETINET_SCTP_H_

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

/*
 * SCTP protocol - RFC2960.
 */

struct sctphdr {
	u_int16_t src_port;		/* source port */
	u_int16_t dest_port;		/* destination port */
	u_int32_t v_tag;		/* verification tag of packet */
	u_int32_t checksum;		/* Adler32 C-Sum */
	/* chunks follow... */
};

/*
 * SCTP Chunks
 */
struct sctp_chunkhdr {
	u_int8_t  chunk_type;		/* chunk type */
	u_int8_t  chunk_flags;		/* chunk flags */
	u_int16_t chunk_length;		/* chunk length */
	/* optional params follow */
};

/*
 * SCTP chunk parameters
 */
struct sctp_paramhdr {
	u_int16_t param_type;		/* parameter type */
	u_int16_t param_length;		/* parameter length */
};


/*
 * user socket options
 */
/* read-write options */
#define SCTP_NODELAY			0x00000001
#define SCTP_MAXSEG			0x00000002
#define SCTP_ASSOCINFO			0x00000004

#define SCTP_INITMSG			0x00000008
#define SCTP_AUTOCLOSE			0x00000010
#define SCTP_SET_PEER_PRIMARY_ADDR	0x00000020
#define SCTP_PRIMARY_ADDR		0x00000040

/* read-only options */
#define SCTP_STATUS			0x00000080
#define SCTP_PCB_STATUS			0x00000100

/* ancillary data/notification interest options */
#define SCTP_EVENTS			0x00000200
/* sctp_opt_info params */
#define SCTP_PEER_ADDR_PARAMS 		0x00000400
#define SCTP_GET_PEER_ADDR_INFO		0x00001000
/* Hidden socket option that gets the addresses */
#define SCTP_GET_PEER_ADDRESSES		0x00002000
#define SCTP_GET_LOCAL_ADDRESSES	0x00004000
/*
 * Blocking I/O is enabled on any TCP type socket by default.
 * For the UDP model if this is turned on then the socket buffer is
 * shared for send resources amongst all associations. The default
 * for the UDP model is that is SS_NBIO is set. Which means all associations
 * have a seperate send limit BUT they will NOT ever BLOCK instead
 * you will get an error back EAGAIN if you try to send to much. If
 * you want the blocking symantics you set this option at the cost
 * of sharing one socket send buffer size amongst all associations.
 * Peeled off sockets turn this option off and block... but since both TCP and
 * peeled off sockets have only one assoc per socket this is fine.
 * It probably does NOT make sense to set this  on SS_NBIO on a TCP model OR
 * peeled off UDP model, but we do allow you to do so. You just use
 * the normal syscall to toggle SS_NBIO the way you want.
 */
/* Blocking I/O is controled by the SS_NBIO flag on the
 * socket state so_state field.
 */
#define SCTP_GET_SNDBUF_USE		0x00008000
/* latter added read/write */
#define SCTP_ADAPTION_LAYER		0x00010000
#define SCTP_DISABLE_FRAGMENTS		0x00020000
/* sctp_bindx() flags as socket options */
#define SCTP_BINDX_ADD_ADDR		0x00040000
#define SCTP_BINDX_REM_ADDR		0x00080000
/* return the total count in bytes needed to hold all local addresses bound */
#define SCTP_GET_LOCAL_ADDR_SIZE	0x00100000
/* Without this applied we will give V4 and V6 addresses on a V6 socket */
#define SCTP_I_WANT_MAPPED_V4_ADDR	0x00200000
/* Return the total count in bytes needed to hold the remote address */
#define SCTP_GET_REMOTE_ADDR_SIZE	0x00400000
#define SCTP_GET_PEGS			0x00800000
#define SCTP_DEFAULT_SEND_PARAM		0x01000000
#define SCTP_SET_DEBUG_LEVEL		0x02000000
#define SCTP_RTOINFO			0x04000000
#define SCTP_AUTO_ASCONF		0x08000000
#define SCTP_MAXBURST			0x10000000
#define SCTP_GET_CWND_LOG		0x20000000
#define SCTP_CONNECT_X			0x40000000	/* hidden opt for connectx */
/* Test/debug get a route */

/* Debug things that need to be purged */
#define SCTP_SET_INITIAL_DBG_SEQ	0x80000000

/*
 * user state values
 */
#define SCTP_CLOSED			0x0000
#define SCTP_BOUND			0x1000
#define SCTP_LISTEN			0x2000
#define SCTP_COOKIE_WAIT		0x0002
#define SCTP_COOKIE_ECHOED		0x0004
#define SCTP_ESTABLISHED		0x0008
#define SCTP_SHUTDOWN_SENT		0x0010
#define SCTP_SHUTDOWN_RECEIVED		0x0020
#define SCTP_SHUTDOWN_ACK_SENT		0x0040
#define SCTP_SHUTDOWN_PENDING		0x0080

/*
 * SCTP operational error codes (user visible)
 */
#define SCTP_ERROR_NO_ERROR		0x0000
#define SCTP_ERROR_INVALID_STREAM	0x0001
#define SCTP_ERROR_MISSING_PARAM	0x0002
#define SCTP_ERROR_STALE_COOKIE		0x0003
#define SCTP_ERROR_OUT_OF_RESOURCES	0x0004
#define SCTP_ERROR_UNRESOLVABLE_ADDR	0x0005
#define SCTP_ERROR_UNRECOG_CHUNK	0x0006
#define SCTP_ERROR_INVALID_PARAM	0x0007
#define SCTP_ERROR_UNRECOG_PARAM	0x0008
#define SCTP_ERROR_NO_USER_DATA		0x0009
#define SCTP_ERROR_COOKIE_IN_SHUTDOWN	0x000a
/* draft-ietf-tsvwg-sctpimpguide */
#define SCTP_ERROR_RESTART_NEWADDRS	0x000b
/* draft-ietf-tsvwg-addip-sctp */
#define SCTP_ERROR_DELETE_LAST_ADDR	0x0100
#define SCTP_ERROR_RESOURCE_SHORTAGE	0x0101
#define SCTP_ERROR_DELETE_SOURCE_ADDR	0x0102
#define SCTP_ERROR_ILLEGAL_ASCONF_ACK	0x0103

/*
 * error cause parameters (user visisble)
 */
struct sctp_error_cause {
	u_int16_t code;
	u_int16_t length;
	/* optional cause-specific info may follow */
};

struct sctp_error_invalid_stream {
	struct sctp_error_cause cause;	/* code=SCTP_ERROR_INVALID_STREAM */
	u_int16_t stream_id;		/* stream id of the DATA in error */
	u_int16_t reserved;
};

struct sctp_error_missing_param {
	struct sctp_error_cause cause;	/* code=SCTP_ERROR_MISSING_PARAM */
	u_int32_t num_missing_params;	/* number of missing parameters */
	/* u_int16_t param_type's follow */
};

struct sctp_error_stale_cookie {
	struct sctp_error_cause cause;	/* code=SCTP_ERROR_STALE_COOKIE */
	u_int32_t stale_time;		/* time in usec of staleness */
};

struct sctp_error_out_of_resource {
	struct sctp_error_cause cause;	/* code=SCTP_ERROR_OUT_OF_RESOURCES */
};

struct sctp_error_unresolv_addr {
	struct sctp_error_cause cause;	/* code=SCTP_ERROR_UNRESOLVABLE_ADDR */

};

struct sctp_error_unrecognized_chunk {
	struct sctp_error_cause cause;	/* code=SCTP_ERROR_UNRECOG_CHUNK */
	struct sctp_chunkhdr ch;	/* header from chunk in error */
};

#define HAVE_SCTP			1
#define HAVE_KERNEL_SCTP		1
#define HAVE_SCTP_PRSCTP		1
#define HAVE_SCTP_ADDIP			1
#define HAVE_SCTP_CANSET_PRIMARY	1
#define HAVE_SCTP_SAT_NETWORK_CAPABILITY1
#define HAVE_SCTP_MULTIBUF              1
#define HAVE_SCTP_NOCONNECT             0

#include <netinet/sctp_uio.h>

#endif /* !_NETINET_SCTP_H_ */
