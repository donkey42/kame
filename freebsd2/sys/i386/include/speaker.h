/*
 * speaker.h -- interface definitions for speaker ioctl()
 *
 * v1.4 by Eric S. Raymond (esr@snark.thyrsus.com) Aug 1993
 *      modified for FreeBSD by Andrew A. Chernov <ache@astral.msk.su>
 *
 * $Id: speaker.h,v 1.3 1996/09/21 14:58:26 bde Exp $
 */

#ifndef	_MACHINE_SPEAKER_H_
#define	_MACHINE_SPEAKER_H_

#include <sys/ioccom.h>

#define SPKRTONE        _IOW('S', 1, tone_t)    /* emit tone */
#define SPKRTUNE        _IO('S', 2)             /* emit tone sequence*/

typedef struct
{
    int	frequency;	/* in hertz */
    int duration;	/* in 1/100ths of a second */
}
tone_t;

/*
 * Strings written to the speaker device are interpreted as tunes and played;
 * see the spkr(4) man page for details.
 */

#endif /* !_MACHINE_SPEAKER_H_ */
