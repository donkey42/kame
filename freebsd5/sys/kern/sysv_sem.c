/* $FreeBSD: src/sys/kern/sysv_sem.c,v 1.56 2002/10/19 02:07:35 alfred Exp $ */

/*
 * Implementation of SVID semaphores
 *
 * Author:  Daniel Boulet
 *
 * This software is provided ``AS IS'' without any warranties of any kind.
 */

#include "opt_sysvipc.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/sysproto.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/lock.h>
#include <sys/mutex.h>
#include <sys/sem.h>
#include <sys/syscall.h>
#include <sys/sysent.h>
#include <sys/sysctl.h>
#include <sys/malloc.h>
#include <sys/jail.h>

static MALLOC_DEFINE(M_SEM, "sem", "SVID compatible semaphores");

#ifdef SEM_DEBUG
#define DPRINTF(a)	printf a
#else
#define DPRINTF(a)
#endif

static void seminit(void);
static int sysvsem_modload(struct module *, int, void *);
static int semunload(void);
static void semexit_myhook(struct proc *p);
static int sysctl_sema(SYSCTL_HANDLER_ARGS);
static int semvalid(int semid, struct semid_ds *semaptr);

#ifndef _SYS_SYSPROTO_H_
struct __semctl_args;
int __semctl(struct thread *td, struct __semctl_args *uap);
struct semget_args;
int semget(struct thread *td, struct semget_args *uap);
struct semop_args;
int semop(struct thread *td, struct semop_args *uap);
#endif

static struct sem_undo *semu_alloc(struct thread *td);
static int semundo_adjust(struct thread *td, struct sem_undo **supptr, 
		int semid, int semnum, int adjval);
static void semundo_clear(int semid, int semnum);

/* XXX casting to (sy_call_t *) is bogus, as usual. */
static sy_call_t *semcalls[] = {
	(sy_call_t *)__semctl, (sy_call_t *)semget,
	(sy_call_t *)semop
};

static struct mtx	sem_mtx;	/* semaphore global lock */
static int	semtot = 0;
static struct semid_ds *sema;	/* semaphore id pool */
static struct mtx *sema_mtx;	/* semaphore id pool mutexes*/
static struct sem *sem;		/* semaphore pool */
SLIST_HEAD(, sem_undo) semu_list;	/* list of active undo structures */
static int	*semu;		/* undo structure pool */

#define SEMUNDO_MTX		sem_mtx
#define SEMUNDO_LOCK()		mtx_lock(&SEMUNDO_MTX);
#define SEMUNDO_UNLOCK()	mtx_unlock(&SEMUNDO_MTX);
#define SEMUNDO_LOCKASSERT(how)	mtx_assert(&SEMUNDO_MTX, (how));

struct sem {
	u_short	semval;		/* semaphore value */
	pid_t	sempid;		/* pid of last operation */
	u_short	semncnt;	/* # awaiting semval > cval */
	u_short	semzcnt;	/* # awaiting semval = 0 */
};

/*
 * Undo structure (one per process)
 */
struct sem_undo {
	SLIST_ENTRY(sem_undo) un_next;	/* ptr to next active undo structure */
	struct	proc *un_proc;		/* owner of this structure */
	short	un_cnt;			/* # of active entries */
	struct undo {
		short	un_adjval;	/* adjust on exit values */
		short	un_num;		/* semaphore # */
		int	un_id;		/* semid */
	} un_ent[1];			/* undo entries */
};

/*
 * Configuration parameters
 */
#ifndef SEMMNI
#define SEMMNI	10		/* # of semaphore identifiers */
#endif
#ifndef SEMMNS
#define SEMMNS	60		/* # of semaphores in system */
#endif
#ifndef SEMUME
#define SEMUME	10		/* max # of undo entries per process */
#endif
#ifndef SEMMNU
#define SEMMNU	30		/* # of undo structures in system */
#endif

/* shouldn't need tuning */
#ifndef SEMMAP
#define SEMMAP	30		/* # of entries in semaphore map */
#endif
#ifndef SEMMSL
#define SEMMSL	SEMMNS		/* max # of semaphores per id */
#endif
#ifndef SEMOPM
#define SEMOPM	100		/* max # of operations per semop call */
#endif

#define SEMVMX	32767		/* semaphore maximum value */
#define SEMAEM	16384		/* adjust on exit max value */

/*
 * Due to the way semaphore memory is allocated, we have to ensure that
 * SEMUSZ is properly aligned.
 */

#define SEM_ALIGN(bytes) (((bytes) + (sizeof(long) - 1)) & ~(sizeof(long) - 1))

/* actual size of an undo structure */
#define SEMUSZ	SEM_ALIGN(offsetof(struct sem_undo, un_ent[SEMUME]))

/*
 * Macro to find a particular sem_undo vector
 */
#define SEMU(ix) \
	((struct sem_undo *)(((intptr_t)semu)+ix * seminfo.semusz))

/*
 * semaphore info struct
 */
struct seminfo seminfo = {
                SEMMAP,         /* # of entries in semaphore map */
                SEMMNI,         /* # of semaphore identifiers */
                SEMMNS,         /* # of semaphores in system */
                SEMMNU,         /* # of undo structures in system */
                SEMMSL,         /* max # of semaphores per id */
                SEMOPM,         /* max # of operations per semop call */
                SEMUME,         /* max # of undo entries per process */
                SEMUSZ,         /* size in bytes of undo structure */
                SEMVMX,         /* semaphore maximum value */
                SEMAEM          /* adjust on exit max value */
};

SYSCTL_DECL(_kern_ipc);
SYSCTL_INT(_kern_ipc, OID_AUTO, semmap, CTLFLAG_RW, &seminfo.semmap, 0, "");
SYSCTL_INT(_kern_ipc, OID_AUTO, semmni, CTLFLAG_RD, &seminfo.semmni, 0, "");
SYSCTL_INT(_kern_ipc, OID_AUTO, semmns, CTLFLAG_RD, &seminfo.semmns, 0, "");
SYSCTL_INT(_kern_ipc, OID_AUTO, semmnu, CTLFLAG_RD, &seminfo.semmnu, 0, "");
SYSCTL_INT(_kern_ipc, OID_AUTO, semmsl, CTLFLAG_RW, &seminfo.semmsl, 0, "");
SYSCTL_INT(_kern_ipc, OID_AUTO, semopm, CTLFLAG_RD, &seminfo.semopm, 0, "");
SYSCTL_INT(_kern_ipc, OID_AUTO, semume, CTLFLAG_RD, &seminfo.semume, 0, "");
SYSCTL_INT(_kern_ipc, OID_AUTO, semusz, CTLFLAG_RD, &seminfo.semusz, 0, "");
SYSCTL_INT(_kern_ipc, OID_AUTO, semvmx, CTLFLAG_RW, &seminfo.semvmx, 0, "");
SYSCTL_INT(_kern_ipc, OID_AUTO, semaem, CTLFLAG_RW, &seminfo.semaem, 0, "");
SYSCTL_PROC(_kern_ipc, OID_AUTO, sema, CTLFLAG_RD,
    NULL, 0, sysctl_sema, "", "");

static void
seminit(void)
{
	int i;

	TUNABLE_INT_FETCH("kern.ipc.semmap", &seminfo.semmap);
	TUNABLE_INT_FETCH("kern.ipc.semmni", &seminfo.semmni);
	TUNABLE_INT_FETCH("kern.ipc.semmns", &seminfo.semmns);
	TUNABLE_INT_FETCH("kern.ipc.semmnu", &seminfo.semmnu);
	TUNABLE_INT_FETCH("kern.ipc.semmsl", &seminfo.semmsl);
	TUNABLE_INT_FETCH("kern.ipc.semopm", &seminfo.semopm);
	TUNABLE_INT_FETCH("kern.ipc.semume", &seminfo.semume);
	TUNABLE_INT_FETCH("kern.ipc.semusz", &seminfo.semusz);
	TUNABLE_INT_FETCH("kern.ipc.semvmx", &seminfo.semvmx);
	TUNABLE_INT_FETCH("kern.ipc.semaem", &seminfo.semaem);

	sem = malloc(sizeof(struct sem) * seminfo.semmns, M_SEM, M_WAITOK);
	sema = malloc(sizeof(struct semid_ds) * seminfo.semmni, M_SEM,
	    M_WAITOK);
	sema_mtx = malloc(sizeof(struct mtx) * seminfo.semmni, M_SEM,
	    M_WAITOK | M_ZERO);
	semu = malloc(seminfo.semmnu * seminfo.semusz, M_SEM, M_WAITOK);

	for (i = 0; i < seminfo.semmni; i++) {
		sema[i].sem_base = 0;
		sema[i].sem_perm.mode = 0;
	}
	for (i = 0; i < seminfo.semmni; i++)
		mtx_init(&sema_mtx[i], "semid", NULL, MTX_DEF);
	for (i = 0; i < seminfo.semmnu; i++) {
		struct sem_undo *suptr = SEMU(i);
		suptr->un_proc = NULL;
	}
	SLIST_INIT(&semu_list);
	at_exit(semexit_myhook);
	mtx_init(&sem_mtx, "sem", NULL, MTX_DEF);
}

static int
semunload(void)
{
	int i;

	if (semtot != 0)
		return (EBUSY);

	free(sem, M_SEM);
	free(sema, M_SEM);
	free(semu, M_SEM);
	rm_at_exit(semexit_myhook);
	for (i = 0; i < seminfo.semmni; i++)
		mtx_destroy(&sema_mtx[i]);
	mtx_destroy(&sem_mtx);
	return (0);
}

static int
sysvsem_modload(struct module *module, int cmd, void *arg)
{
	int error = 0;

	switch (cmd) {
	case MOD_LOAD:
		seminit();
		break;
	case MOD_UNLOAD:
		error = semunload();
		break;
	case MOD_SHUTDOWN:
		break;
	default:
		error = EINVAL;
		break;
	}
	return (error);
}

static moduledata_t sysvsem_mod = {
	"sysvsem",
	&sysvsem_modload,
	NULL
};

SYSCALL_MODULE_HELPER(semsys);
SYSCALL_MODULE_HELPER(__semctl);
SYSCALL_MODULE_HELPER(semget);
SYSCALL_MODULE_HELPER(semop);

DECLARE_MODULE(sysvsem, sysvsem_mod,
	SI_SUB_SYSV_SEM, SI_ORDER_FIRST);
MODULE_VERSION(sysvsem, 1);

/*
 * Entry point for all SEM calls
 *
 * MPSAFE
 */
int
semsys(td, uap)
	struct thread *td;
	/* XXX actually varargs. */
	struct semsys_args /* {
		u_int	which;
		int	a2;
		int	a3;
		int	a4;
		int	a5;
	} */ *uap;
{
	int error;

	if (!jail_sysvipc_allowed && jailed(td->td_ucred))
		return (ENOSYS);
	if (uap->which >= sizeof(semcalls)/sizeof(semcalls[0]))
		return (EINVAL);
	error = (*semcalls[uap->which])(td, &uap->a2);
	return (error);
}

/*
 * Allocate a new sem_undo structure for a process
 * (returns ptr to structure or NULL if no more room)
 */

static struct sem_undo *
semu_alloc(td)
	struct thread *td;
{
	int i;
	struct sem_undo *suptr;
	struct sem_undo **supptr;
	int attempt;

	SEMUNDO_LOCKASSERT(MA_OWNED);
	/*
	 * Try twice to allocate something.
	 * (we'll purge any empty structures after the first pass so
	 * two passes are always enough)
	 */

	for (attempt = 0; attempt < 2; attempt++) {
		/*
		 * Look for a free structure.
		 * Fill it in and return it if we find one.
		 */

		for (i = 0; i < seminfo.semmnu; i++) {
			suptr = SEMU(i);
			if (suptr->un_proc == NULL) {
				SLIST_INSERT_HEAD(&semu_list, suptr, un_next);
				suptr->un_cnt = 0;
				suptr->un_proc = td->td_proc;
				return(suptr);
			}
		}

		/*
		 * We didn't find a free one, if this is the first attempt
		 * then try to free some structures.
		 */

		if (attempt == 0) {
			/* All the structures are in use - try to free some */
			int did_something = 0;

			SLIST_FOREACH_PREVPTR(suptr, supptr, &semu_list,
			    un_next) {
				if (suptr->un_cnt == 0) {
					suptr->un_proc = NULL;
					did_something = 1;
					*supptr = SLIST_NEXT(suptr, un_next);
				}
			}

			/* If we didn't free anything then just give-up */
			if (!did_something)
				return(NULL);
		} else {
			/*
			 * The second pass failed even though we freed
			 * something after the first pass!
			 * This is IMPOSSIBLE!
			 */
			panic("semu_alloc - second attempt failed");
		}
	}
	return (NULL);
}

/*
 * Adjust a particular entry for a particular proc
 */

static int
semundo_adjust(td, supptr, semid, semnum, adjval)
	struct thread *td;
	struct sem_undo **supptr;
	int semid, semnum;
	int adjval;
{
	struct proc *p = td->td_proc;
	struct sem_undo *suptr;
	struct undo *sunptr;
	int i;

	SEMUNDO_LOCKASSERT(MA_OWNED);
	/* Look for and remember the sem_undo if the caller doesn't provide
	   it */

	suptr = *supptr;
	if (suptr == NULL) {
		SLIST_FOREACH(suptr, &semu_list, un_next) {
			if (suptr->un_proc == p) {
				*supptr = suptr;
				break;
			}
		}
		if (suptr == NULL) {
			if (adjval == 0)
				return(0);
			suptr = semu_alloc(td);
			if (suptr == NULL)
				return(ENOSPC);
			*supptr = suptr;
		}
	}

	/*
	 * Look for the requested entry and adjust it (delete if adjval becomes
	 * 0).
	 */
	sunptr = &suptr->un_ent[0];
	for (i = 0; i < suptr->un_cnt; i++, sunptr++) {
		if (sunptr->un_id != semid || sunptr->un_num != semnum)
			continue;
		if (adjval != 0) {
			adjval += sunptr->un_adjval;
			if (adjval > seminfo.semaem || adjval < -seminfo.semaem)
				return (ERANGE);
		}
		sunptr->un_adjval = adjval;
		if (sunptr->un_adjval == 0) {
			suptr->un_cnt--;
			if (i < suptr->un_cnt)
				suptr->un_ent[i] =
				    suptr->un_ent[suptr->un_cnt];
		}
		return(0);
	}

	/* Didn't find the right entry - create it */
	if (adjval == 0)
		return(0);
	if (adjval > seminfo.semaem || adjval < -seminfo.semaem)
		return (ERANGE);
	if (suptr->un_cnt != seminfo.semume) {
		sunptr = &suptr->un_ent[suptr->un_cnt];
		suptr->un_cnt++;
		sunptr->un_adjval = adjval;
		sunptr->un_id = semid; sunptr->un_num = semnum;
	} else
		return(EINVAL);
	return(0);
}

static void
semundo_clear(semid, semnum)
	int semid, semnum;
{
	struct sem_undo *suptr;

	SEMUNDO_LOCKASSERT(MA_OWNED);
	SLIST_FOREACH(suptr, &semu_list, un_next) {
		struct undo *sunptr = &suptr->un_ent[0];
		int i = 0;

		while (i < suptr->un_cnt) {
			if (sunptr->un_id == semid) {
				if (semnum == -1 || sunptr->un_num == semnum) {
					suptr->un_cnt--;
					if (i < suptr->un_cnt) {
						suptr->un_ent[i] =
						  suptr->un_ent[suptr->un_cnt];
						continue;
					}
				}
				if (semnum != -1)
					break;
			}
			i++, sunptr++;
		}
	}
}

static int
semvalid(semid, semaptr)
	int semid;
	struct semid_ds *semaptr;
{

	return ((semaptr->sem_perm.mode & SEM_ALLOC) == 0 ||
	    semaptr->sem_perm.seq != IPCID_TO_SEQ(semid) ? EINVAL : 0);
}

/*
 * Note that the user-mode half of this passes a union, not a pointer
 */
#ifndef _SYS_SYSPROTO_H_
struct __semctl_args {
	int	semid;
	int	semnum;
	int	cmd;
	union	semun *arg;
};
#endif

/*
 * MPSAFE
 */
int
__semctl(td, uap)
	struct thread *td;
	struct __semctl_args *uap;
{
	int semid = uap->semid;
	int semnum = uap->semnum;
	int cmd = uap->cmd;
	u_short *array;
	union semun *arg = uap->arg;
	union semun real_arg;
	struct ucred *cred = td->td_ucred;
	int i, rval, error;
	struct semid_ds sbuf;
	struct semid_ds *semaptr;
	struct mtx *sema_mtxp;
	u_short usval, count;

	DPRINTF(("call to semctl(%d, %d, %d, 0x%x)\n",
	    semid, semnum, cmd, arg));
	if (!jail_sysvipc_allowed && jailed(td->td_ucred))
		return (ENOSYS);

	array = NULL;

	switch(cmd) {
	case SEM_STAT:
		if (semid < 0 || semid >= seminfo.semmni)
			return (EINVAL);
		if ((error = copyin(arg, &real_arg, sizeof(real_arg))) != 0)
			return (error);
		semaptr = &sema[semid];
		sema_mtxp = &sema_mtx[semid];
		mtx_lock(sema_mtxp);
		if ((semaptr->sem_perm.mode & SEM_ALLOC) == 0 ) {
			error = EINVAL;
			goto done2;
		}
		if ((error = ipcperm(td, &semaptr->sem_perm, IPC_R)))
			goto done2;
		mtx_unlock(sema_mtxp);
		error = copyout(semaptr, real_arg.buf, sizeof(struct semid_ds));
		rval = IXSEQ_TO_IPCID(semid,semaptr->sem_perm);
		if (error == 0)
			td->td_retval[0] = rval;
		return (error);
	}

	semid = IPCID_TO_IX(semid);
	if (semid < 0 || semid >= seminfo.semmni)
		return (EINVAL);

	semaptr = &sema[semid];
	sema_mtxp = &sema_mtx[semid];
		
	error = 0;
	rval = 0;

	switch (cmd) {
	case IPC_RMID:
		mtx_lock(sema_mtxp);
		if ((error = semvalid(uap->semid, semaptr)) != 0)
			goto done2;
		if ((error = ipcperm(td, &semaptr->sem_perm, IPC_M)))
			goto done2;
		semaptr->sem_perm.cuid = cred->cr_uid;
		semaptr->sem_perm.uid = cred->cr_uid;
		semtot -= semaptr->sem_nsems;
		for (i = semaptr->sem_base - sem; i < semtot; i++)
			sem[i] = sem[i + semaptr->sem_nsems];
		for (i = 0; i < seminfo.semmni; i++) {
			if ((sema[i].sem_perm.mode & SEM_ALLOC) &&
			    sema[i].sem_base > semaptr->sem_base)
				sema[i].sem_base -= semaptr->sem_nsems;
		}
		semaptr->sem_perm.mode = 0;
		SEMUNDO_LOCK();
		semundo_clear(semid, -1);
		SEMUNDO_UNLOCK();
		wakeup(semaptr);
		break;

	case IPC_SET:
		if ((error = copyin(arg, &real_arg, sizeof(real_arg))) != 0)
			goto done2;
		if ((error = copyin(real_arg.buf, &sbuf, sizeof(sbuf))) != 0)
			goto done2;
		mtx_lock(sema_mtxp);
		if ((error = semvalid(uap->semid, semaptr)) != 0)
			goto done2;
		if ((error = ipcperm(td, &semaptr->sem_perm, IPC_M)))
			goto done2;
		semaptr->sem_perm.uid = sbuf.sem_perm.uid;
		semaptr->sem_perm.gid = sbuf.sem_perm.gid;
		semaptr->sem_perm.mode = (semaptr->sem_perm.mode & ~0777) |
		    (sbuf.sem_perm.mode & 0777);
		semaptr->sem_ctime = time_second;
		break;

	case IPC_STAT:
		if ((error = copyin(arg, &real_arg, sizeof(real_arg))) != 0)
			goto done2;
		mtx_lock(sema_mtxp);
		if ((error = semvalid(uap->semid, semaptr)) != 0)
			goto done2;
		if ((error = ipcperm(td, &semaptr->sem_perm, IPC_R)))
			goto done2;
		sbuf = *semaptr;
		mtx_unlock(sema_mtxp);
		error = copyout(semaptr, real_arg.buf,
				sizeof(struct semid_ds));
		break;

	case GETNCNT:
		mtx_lock(sema_mtxp);
		if ((error = semvalid(uap->semid, semaptr)) != 0)
			goto done2;
		if ((error = ipcperm(td, &semaptr->sem_perm, IPC_R)))
			goto done2;
		if (semnum < 0 || semnum >= semaptr->sem_nsems) {
			error = EINVAL;
			goto done2;
		}
		rval = semaptr->sem_base[semnum].semncnt;
		break;

	case GETPID:
		mtx_lock(sema_mtxp);
		if ((error = semvalid(uap->semid, semaptr)) != 0)
			goto done2;
		if ((error = ipcperm(td, &semaptr->sem_perm, IPC_R)))
			goto done2;
		if (semnum < 0 || semnum >= semaptr->sem_nsems) {
			error = EINVAL;
			goto done2;
		}
		rval = semaptr->sem_base[semnum].sempid;
		break;

	case GETVAL:
		mtx_lock(sema_mtxp);
		if ((error = semvalid(uap->semid, semaptr)) != 0)
			goto done2;
		if ((error = ipcperm(td, &semaptr->sem_perm, IPC_R)))
			goto done2;
		if (semnum < 0 || semnum >= semaptr->sem_nsems) {
			error = EINVAL;
			goto done2;
		}
		rval = semaptr->sem_base[semnum].semval;
		break;

	case GETALL:
		if ((error = copyin(arg, &real_arg, sizeof(real_arg))) != 0)
			goto done2;
		array = malloc(sizeof(*array) * semaptr->sem_nsems, M_TEMP,
		    M_WAITOK);
		mtx_lock(sema_mtxp);
		if ((error = semvalid(uap->semid, semaptr)) != 0)
			goto done2;
		if ((error = ipcperm(td, &semaptr->sem_perm, IPC_R)))
			goto done2;
		for (i = 0; i < semaptr->sem_nsems; i++)
			array[i] = semaptr->sem_base[i].semval;
		mtx_unlock(sema_mtxp);
		error = copyout(array, real_arg.array,
		    i * sizeof(real_arg.array[0]));
		break;

	case GETZCNT:
		mtx_lock(sema_mtxp);
		if ((error = semvalid(uap->semid, semaptr)) != 0)
			goto done2;
		if ((error = ipcperm(td, &semaptr->sem_perm, IPC_R)))
			goto done2;
		if (semnum < 0 || semnum >= semaptr->sem_nsems) {
			error = EINVAL;
			goto done2;
		}
		rval = semaptr->sem_base[semnum].semzcnt;
		break;

	case SETVAL:
		if ((error = copyin(arg, &real_arg, sizeof(real_arg))) != 0)
			goto done2;
		mtx_lock(sema_mtxp);
		if ((error = semvalid(uap->semid, semaptr)) != 0)
			goto done2;
		if ((error = ipcperm(td, &semaptr->sem_perm, IPC_W)))
			goto done2;
		if (semnum < 0 || semnum >= semaptr->sem_nsems) {
			error = EINVAL;
			goto done2;
		}
		if (real_arg.val < 0 || real_arg.val > seminfo.semvmx) {
			error = ERANGE;
			goto done2;
		}
		semaptr->sem_base[semnum].semval = real_arg.val;
		SEMUNDO_LOCK();
		semundo_clear(semid, semnum);
		SEMUNDO_UNLOCK();
		wakeup(semaptr);
		break;

	case SETALL:
		mtx_lock(sema_mtxp);
raced:
		if ((error = semvalid(uap->semid, semaptr)) != 0)
			goto done2;
		count = semaptr->sem_nsems;
		mtx_unlock(sema_mtxp);
		if ((error = copyin(arg, &real_arg, sizeof(real_arg))) != 0)
			goto done2;
		array = malloc(sizeof(*array) * count, M_TEMP, M_WAITOK);
		copyin(real_arg.array, array, count * sizeof(*array));
		if (error)
			break;
		mtx_lock(sema_mtxp);
		if ((error = semvalid(uap->semid, semaptr)) != 0)
			goto done2;
		/* we could have raced? */
		if (count != semaptr->sem_nsems) {
			free(array, M_TEMP);
			array = NULL;
			goto raced;
		}
		if ((error = ipcperm(td, &semaptr->sem_perm, IPC_W)))
			goto done2;
		for (i = 0; i < semaptr->sem_nsems; i++) {
			usval = array[i];
			if (usval > seminfo.semvmx) {
				error = ERANGE;
				break;
			}
			semaptr->sem_base[i].semval = usval;
		}
		SEMUNDO_LOCK();
		semundo_clear(semid, -1);
		SEMUNDO_UNLOCK();
		wakeup(semaptr);
		break;

	default:
		error = EINVAL;
		break;
	}

	if (error == 0)
		td->td_retval[0] = rval;
done2:
	if (mtx_owned(sema_mtxp))
		mtx_unlock(sema_mtxp);
	if (array != NULL)
		free(array, M_TEMP);
	return(error);
}

#ifndef _SYS_SYSPROTO_H_
struct semget_args {
	key_t	key;
	int	nsems;
	int	semflg;
};
#endif

/*
 * MPSAFE
 */
int
semget(td, uap)
	struct thread *td;
	struct semget_args *uap;
{
	int semid, error = 0;
	int key = uap->key;
	int nsems = uap->nsems;
	int semflg = uap->semflg;
	struct ucred *cred = td->td_ucred;

	DPRINTF(("semget(0x%x, %d, 0%o)\n", key, nsems, semflg));
	if (!jail_sysvipc_allowed && jailed(td->td_ucred))
		return (ENOSYS);

	mtx_lock(&Giant);
	if (key != IPC_PRIVATE) {
		for (semid = 0; semid < seminfo.semmni; semid++) {
			if ((sema[semid].sem_perm.mode & SEM_ALLOC) &&
			    sema[semid].sem_perm.key == key)
				break;
		}
		if (semid < seminfo.semmni) {
			DPRINTF(("found public key\n"));
			if ((error = ipcperm(td, &sema[semid].sem_perm,
			    semflg & 0700))) {
				goto done2;
			}
			if (nsems > 0 && sema[semid].sem_nsems < nsems) {
				DPRINTF(("too small\n"));
				error = EINVAL;
				goto done2;
			}
			if ((semflg & IPC_CREAT) && (semflg & IPC_EXCL)) {
				DPRINTF(("not exclusive\n"));
				error = EEXIST;
				goto done2;
			}
			goto found;
		}
	}

	DPRINTF(("need to allocate the semid_ds\n"));
	if (key == IPC_PRIVATE || (semflg & IPC_CREAT)) {
		if (nsems <= 0 || nsems > seminfo.semmsl) {
			DPRINTF(("nsems out of range (0<%d<=%d)\n", nsems,
			    seminfo.semmsl));
			error = EINVAL;
			goto done2;
		}
		if (nsems > seminfo.semmns - semtot) {
			DPRINTF((
			    "not enough semaphores left (need %d, got %d)\n",
			    nsems, seminfo.semmns - semtot));
			error = ENOSPC;
			goto done2;
		}
		for (semid = 0; semid < seminfo.semmni; semid++) {
			if ((sema[semid].sem_perm.mode & SEM_ALLOC) == 0)
				break;
		}
		if (semid == seminfo.semmni) {
			DPRINTF(("no more semid_ds's available\n"));
			error = ENOSPC;
			goto done2;
		}
		DPRINTF(("semid %d is available\n", semid));
		sema[semid].sem_perm.key = key;
		sema[semid].sem_perm.cuid = cred->cr_uid;
		sema[semid].sem_perm.uid = cred->cr_uid;
		sema[semid].sem_perm.cgid = cred->cr_gid;
		sema[semid].sem_perm.gid = cred->cr_gid;
		sema[semid].sem_perm.mode = (semflg & 0777) | SEM_ALLOC;
		sema[semid].sem_perm.seq =
		    (sema[semid].sem_perm.seq + 1) & 0x7fff;
		sema[semid].sem_nsems = nsems;
		sema[semid].sem_otime = 0;
		sema[semid].sem_ctime = time_second;
		sema[semid].sem_base = &sem[semtot];
		semtot += nsems;
		bzero(sema[semid].sem_base,
		    sizeof(sema[semid].sem_base[0])*nsems);
		DPRINTF(("sembase = 0x%x, next = 0x%x\n", sema[semid].sem_base,
		    &sem[semtot]));
	} else {
		DPRINTF(("didn't find it and wasn't asked to create it\n"));
		error = ENOENT;
		goto done2;
	}

found:
	td->td_retval[0] = IXSEQ_TO_IPCID(semid, sema[semid].sem_perm);
done2:
	mtx_unlock(&Giant);
	return (error);
}

#ifndef _SYS_SYSPROTO_H_
struct semop_args {
	int	semid;
	struct	sembuf *sops;
	u_int	nsops;
};
#endif

/*
 * MPSAFE
 */
int
semop(td, uap)
	struct thread *td;
	struct semop_args *uap;
{
	int semid = uap->semid;
	u_int nsops = uap->nsops;
	struct sembuf *sops;
	struct semid_ds *semaptr;
	struct sembuf *sopptr = 0;
	struct sem *semptr = 0;
	struct sem_undo *suptr;
	struct mtx *sema_mtxp;
	int i, j, error;
	int do_wakeup, do_undos;

	DPRINTF(("call to semop(%d, 0x%x, %u)\n", semid, sops, nsops));

	if (!jail_sysvipc_allowed && jailed(td->td_ucred))
		return (ENOSYS);

	semid = IPCID_TO_IX(semid);	/* Convert back to zero origin */

	if (semid < 0 || semid >= seminfo.semmni)
		return (EINVAL);

	/* Allocate memory for sem_ops */
	if (nsops > seminfo.semopm) {
		DPRINTF(("too many sops (max=%d, nsops=%d)\n", seminfo.semopm,
		    nsops));
		return (E2BIG);
	}
	sops = malloc(nsops * sizeof(sops[0]), M_SEM, M_WAITOK);
	if ((error = copyin(uap->sops, sops, nsops * sizeof(sops[0]))) != 0) {
		DPRINTF(("error = %d from copyin(%08x, %08x, %d)\n", error,
		    uap->sops, sops, nsops * sizeof(sops[0])));
		free(sops, M_SEM);
		return (error);
	}

	semaptr = &sema[semid];
	sema_mtxp = &sema_mtx[semid];
	mtx_lock(sema_mtxp);
	if ((semaptr->sem_perm.mode & SEM_ALLOC) == 0) {
		error = EINVAL;
		goto done2;
	}
	if (semaptr->sem_perm.seq != IPCID_TO_SEQ(uap->semid)) {
		error = EINVAL;
		goto done2;
	}
	/*
	 * Initial pass thru sops to see what permissions are needed.
	 * Also perform any checks that don't need repeating on each
	 * attempt to satisfy the request vector.
	 */
	j = 0;		/* permission needed */
	do_undos = 0;
	for (i = 0; i < nsops; i++) {
		sopptr = &sops[i];
		if (sopptr->sem_num >= semaptr->sem_nsems) {
			error = EFBIG;
			goto done2;
		}
		if (sopptr->sem_flg & SEM_UNDO && sopptr->sem_op != 0)
			do_undos = 1;
		j |= (sopptr->sem_op == 0) ? SEM_R : SEM_A;
	}

	if ((error = ipcperm(td, &semaptr->sem_perm, j))) {
		DPRINTF(("error = %d from ipaccess\n", error));
		goto done2;
	}

	/*
	 * Loop trying to satisfy the vector of requests.
	 * If we reach a point where we must wait, any requests already
	 * performed are rolled back and we go to sleep until some other
	 * process wakes us up.  At this point, we start all over again.
	 *
	 * This ensures that from the perspective of other tasks, a set
	 * of requests is atomic (never partially satisfied).
	 */
	for (;;) {
		do_wakeup = 0;
		error = 0;	/* error return if necessary */

		for (i = 0; i < nsops; i++) {
			sopptr = &sops[i];
			semptr = &semaptr->sem_base[sopptr->sem_num];

			DPRINTF((
			    "semop:  semaptr=%x, sem_base=%x, "
			    "semptr=%x, sem[%d]=%d : op=%d, flag=%s\n",
			    semaptr, semaptr->sem_base, semptr,
			    sopptr->sem_num, semptr->semval, sopptr->sem_op,
			    (sopptr->sem_flg & IPC_NOWAIT) ?
			    "nowait" : "wait"));

			if (sopptr->sem_op < 0) {
				if (semptr->semval + sopptr->sem_op < 0) {
					DPRINTF(("semop:  can't do it now\n"));
					break;
				} else {
					semptr->semval += sopptr->sem_op;
					if (semptr->semval == 0 &&
					    semptr->semzcnt > 0)
						do_wakeup = 1;
				}
			} else if (sopptr->sem_op == 0) {
				if (semptr->semval != 0) {
					DPRINTF(("semop:  not zero now\n"));
					break;
				}
			} else if (semptr->semval + sopptr->sem_op >
			    seminfo.semvmx) {
				error = ERANGE;
				break;
			} else {
				if (semptr->semncnt > 0)
					do_wakeup = 1;
				semptr->semval += sopptr->sem_op;
			}
		}

		/*
		 * Did we get through the entire vector?
		 */
		if (i >= nsops)
			goto done;

		/*
		 * No ... rollback anything that we've already done
		 */
		DPRINTF(("semop:  rollback 0 through %d\n", i-1));
		for (j = 0; j < i; j++)
			semaptr->sem_base[sops[j].sem_num].semval -=
			    sops[j].sem_op;

		/* If we detected an error, return it */
		if (error != 0)
			goto done2;

		/*
		 * If the request that we couldn't satisfy has the
		 * NOWAIT flag set then return with EAGAIN.
		 */
		if (sopptr->sem_flg & IPC_NOWAIT) {
			error = EAGAIN;
			goto done2;
		}

		if (sopptr->sem_op == 0)
			semptr->semzcnt++;
		else
			semptr->semncnt++;

		DPRINTF(("semop:  good night!\n"));
		error = msleep(semaptr, sema_mtxp, (PZERO - 4) | PCATCH,
		    "semwait", 0);
		DPRINTF(("semop:  good morning (error=%d)!\n", error));

		if (error != 0) {
			error = EINTR;
			goto done2;
		}
		DPRINTF(("semop:  good morning!\n"));

		/*
		 * Make sure that the semaphore still exists
		 */
		if ((semaptr->sem_perm.mode & SEM_ALLOC) == 0 ||
		    semaptr->sem_perm.seq != IPCID_TO_SEQ(uap->semid)) {
			error = EIDRM;
			goto done2;
		}

		/*
		 * The semaphore is still alive.  Readjust the count of
		 * waiting processes.
		 */
		if (sopptr->sem_op == 0)
			semptr->semzcnt--;
		else
			semptr->semncnt--;
	}

done:
	/*
	 * Process any SEM_UNDO requests.
	 */
	if (do_undos) {
		SEMUNDO_LOCK();
		suptr = NULL;
		for (i = 0; i < nsops; i++) {
			/*
			 * We only need to deal with SEM_UNDO's for non-zero
			 * op's.
			 */
			int adjval;

			if ((sops[i].sem_flg & SEM_UNDO) == 0)
				continue;
			adjval = sops[i].sem_op;
			if (adjval == 0)
				continue;
			error = semundo_adjust(td, &suptr, semid,
			    sops[i].sem_num, -adjval);
			if (error == 0)
				continue;

			/*
			 * Oh-Oh!  We ran out of either sem_undo's or undo's.
			 * Rollback the adjustments to this point and then
			 * rollback the semaphore ups and down so we can return
			 * with an error with all structures restored.  We
			 * rollback the undo's in the exact reverse order that
			 * we applied them.  This guarantees that we won't run
			 * out of space as we roll things back out.
			 */
			for (j = i - 1; j >= 0; j--) {
				if ((sops[j].sem_flg & SEM_UNDO) == 0)
					continue;
				adjval = sops[j].sem_op;
				if (adjval == 0)
					continue;
				if (semundo_adjust(td, &suptr, semid,
				    sops[j].sem_num, adjval) != 0)
					panic("semop - can't undo undos");
			}

			for (j = 0; j < nsops; j++)
				semaptr->sem_base[sops[j].sem_num].semval -=
				    sops[j].sem_op;

			DPRINTF(("error = %d from semundo_adjust\n", error));
			SEMUNDO_UNLOCK();
			goto done2;
		} /* loop through the sops */
		SEMUNDO_UNLOCK();
	} /* if (do_undos) */

	/* We're definitely done - set the sempid's and time */
	for (i = 0; i < nsops; i++) {
		sopptr = &sops[i];
		semptr = &semaptr->sem_base[sopptr->sem_num];
		semptr->sempid = td->td_proc->p_pid;
	}
	semaptr->sem_otime = time_second;

	/*
	 * Do a wakeup if any semaphore was up'd whilst something was
	 * sleeping on it.
	 */
	if (do_wakeup) {
		DPRINTF(("semop:  doing wakeup\n"));
		wakeup(semaptr);
		DPRINTF(("semop:  back from wakeup\n"));
	}
	DPRINTF(("semop:  done\n"));
	td->td_retval[0] = 0;
done2:
	mtx_unlock(sema_mtxp);
	free(sops, M_SEM);
	return (error);
}

/*
 * Go through the undo structures for this process and apply the adjustments to
 * semaphores.
 */
static void
semexit_myhook(p)
	struct proc *p;
{
	struct sem_undo *suptr;
	struct sem_undo **supptr;

	/*
	 * Go through the chain of undo vectors looking for one
	 * associated with this process.
	 */
	SEMUNDO_LOCK();
	SLIST_FOREACH_PREVPTR(suptr, supptr, &semu_list, un_next) {
		if (suptr->un_proc == p)
			break;
	}
	SEMUNDO_UNLOCK();

	if (suptr == NULL)
		return;

	DPRINTF(("proc @%08x has undo structure with %d entries\n", p,
	    suptr->un_cnt));

	/*
	 * If there are any active undo elements then process them.
	 */
	if (suptr->un_cnt > 0) {
		int ix;

		for (ix = 0; ix < suptr->un_cnt; ix++) {
			int semid = suptr->un_ent[ix].un_id;
			int semnum = suptr->un_ent[ix].un_num;
			int adjval = suptr->un_ent[ix].un_adjval;
			struct semid_ds *semaptr;
			struct mtx *sema_mtxp;

			semaptr = &sema[semid];
			sema_mtxp = &sema_mtx[semid];
			mtx_lock(sema_mtxp);
			SEMUNDO_LOCK();
			if ((semaptr->sem_perm.mode & SEM_ALLOC) == 0)
				panic("semexit - semid not allocated");
			if (semnum >= semaptr->sem_nsems)
				panic("semexit - semnum out of range");

			DPRINTF((
			    "semexit:  %08x id=%d num=%d(adj=%d) ; sem=%d\n",
			    suptr->un_proc, suptr->un_ent[ix].un_id,
			    suptr->un_ent[ix].un_num,
			    suptr->un_ent[ix].un_adjval,
			    semaptr->sem_base[semnum].semval));

			if (adjval < 0) {
				if (semaptr->sem_base[semnum].semval < -adjval)
					semaptr->sem_base[semnum].semval = 0;
				else
					semaptr->sem_base[semnum].semval +=
					    adjval;
			} else
				semaptr->sem_base[semnum].semval += adjval;

			wakeup(semaptr);
			DPRINTF(("semexit:  back from wakeup\n"));
			mtx_unlock(sema_mtxp);
			SEMUNDO_UNLOCK();
		}
	}

	/*
	 * Deallocate the undo vector.
	 */
	DPRINTF(("removing vector\n"));
	suptr->un_proc = NULL;
	*supptr = SLIST_NEXT(suptr, un_next);
}

static int
sysctl_sema(SYSCTL_HANDLER_ARGS)
{

	return (SYSCTL_OUT(req, sema,
	    sizeof(struct semid_ds) * seminfo.semmni));
}
