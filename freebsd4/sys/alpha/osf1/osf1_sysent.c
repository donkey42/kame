/*
 * System call switch table.
 *
 * DO NOT EDIT-- this file is automatically generated.
 * $FreeBSD: src/sys/alpha/osf1/osf1_sysent.c,v 1.1 1999/12/14 22:37:09 gallatin Exp $
 * created from;	FreeBSD
 */

#include "opt_compat.h"
#include <sys/param.h>
#include <sys/sysent.h>
#include <sys/sysproto.h>
#include <alpha/osf1/osf1.h>
#include <alpha/osf1/osf1_signal.h>
#include <alpha/osf1/osf1_proto.h>

/* The casts are bogus but will do for now. */
struct sysent osf1_sysent[] = {
	{ 0, (sy_call_t *)nosys },			/* 0 = nosys */
	{ 1, (sy_call_t *)exit },			/* 1 = exit */
	{ 0, (sy_call_t *)fork },			/* 2 = fork */
	{ 3, (sy_call_t *)read },			/* 3 = read */
	{ 3, (sy_call_t *)write },			/* 4 = write */
	{ 0, (sy_call_t *)nosys },			/* 5 = old open */
	{ 1, (sy_call_t *)close },			/* 6 = close */
	{ 4, (sy_call_t *)osf1_wait4 },			/* 7 = osf1_wait4 */
	{ 0, (sy_call_t *)nosys },			/* 8 = old creat */
	{ 2, (sy_call_t *)link },			/* 9 = link */
	{ 1, (sy_call_t *)unlink },			/* 10 = unlink */
	{ 0, (sy_call_t *)nosys },			/* 11 = execv */
	{ 1, (sy_call_t *)chdir },			/* 12 = chdir */
	{ 1, (sy_call_t *)fchdir },			/* 13 = fchdir */
	{ 3, (sy_call_t *)osf1_mknod },			/* 14 = osf1_mknod */
	{ 2, (sy_call_t *)chmod },			/* 15 = chmod */
	{ 3, (sy_call_t *)chown },			/* 16 = chown */
	{ 1, (sy_call_t *)obreak },			/* 17 = obreak */
	{ 3, (sy_call_t *)osf1_getfsstat },		/* 18 = osf1_getfsstat */
	{ 4, (sy_call_t *)osf1_lseek },			/* 19 = osf1_lseek */
	{ 0, (sy_call_t *)getpid },			/* 20 = getpid */
	{ 4, (sy_call_t *)osf1_mount },			/* 21 = osf1_mount */
	{ 2, (sy_call_t *)osf1_unmount },		/* 22 = osf1_unmount */
	{ 1, (sy_call_t *)osf1_setuid },		/* 23 = osf1_setuid */
	{ 0, (sy_call_t *)getuid },			/* 24 = getuid */
	{ 0, (sy_call_t *)nosys },			/* 25 = exec_with_loader */
	{ 0, (sy_call_t *)nosys },			/* 26 = ptrace */
	{ 0, (sy_call_t *)nosys },			/* 27 = recvmsg */
	{ 0, (sy_call_t *)nosys },			/* 28 = sendmsg */
	{ 6, (sy_call_t *)recvfrom },			/* 29 = recvfrom */
	{ 3, (sy_call_t *)accept },			/* 30 = accept */
	{ 3, (sy_call_t *)getpeername },		/* 31 = getpeername */
	{ 3, (sy_call_t *)getsockname },		/* 32 = getsockname */
	{ 2, (sy_call_t *)osf1_access },		/* 33 = osf1_access */
	{ 0, (sy_call_t *)nosys },			/* 34 = chflags */
	{ 0, (sy_call_t *)nosys },			/* 35 = fchflags */
	{ 0, (sy_call_t *)sync },			/* 36 = sync */
	{ 2, (sy_call_t *)osf1_kill },			/* 37 = osf1_kill */
	{ 0, (sy_call_t *)nosys },			/* 38 = old stat */
	{ 2, (sy_call_t *)setpgid },			/* 39 = setpgid */
	{ 0, (sy_call_t *)nosys },			/* 40 = old lstat */
	{ 1, (sy_call_t *)dup },			/* 41 = dup */
	{ 0, (sy_call_t *)pipe },			/* 42 = pipe */
	{ 4, (sy_call_t *)osf1_set_program_attributes },		/* 43 = osf1_set_program_attributes */
	{ 0, (sy_call_t *)nosys },			/* 44 = profil */
	{ 3, (sy_call_t *)osf1_open },			/* 45 = osf1_open */
	{ 0, (sy_call_t *)nosys },			/* 46 = obsolete sigaction */
	{ 0, (sy_call_t *)getgid },			/* 47 = getgid */
	{ 2, (sy_call_t *)osf1_sigprocmask },		/* 48 = osf1_sigprocmask */
	{ 2, (sy_call_t *)getlogin },			/* 49 = getlogin */
	{ 1, (sy_call_t *)setlogin },			/* 50 = setlogin */
	{ 1, (sy_call_t *)acct },			/* 51 = acct */
	{ 2, (sy_call_t *)osf1_sigpending },		/* 52 = osf1_sigpending */
	{ 4, (sy_call_t *)osf1_classcntl },		/* 53 = osf1_classcntl */
	{ 3, (sy_call_t *)osf1_ioctl },			/* 54 = osf1_ioctl */
	{ 1, (sy_call_t *)osf1_reboot },		/* 55 = osf1_reboot */
	{ 1, (sy_call_t *)revoke },			/* 56 = revoke */
	{ 2, (sy_call_t *)symlink },			/* 57 = symlink */
	{ 3, (sy_call_t *)readlink },			/* 58 = readlink */
	{ 3, (sy_call_t *)osf1_execve },		/* 59 = osf1_execve */
	{ 1, (sy_call_t *)umask },			/* 60 = umask */
	{ 1, (sy_call_t *)chroot },			/* 61 = chroot */
	{ 0, (sy_call_t *)nosys },			/* 62 = old fstat */
	{ 0, (sy_call_t *)getpgrp },			/* 63 = getpgrp */
	{ 0, (sy_call_t *)ogetpagesize },		/* 64 = ogetpagesize */
	{ 0, (sy_call_t *)nosys },			/* 65 = mremap */
	{ 0, (sy_call_t *)vfork },			/* 66 = vfork */
	{ 2, (sy_call_t *)osf1_stat },			/* 67 = osf1_stat */
	{ 2, (sy_call_t *)osf1_lstat },			/* 68 = osf1_lstat */
	{ 0, (sy_call_t *)nosys },			/* 69 = sbrk */
	{ 0, (sy_call_t *)nosys },			/* 70 = sstk */
	{ 7, (sy_call_t *)osf1_mmap },			/* 71 = osf1_mmap */
	{ 0, (sy_call_t *)nosys },			/* 72 = ovadvise */
	{ 2, (sy_call_t *)munmap },			/* 73 = munmap */
	{ 3, (sy_call_t *)mprotect },			/* 74 = mprotect */
	{ 0, (sy_call_t *)osf1_madvise },		/* 75 = osf1_madvise */
	{ 0, (sy_call_t *)nosys },			/* 76 = old vhangup */
	{ 0, (sy_call_t *)nosys },			/* 77 = kmodcall */
	{ 0, (sy_call_t *)nosys },			/* 78 = mincore */
	{ 2, (sy_call_t *)getgroups },			/* 79 = getgroups */
	{ 2, (sy_call_t *)setgroups },			/* 80 = setgroups */
	{ 0, (sy_call_t *)nosys },			/* 81 = old getpgrp */
	{ 2, (sy_call_t *)osf1_setpgrp },		/* 82 = osf1_setpgrp */
	{ 3, (sy_call_t *)osf1_setitimer },		/* 83 = osf1_setitimer */
	{ 0, (sy_call_t *)nosys },			/* 84 = old wait */
	{ 5, (sy_call_t *)osf1_table },			/* 85 = osf1_table */
	{ 2, (sy_call_t *)osf1_getitimer },		/* 86 = osf1_getitimer */
	{ 2, (sy_call_t *)ogethostname },		/* 87 = ogethostname */
	{ 2, (sy_call_t *)osethostname },		/* 88 = osethostname */
	{ 0, (sy_call_t *)getdtablesize },		/* 89 = getdtablesize */
	{ 2, (sy_call_t *)dup2 },			/* 90 = dup2 */
	{ 2, (sy_call_t *)osf1_fstat },			/* 91 = osf1_fstat */
	{ 3, (sy_call_t *)osf1_fcntl },			/* 92 = osf1_fcntl */
	{ 5, (sy_call_t *)osf1_select },		/* 93 = osf1_select */
	{ 3, (sy_call_t *)poll },			/* 94 = poll */
	{ 1, (sy_call_t *)fsync },			/* 95 = fsync */
	{ 3, (sy_call_t *)setpriority },		/* 96 = setpriority */
	{ 3, (sy_call_t *)osf1_socket },		/* 97 = osf1_socket */
	{ 3, (sy_call_t *)connect },			/* 98 = connect */
	{ 3, (sy_call_t *)oaccept },			/* 99 = oaccept */
	{ 2, (sy_call_t *)getpriority },		/* 100 = getpriority */
	{ 4, (sy_call_t *)osend },			/* 101 = osend */
	{ 4, (sy_call_t *)orecv },			/* 102 = orecv */
	{ 1, (sy_call_t *)osf1_sigreturn },		/* 103 = osf1_sigreturn */
	{ 3, (sy_call_t *)bind },			/* 104 = bind */
	{ 5, (sy_call_t *)setsockopt },			/* 105 = setsockopt */
	{ 2, (sy_call_t *)listen },			/* 106 = listen */
	{ 0, (sy_call_t *)nosys },			/* 107 = plock */
	{ 0, (sy_call_t *)nosys },			/* 108 = old sigvec */
	{ 0, (sy_call_t *)nosys },			/* 109 = old sigblock */
	{ 0, (sy_call_t *)nosys },			/* 110 = old sigsetmask */
	{ 1, (sy_call_t *)osf1_sigsuspend },		/* 111 = osf1_sigsuspend */
	{ 2, (sy_call_t *)osf1_osigstack },		/* 112 = osf1_osigstack */
	{ 0, (sy_call_t *)nosys },			/* 113 = old recvmsg */
	{ 0, (sy_call_t *)nosys },			/* 114 = old sendmsg */
	{ 0, (sy_call_t *)nosys },			/* 115 = vtrace */
	{ 2, (sy_call_t *)osf1_gettimeofday },		/* 116 = osf1_gettimeofday */
	{ 2, (sy_call_t *)osf1_getrusage },		/* 117 = osf1_getrusage */
	{ 5, (sy_call_t *)getsockopt },			/* 118 = getsockopt */
	{ 0, (sy_call_t *)nosys },			/* 119 =  */
	{ 3, (sy_call_t *)osf1_readv },			/* 120 = osf1_readv */
	{ 3, (sy_call_t *)osf1_writev },		/* 121 = osf1_writev */
	{ 2, (sy_call_t *)settimeofday },		/* 122 = settimeofday */
	{ 3, (sy_call_t *)fchown },			/* 123 = fchown */
	{ 2, (sy_call_t *)fchmod },			/* 124 = fchmod */
	{ 6, (sy_call_t *)orecvfrom },			/* 125 = orecvfrom */
	{ 2, (sy_call_t *)setreuid },			/* 126 = setreuid */
	{ 2, (sy_call_t *)setregid },			/* 127 = setregid */
	{ 2, (sy_call_t *)rename },			/* 128 = rename */
	{ 3, (sy_call_t *)osf1_truncate },		/* 129 = osf1_truncate */
	{ 3, (sy_call_t *)osf1_ftruncate },		/* 130 = osf1_ftruncate */
	{ 2, (sy_call_t *)flock },			/* 131 = flock */
	{ 1, (sy_call_t *)osf1_setgid },		/* 132 = osf1_setgid */
	{ 6, (sy_call_t *)osf1_sendto },		/* 133 = osf1_sendto */
	{ 2, (sy_call_t *)shutdown },			/* 134 = shutdown */
	{ 0, (sy_call_t *)nosys },			/* 135 = socketpair */
	{ 2, (sy_call_t *)mkdir },			/* 136 = mkdir */
	{ 1, (sy_call_t *)rmdir },			/* 137 = rmdir */
	{ 2, (sy_call_t *)utimes },			/* 138 = utimes */
	{ 0, (sy_call_t *)nosys },			/* 139 = obsolete 4.2 sigreturn */
	{ 0, (sy_call_t *)nosys },			/* 140 = adjtime */
	{ 3, (sy_call_t *)ogetpeername },		/* 141 = ogetpeername */
	{ 0, (sy_call_t *)ogethostid },			/* 142 = ogethostid */
	{ 1, (sy_call_t *)osethostid },			/* 143 = osethostid */
	{ 2, (sy_call_t *)osf1_getrlimit },		/* 144 = osf1_getrlimit */
	{ 2, (sy_call_t *)osf1_setrlimit },		/* 145 = osf1_setrlimit */
	{ 0, (sy_call_t *)nosys },			/* 146 = old killpg */
	{ 0, (sy_call_t *)setsid },			/* 147 = setsid */
	{ 0, (sy_call_t *)nosys },			/* 148 = quotactl */
	{ 0, (sy_call_t *)oquota },			/* 149 = oquota */
	{ 3, (sy_call_t *)ogetsockname },		/* 150 = ogetsockname */
	{ 0, (sy_call_t *)nosys },			/* 151 =  */
	{ 0, (sy_call_t *)nosys },			/* 152 =  */
	{ 0, (sy_call_t *)nosys },			/* 153 =  */
	{ 0, (sy_call_t *)nosys },			/* 154 =  */
	{ 0, (sy_call_t *)nosys },			/* 155 =  */
	{ 4, (sy_call_t *)osf1_sigaction },		/* 156 = osf1_sigaction */
	{ 0, (sy_call_t *)nosys },			/* 157 =  */
	{ 0, (sy_call_t *)nosys },			/* 158 = nfssvc */
	{ 4, (sy_call_t *)ogetdirentries },		/* 159 = ogetdirentries */
	{ 3, (sy_call_t *)osf1_statfs },		/* 160 = osf1_statfs */
	{ 3, (sy_call_t *)osf1_fstatfs },		/* 161 = osf1_fstatfs */
	{ 0, (sy_call_t *)nosys },			/* 162 =  */
	{ 0, (sy_call_t *)nosys },			/* 163 = async_daemon */
	{ 0, (sy_call_t *)nosys },			/* 164 = getfh */
	{ 2, (sy_call_t *)getdomainname },		/* 165 = getdomainname */
	{ 2, (sy_call_t *)setdomainname },		/* 166 = setdomainname */
	{ 0, (sy_call_t *)nosys },			/* 167 =  */
	{ 0, (sy_call_t *)nosys },			/* 168 =  */
	{ 0, (sy_call_t *)nosys },			/* 169 = exportfs */
	{ 0, (sy_call_t *)nosys },			/* 170 =  */
	{ 0, (sy_call_t *)nosys },			/* 171 =  */
	{ 0, (sy_call_t *)nosys },			/* 172 = alt msgctl */
	{ 0, (sy_call_t *)nosys },			/* 173 = alt msgget */
	{ 0, (sy_call_t *)nosys },			/* 174 = alt msgrcv */
	{ 0, (sy_call_t *)nosys },			/* 175 = alt msgsnd */
	{ 0, (sy_call_t *)nosys },			/* 176 = alt semctl */
	{ 0, (sy_call_t *)nosys },			/* 177 = alt semget */
	{ 0, (sy_call_t *)nosys },			/* 178 = alt semop */
	{ 0, (sy_call_t *)nosys },			/* 179 = alt uname */
	{ 0, (sy_call_t *)nosys },			/* 180 =  */
	{ 0, (sy_call_t *)nosys },			/* 181 = alt plock */
	{ 0, (sy_call_t *)nosys },			/* 182 = lockf */
	{ 0, (sy_call_t *)nosys },			/* 183 =  */
	{ 0, (sy_call_t *)nosys },			/* 184 = getmnt */
	{ 0, (sy_call_t *)nosys },			/* 185 =  */
	{ 0, (sy_call_t *)nosys },			/* 186 = unmount */
	{ 0, (sy_call_t *)nosys },			/* 187 = alt sigpending */
	{ 0, (sy_call_t *)nosys },			/* 188 = alt setsid */
	{ 0, (sy_call_t *)nosys },			/* 189 =  */
	{ 0, (sy_call_t *)nosys },			/* 190 =  */
	{ 0, (sy_call_t *)nosys },			/* 191 =  */
	{ 0, (sy_call_t *)nosys },			/* 192 =  */
	{ 0, (sy_call_t *)nosys },			/* 193 =  */
	{ 0, (sy_call_t *)nosys },			/* 194 =  */
	{ 0, (sy_call_t *)nosys },			/* 195 =  */
	{ 0, (sy_call_t *)nosys },			/* 196 =  */
	{ 0, (sy_call_t *)nosys },			/* 197 =  */
	{ 0, (sy_call_t *)nosys },			/* 198 =  */
	{ 0, (sy_call_t *)nosys },			/* 199 = swapon */
	{ 3, (sy_call_t *)msgctl },			/* 200 = msgctl */
	{ 2, (sy_call_t *)msgget },			/* 201 = msgget */
	{ 5, (sy_call_t *)msgrcv },			/* 202 = msgrcv */
	{ 4, (sy_call_t *)msgsnd },			/* 203 = msgsnd */
	{ 4, (sy_call_t *)__semctl },			/* 204 = __semctl */
	{ 3, (sy_call_t *)semget },			/* 205 = semget */
	{ 3, (sy_call_t *)semop },			/* 206 = semop */
	{ 1, (sy_call_t *)uname },			/* 207 = uname */
	{ 3, (sy_call_t *)lchown },			/* 208 = lchown */
	{ 3, (sy_call_t *)shmat },			/* 209 = shmat */
	{ 3, (sy_call_t *)shmctl },			/* 210 = shmctl */
	{ 1, (sy_call_t *)shmdt },			/* 211 = shmdt */
	{ 3, (sy_call_t *)shmget },			/* 212 = shmget */
	{ 0, (sy_call_t *)nosys },			/* 213 = mvalid */
	{ 0, (sy_call_t *)nosys },			/* 214 = getaddressconf */
	{ 0, (sy_call_t *)nosys },			/* 215 = msleep */
	{ 0, (sy_call_t *)nosys },			/* 216 = mwakeup */
	{ 3, (sy_call_t *)osf1_msync },			/* 217 = osf1_msync */
	{ 2, (sy_call_t *)osf1_signal },		/* 218 = osf1_signal */
	{ 0, (sy_call_t *)nosys },			/* 219 = utc gettime */
	{ 0, (sy_call_t *)nosys },			/* 220 = utc adjtime */
	{ 0, (sy_call_t *)nosys },			/* 221 =  */
	{ 0, (sy_call_t *)nosys },			/* 222 = security */
	{ 0, (sy_call_t *)nosys },			/* 223 = kloadcall */
	{ 0, (sy_call_t *)nosys },			/* 224 =  */
	{ 0, (sy_call_t *)nosys },			/* 225 =  */
	{ 0, (sy_call_t *)nosys },			/* 226 =  */
	{ 0, (sy_call_t *)nosys },			/* 227 =  */
	{ 0, (sy_call_t *)nosys },			/* 228 =  */
	{ 0, (sy_call_t *)nosys },			/* 229 =  */
	{ 0, (sy_call_t *)nosys },			/* 230 =  */
	{ 0, (sy_call_t *)nosys },			/* 231 =  */
	{ 0, (sy_call_t *)nosys },			/* 232 =  */
	{ 1, (sy_call_t *)getpgid },			/* 233 = getpgid */
	{ 1, (sy_call_t *)getsid },			/* 234 = getsid */
	{ 2, (sy_call_t *)osf1_sigaltstack },		/* 235 = osf1_sigaltstack */
	{ 0, (sy_call_t *)nosys },			/* 236 = waitid */
	{ 0, (sy_call_t *)nosys },			/* 237 = priocntlset */
	{ 0, (sy_call_t *)nosys },			/* 238 = sigsendset */
	{ 0, (sy_call_t *)nosys },			/* 239 =  */
	{ 0, (sy_call_t *)nosys },			/* 240 = msfs_syscall */
	{ 3, (sy_call_t *)osf1_sysinfo },		/* 241 = osf1_sysinfo */
	{ 0, (sy_call_t *)nosys },			/* 242 = uadmin */
	{ 0, (sy_call_t *)nosys },			/* 243 = fuser */
	{ 0, (sy_call_t *)osf1_proplist_syscall },		/* 244 = osf1_proplist_syscall */
	{ 1, (sy_call_t *)osf1_ntpadjtime },		/* 245 = osf1_ntpadjtime */
	{ 1, (sy_call_t *)osf1_ntpgettime },		/* 246 = osf1_ntpgettime */
	{ 2, (sy_call_t *)osf1_pathconf },		/* 247 = osf1_pathconf */
	{ 2, (sy_call_t *)osf1_fpathconf },		/* 248 = osf1_fpathconf */
	{ 0, (sy_call_t *)nosys },			/* 249 =  */
	{ 2, (sy_call_t *)osf1_uswitch },		/* 250 = osf1_uswitch */
	{ 2, (sy_call_t *)osf1_usleep_thread },		/* 251 = osf1_usleep_thread */
	{ 0, (sy_call_t *)nosys },			/* 252 = audcntl */
	{ 0, (sy_call_t *)nosys },			/* 253 = audgen */
	{ 0, (sy_call_t *)nosys },			/* 254 = sysfs */
	{ 0, (sy_call_t *)nosys },			/* 255 =  */
	{ 5, (sy_call_t *)osf1_getsysinfo },		/* 256 = osf1_getsysinfo */
	{ 5, (sy_call_t *)osf1_setsysinfo },		/* 257 = osf1_setsysinfo */
	{ 0, (sy_call_t *)nosys },			/* 258 = afs_syscall */
	{ 0, (sy_call_t *)nosys },			/* 259 = swapctl */
	{ 0, (sy_call_t *)nosys },			/* 260 = memcntl */
	{ 0, (sy_call_t *)nosys },			/* 261 =  */
	{ 0, (sy_call_t *)nosys },			/* 262 =  */
	{ 0, (sy_call_t *)nosys },			/* 263 =  */
	{ 0, (sy_call_t *)nosys },			/* 264 =  */
	{ 0, (sy_call_t *)nosys },			/* 265 =  */
	{ 0, (sy_call_t *)nosys },			/* 266 =  */
	{ 0, (sy_call_t *)nosys },			/* 267 =  */
	{ 0, (sy_call_t *)nosys },			/* 268 =  */
	{ 0, (sy_call_t *)nosys },			/* 269 =  */
	{ 0, (sy_call_t *)nosys },			/* 270 =  */
	{ 0, (sy_call_t *)nosys },			/* 271 =  */
	{ 0, (sy_call_t *)nosys },			/* 272 =  */
	{ 0, (sy_call_t *)nosys },			/* 273 =  */
	{ 0, (sy_call_t *)nosys },			/* 274 =  */
	{ 0, (sy_call_t *)nosys },			/* 275 =  */
	{ 0, (sy_call_t *)nosys },			/* 276 =  */
	{ 0, (sy_call_t *)nosys },			/* 277 =  */
	{ 0, (sy_call_t *)nosys },			/* 278 =  */
	{ 0, (sy_call_t *)nosys },			/* 279 =  */
	{ 0, (sy_call_t *)nosys },			/* 280 =  */
	{ 0, (sy_call_t *)nosys },			/* 281 =  */
	{ 0, (sy_call_t *)nosys },			/* 282 =  */
	{ 0, (sy_call_t *)nosys },			/* 283 =  */
	{ 0, (sy_call_t *)nosys },			/* 284 =  */
	{ 0, (sy_call_t *)nosys },			/* 285 =  */
	{ 0, (sy_call_t *)nosys },			/* 286 =  */
	{ 0, (sy_call_t *)nosys },			/* 287 =  */
	{ 0, (sy_call_t *)nosys },			/* 288 =  */
	{ 0, (sy_call_t *)nosys },			/* 289 =  */
	{ 0, (sy_call_t *)nosys },			/* 290 =  */
	{ 0, (sy_call_t *)nosys },			/* 291 =  */
	{ 0, (sy_call_t *)nosys },			/* 292 =  */
	{ 0, (sy_call_t *)nosys },			/* 293 =  */
	{ 0, (sy_call_t *)nosys },			/* 294 =  */
	{ 0, (sy_call_t *)nosys },			/* 295 =  */
	{ 0, (sy_call_t *)nosys },			/* 296 =  */
	{ 0, (sy_call_t *)nosys },			/* 297 =  */
	{ 0, (sy_call_t *)nosys },			/* 298 =  */
	{ 0, (sy_call_t *)nosys },			/* 299 =  */
	{ 0, (sy_call_t *)nosys },			/* 300 =  */
};
