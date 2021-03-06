/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)dead_vnops.c	8.1 (Berkeley) 6/10/93
 * $Id: dead_vnops.c,v 1.10 1995/12/02 17:14:34 phk Exp $
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/vnode.h>
#include <sys/errno.h>
#include <sys/namei.h>
#include <sys/buf.h>

static int	chkvnlock __P((struct vnode *));
/*
 * Prototypes for dead operations on vnodes.
 */
static int	dead_badop __P((void));
static int	dead_ebadf __P((void));
static int	dead_lookup __P((struct vop_lookup_args *));
#define dead_create ((int (*) __P((struct  vop_create_args *)))dead_badop)
#define dead_mknod ((int (*) __P((struct  vop_mknod_args *)))dead_badop)
static int	dead_open __P((struct vop_open_args *));
#define dead_close ((int (*) __P((struct  vop_close_args *)))nullop)
#define dead_access ((int (*) __P((struct  vop_access_args *)))dead_ebadf)
#define dead_getattr ((int (*) __P((struct  vop_getattr_args *)))dead_ebadf)
#define dead_setattr ((int (*) __P((struct  vop_setattr_args *)))dead_ebadf)
static int	dead_read __P((struct vop_read_args *));
static int	dead_write __P((struct vop_write_args *));
static int	dead_ioctl __P((struct vop_ioctl_args *));
static int	dead_select __P((struct vop_select_args *));
#define dead_mmap ((int (*) __P((struct  vop_mmap_args *)))dead_badop)
#define dead_fsync ((int (*) __P((struct  vop_fsync_args *)))nullop)
#define dead_seek ((int (*) __P((struct  vop_seek_args *)))nullop)
#define dead_remove ((int (*) __P((struct  vop_remove_args *)))dead_badop)
#define dead_link ((int (*) __P((struct  vop_link_args *)))dead_badop)
#define dead_rename ((int (*) __P((struct  vop_rename_args *)))dead_badop)
#define dead_mkdir ((int (*) __P((struct  vop_mkdir_args *)))dead_badop)
#define dead_rmdir ((int (*) __P((struct  vop_rmdir_args *)))dead_badop)
#define dead_symlink ((int (*) __P((struct  vop_symlink_args *)))dead_badop)
#define dead_readdir ((int (*) __P((struct  vop_readdir_args *)))dead_ebadf)
#define dead_readlink ((int (*) __P((struct  vop_readlink_args *)))dead_ebadf)
#define dead_abortop ((int (*) __P((struct  vop_abortop_args *)))dead_badop)
#define dead_inactive ((int (*) __P((struct  vop_inactive_args *)))nullop)
#define dead_reclaim ((int (*) __P((struct  vop_reclaim_args *)))nullop)
static int	dead_lock __P((struct vop_lock_args *));
#define dead_unlock ((int (*) __P((struct  vop_unlock_args *)))nullop)
static int	dead_bmap __P((struct vop_bmap_args *));
static int	dead_strategy __P((struct vop_strategy_args *));
static int	dead_print __P((struct vop_print_args *));
#define dead_islocked ((int (*) __P((struct  vop_islocked_args *)))nullop)
#define dead_pathconf ((int (*) __P((struct  vop_pathconf_args *)))dead_ebadf)
#define dead_advlock ((int (*) __P((struct  vop_advlock_args *)))dead_ebadf)
#define dead_blkatoff ((int (*) __P((struct  vop_blkatoff_args *)))dead_badop)
#define dead_valloc ((int (*) __P((struct  vop_valloc_args *)))dead_badop)
#define dead_vfree ((int (*) __P((struct  vop_vfree_args *)))dead_badop)
#define dead_truncate ((int (*) __P((struct  vop_truncate_args *)))nullop)
#define dead_update ((int (*) __P((struct  vop_update_args *)))nullop)
#define dead_bwrite ((int (*) __P((struct  vop_bwrite_args *)))nullop)

vop_t **dead_vnodeop_p;
static struct vnodeopv_entry_desc dead_vnodeop_entries[] = {
	{ &vop_default_desc, (vop_t *)vn_default_error },
	{ &vop_lookup_desc, (vop_t *)dead_lookup },	/* lookup */
	{ &vop_create_desc, (vop_t *)dead_create },	/* create */
	{ &vop_mknod_desc, (vop_t *)dead_mknod },	/* mknod */
	{ &vop_open_desc, (vop_t *)dead_open },		/* open */
	{ &vop_close_desc, (vop_t *)dead_close },	/* close */
	{ &vop_access_desc, (vop_t *)dead_access },	/* access */
	{ &vop_getattr_desc, (vop_t *)dead_getattr },	/* getattr */
	{ &vop_setattr_desc, (vop_t *)dead_setattr },	/* setattr */
	{ &vop_read_desc, (vop_t *)dead_read },		/* read */
	{ &vop_write_desc, (vop_t *)dead_write },	/* write */
	{ &vop_ioctl_desc, (vop_t *)dead_ioctl },	/* ioctl */
	{ &vop_select_desc, (vop_t *)dead_select },	/* select */
	{ &vop_mmap_desc, (vop_t *)dead_mmap },		/* mmap */
	{ &vop_fsync_desc, (vop_t *)dead_fsync },	/* fsync */
	{ &vop_seek_desc, (vop_t *)dead_seek },		/* seek */
	{ &vop_remove_desc, (vop_t *)dead_remove },	/* remove */
	{ &vop_link_desc, (vop_t *)dead_link },		/* link */
	{ &vop_rename_desc, (vop_t *)dead_rename },	/* rename */
	{ &vop_mkdir_desc, (vop_t *)dead_mkdir },	/* mkdir */
	{ &vop_rmdir_desc, (vop_t *)dead_rmdir },	/* rmdir */
	{ &vop_symlink_desc, (vop_t *)dead_symlink },	/* symlink */
	{ &vop_readdir_desc, (vop_t *)dead_readdir },	/* readdir */
	{ &vop_readlink_desc, (vop_t *)dead_readlink },	/* readlink */
	{ &vop_abortop_desc, (vop_t *)dead_abortop },	/* abortop */
	{ &vop_inactive_desc, (vop_t *)dead_inactive },	/* inactive */
	{ &vop_reclaim_desc, (vop_t *)dead_reclaim },	/* reclaim */
	{ &vop_lock_desc, (vop_t *)dead_lock },		/* lock */
	{ &vop_unlock_desc, (vop_t *)dead_unlock },	/* unlock */
	{ &vop_bmap_desc, (vop_t *)dead_bmap },		/* bmap */
	{ &vop_strategy_desc, (vop_t *)dead_strategy },	/* strategy */
	{ &vop_print_desc, (vop_t *)dead_print },	/* print */
	{ &vop_islocked_desc, (vop_t *)dead_islocked },	/* islocked */
	{ &vop_pathconf_desc, (vop_t *)dead_pathconf },	/* pathconf */
	{ &vop_advlock_desc, (vop_t *)dead_advlock },	/* advlock */
	{ &vop_blkatoff_desc, (vop_t *)dead_blkatoff },	/* blkatoff */
	{ &vop_valloc_desc, (vop_t *)dead_valloc },	/* valloc */
	{ &vop_vfree_desc, (vop_t *)dead_vfree },	/* vfree */
	{ &vop_truncate_desc, (vop_t *)dead_truncate },	/* truncate */
	{ &vop_update_desc, (vop_t *)dead_update },	/* update */
	{ &vop_bwrite_desc, (vop_t *)dead_bwrite },	/* bwrite */
	{ NULL, NULL }
};
static struct vnodeopv_desc dead_vnodeop_opv_desc =
	{ &dead_vnodeop_p, dead_vnodeop_entries };

VNODEOP_SET(dead_vnodeop_opv_desc);

/*
 * Trivial lookup routine that always fails.
 */
/* ARGSUSED */
static int
dead_lookup(ap)
	struct vop_lookup_args /* {
		struct vnode * a_dvp;
		struct vnode ** a_vpp;
		struct componentname * a_cnp;
	} */ *ap;
{

	*ap->a_vpp = NULL;
	return (ENOTDIR);
}

/*
 * Open always fails as if device did not exist.
 */
/* ARGSUSED */
static int
dead_open(ap)
	struct vop_open_args /* {
		struct vnode *a_vp;
		int  a_mode;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap;
{

	return (ENXIO);
}

/*
 * Vnode op for read
 */
/* ARGSUSED */
static int
dead_read(ap)
	struct vop_read_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		int  a_ioflag;
		struct ucred *a_cred;
	} */ *ap;
{

	if (chkvnlock(ap->a_vp))
		panic("dead_read: lock");
	/*
	 * Return EOF for character devices, EIO for others
	 */
	if (ap->a_vp->v_type != VCHR)
		return (EIO);
	return (0);
}

/*
 * Vnode op for write
 */
/* ARGSUSED */
static int
dead_write(ap)
	struct vop_write_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		int  a_ioflag;
		struct ucred *a_cred;
	} */ *ap;
{

	if (chkvnlock(ap->a_vp))
		panic("dead_write: lock");
	return (EIO);
}

/*
 * Device ioctl operation.
 */
/* ARGSUSED */
static int
dead_ioctl(ap)
	struct vop_ioctl_args /* {
		struct vnode *a_vp;
		int  a_command;
		caddr_t  a_data;
		int  a_fflag;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap;
{

	if (!chkvnlock(ap->a_vp))
		return (EBADF);
	return (VCALL(ap->a_vp, VOFFSET(vop_ioctl), ap));
}

/* ARGSUSED */
static int
dead_select(ap)
	struct vop_select_args /* {
		struct vnode *a_vp;
		int  a_which;
		int  a_fflags;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap;
{

	/*
	 * Let the user find out that the descriptor is gone.
	 */
	return (1);
}

/*
 * Just call the device strategy routine
 */
static int
dead_strategy(ap)
	struct vop_strategy_args /* {
		struct buf *a_bp;
	} */ *ap;
{

	if (ap->a_bp->b_vp == NULL || !chkvnlock(ap->a_bp->b_vp)) {
		ap->a_bp->b_flags |= B_ERROR;
		biodone(ap->a_bp);
		return (EIO);
	}
	return (VOP_STRATEGY(ap->a_bp));
}

/*
 * Wait until the vnode has finished changing state.
 */
static int
dead_lock(ap)
	struct vop_lock_args /* {
		struct vnode *a_vp;
	} */ *ap;
{

	if (!chkvnlock(ap->a_vp))
		return (0);
	return (VCALL(ap->a_vp, VOFFSET(vop_lock), ap));
}

/*
 * Wait until the vnode has finished changing state.
 */
static int
dead_bmap(ap)
	struct vop_bmap_args /* {
		struct vnode *a_vp;
		daddr_t  a_bn;
		struct vnode **a_vpp;
		daddr_t *a_bnp;
		int *a_runp;
		int *a_runb;
	} */ *ap;
{

	if (!chkvnlock(ap->a_vp))
		return (EIO);
	return (VOP_BMAP(ap->a_vp, ap->a_bn, ap->a_vpp, ap->a_bnp, ap->a_runp, ap->a_runb));
}

/*
 * Print out the contents of a dead vnode.
 */
/* ARGSUSED */
static int
dead_print(ap)
	struct vop_print_args /* {
		struct vnode *a_vp;
	} */ *ap;
{

	printf("tag VT_NON, dead vnode\n");
	return (0);
}

/*
 * Empty vnode failed operation
 */
static int
dead_ebadf()
{

	return (EBADF);
}

/*
 * Empty vnode bad operation
 */
static int
dead_badop()
{

	panic("dead_badop called");
	/* NOTREACHED */
}

/*
 * We have to wait during times when the vnode is
 * in a state of change.
 */
int
chkvnlock(vp)
	register struct vnode *vp;
{
	int locked = 0;

	while (vp->v_flag & VXLOCK) {
		vp->v_flag |= VXWANT;
		(void) tsleep((caddr_t)vp, PINOD, "ckvnlk", 0);
		locked = 1;
	}
	return (locked);
}
