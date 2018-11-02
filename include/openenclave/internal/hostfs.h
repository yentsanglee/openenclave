// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _OE_HOSTSYSCALL_H
#define _OE_HOSTSYSCALL_H

#define OE_SYSCALL_read 0
#define OE_SYSCALL_write 1
#define OE_SYSCALL_open 2
#define OE_SYSCALL_close 3
#define OE_SYSCALL_stat 4
#define OE_SYSCALL_fstat 5
#define OE_SYSCALL_lstat 6
#define OE_SYSCALL_poll 7
#define OE_SYSCALL_lseek 8
#define OE_SYSCALL_mmap 9
#define OE_SYSCALL_mprotect 10
#define OE_SYSCALL_munmap 11
#define OE_SYSCALL_brk 12
#define OE_SYSCALL_rt_sigaction 13
#define OE_SYSCALL_rt_sigprocmask 14
#define OE_SYSCALL_rt_sigreturn 15
#define OE_SYSCALL_ioctl 16
#define OE_SYSCALL_pread64 17
#define OE_SYSCALL_pwrite64 18
#define OE_SYSCALL_readv 19
#define OE_SYSCALL_writev 20
#define OE_SYSCALL_access 21
#define OE_SYSCALL_pipe 22
#define OE_SYSCALL_select 23
#define OE_SYSCALL_sched_yield 24
#define OE_SYSCALL_mremap 25
#define OE_SYSCALL_msync 26
#define OE_SYSCALL_mincore 27
#define OE_SYSCALL_madvise 28
#define OE_SYSCALL_shmget 29
#define OE_SYSCALL_shmat 30
#define OE_SYSCALL_shmctl 31
#define OE_SYSCALL_dup 32
#define OE_SYSCALL_dup2 33
#define OE_SYSCALL_pause 34
#define OE_SYSCALL_nanosleep 35
#define OE_SYSCALL_getitimer 36
#define OE_SYSCALL_alarm 37
#define OE_SYSCALL_setitimer 38
#define OE_SYSCALL_getpid 39
#define OE_SYSCALL_sendfile 40
#define OE_SYSCALL_socket 41
#define OE_SYSCALL_connect 42
#define OE_SYSCALL_accept 43
#define OE_SYSCALL_sendto 44
#define OE_SYSCALL_recvfrom 45
#define OE_SYSCALL_sendmsg 46
#define OE_SYSCALL_recvmsg 47
#define OE_SYSCALL_shutdown 48
#define OE_SYSCALL_bind 49
#define OE_SYSCALL_listen 50
#define OE_SYSCALL_getsockname 51
#define OE_SYSCALL_getpeername 52
#define OE_SYSCALL_socketpair 53
#define OE_SYSCALL_setsockopt 54
#define OE_SYSCALL_getsockopt 55
#define OE_SYSCALL_clone 56
#define OE_SYSCALL_fork 57
#define OE_SYSCALL_vfork 58
#define OE_SYSCALL_execve 59
#define OE_SYSCALL_exit 60
#define OE_SYSCALL_wait4 61
#define OE_SYSCALL_kill 62
#define OE_SYSCALL_uname 63
#define OE_SYSCALL_semget 64
#define OE_SYSCALL_semop 65
#define OE_SYSCALL_semctl 66
#define OE_SYSCALL_shmdt 67
#define OE_SYSCALL_msgget 68
#define OE_SYSCALL_msgsnd 69
#define OE_SYSCALL_msgrcv 70
#define OE_SYSCALL_msgctl 71
#define OE_SYSCALL_fcntl 72
#define OE_SYSCALL_flock 73
#define OE_SYSCALL_fsync 74
#define OE_SYSCALL_fdatasync 75
#define OE_SYSCALL_truncate 76
#define OE_SYSCALL_ftruncate 77
#define OE_SYSCALL_getdents 78
#define OE_SYSCALL_getcwd 79
#define OE_SYSCALL_chdir 80
#define OE_SYSCALL_fchdir 81
#define OE_SYSCALL_rename 82
#define OE_SYSCALL_mkdir 83
#define OE_SYSCALL_rmdir 84
#define OE_SYSCALL_creat 85
#define OE_SYSCALL_link 86
#define OE_SYSCALL_unlink 87
#define OE_SYSCALL_symlink 88
#define OE_SYSCALL_readlink 89
#define OE_SYSCALL_chmod 90
#define OE_SYSCALL_fchmod 91
#define OE_SYSCALL_chown 92
#define OE_SYSCALL_fchown 93
#define OE_SYSCALL_lchown 94
#define OE_SYSCALL_umask 95
#define OE_SYSCALL_gettimeofday 96
#define OE_SYSCALL_getrlimit 97
#define OE_SYSCALL_getrusage 98
#define OE_SYSCALL_sysinfo 99
#define OE_SYSCALL_times 100
#define OE_SYSCALL_ptrace 101
#define OE_SYSCALL_getuid 102
#define OE_SYSCALL_syslog 103
#define OE_SYSCALL_getgid 104
#define OE_SYSCALL_setuid 105
#define OE_SYSCALL_setgid 106
#define OE_SYSCALL_geteuid 107
#define OE_SYSCALL_getegid 108
#define OE_SYSCALL_setpgid 109
#define OE_SYSCALL_getppid 110
#define OE_SYSCALL_getpgrp 111
#define OE_SYSCALL_setsid 112
#define OE_SYSCALL_setreuid 113
#define OE_SYSCALL_setregid 114
#define OE_SYSCALL_getgroups 115
#define OE_SYSCALL_setgroups 116
#define OE_SYSCALL_setresuid 117
#define OE_SYSCALL_getresuid 118
#define OE_SYSCALL_setresgid 119
#define OE_SYSCALL_getresgid 120
#define OE_SYSCALL_getpgid 121
#define OE_SYSCALL_setfsuid 122
#define OE_SYSCALL_setfsgid 123
#define OE_SYSCALL_getsid 124
#define OE_SYSCALL_capget 125
#define OE_SYSCALL_capset 126
#define OE_SYSCALL_rt_sigpending 127
#define OE_SYSCALL_rt_sigtimedwait 128
#define OE_SYSCALL_rt_sigqueueinfo 129
#define OE_SYSCALL_rt_sigsuspend 130
#define OE_SYSCALL_sigaltstack 131
#define OE_SYSCALL_utime 132
#define OE_SYSCALL_mknod 133
#define OE_SYSCALL_uselib 134
#define OE_SYSCALL_personality 135
#define OE_SYSCALL_ustat 136
#define OE_SYSCALL_statfs 137
#define OE_SYSCALL_fstatfs 138
#define OE_SYSCALL_sysfs 139
#define OE_SYSCALL_getpriority 140
#define OE_SYSCALL_setpriority 141
#define OE_SYSCALL_sched_setparam 142
#define OE_SYSCALL_sched_getparam 143
#define OE_SYSCALL_sched_setscheduler 144
#define OE_SYSCALL_sched_getscheduler 145
#define OE_SYSCALL_sched_get_priority_max 146
#define OE_SYSCALL_sched_get_priority_min 147
#define OE_SYSCALL_sched_rr_get_interval 148
#define OE_SYSCALL_mlock 149
#define OE_SYSCALL_munlock 150
#define OE_SYSCALL_mlockall 151
#define OE_SYSCALL_munlockall 152
#define OE_SYSCALL_vhangup 153
#define OE_SYSCALL_modify_ldt 154
#define OE_SYSCALL_pivot_root 155
#define OE_SYSCALL__sysctl 156
#define OE_SYSCALL_prctl 157
#define OE_SYSCALL_arch_prctl 158
#define OE_SYSCALL_adjtimex 159
#define OE_SYSCALL_setrlimit 160
#define OE_SYSCALL_chroot 161
#define OE_SYSCALL_sync 162
#define OE_SYSCALL_acct 163
#define OE_SYSCALL_settimeofday 164
#define OE_SYSCALL_mount 165
#define OE_SYSCALL_umount2 166
#define OE_SYSCALL_swapon 167
#define OE_SYSCALL_swapoff 168
#define OE_SYSCALL_reboot 169
#define OE_SYSCALL_sethostname 170
#define OE_SYSCALL_setdomainname 171
#define OE_SYSCALL_iopl 172
#define OE_SYSCALL_ioperm 173
#define OE_SYSCALL_create_module 174
#define OE_SYSCALL_init_module 175
#define OE_SYSCALL_delete_module 176
#define OE_SYSCALL_get_kernel_syms 177
#define OE_SYSCALL_query_module 178
#define OE_SYSCALL_quotactl 179
#define OE_SYSCALL_nfsservctl 180
#define OE_SYSCALL_getpmsg 181
#define OE_SYSCALL_putpmsg 182
#define OE_SYSCALL_afs_syscall 183
#define OE_SYSCALL_tuxcall 184
#define OE_SYSCALL_security 185
#define OE_SYSCALL_gettid 186
#define OE_SYSCALL_readahead 187
#define OE_SYSCALL_setxattr 188
#define OE_SYSCALL_lsetxattr 189
#define OE_SYSCALL_fsetxattr 190
#define OE_SYSCALL_getxattr 191
#define OE_SYSCALL_lgetxattr 192
#define OE_SYSCALL_fgetxattr 193
#define OE_SYSCALL_listxattr 194
#define OE_SYSCALL_llistxattr 195
#define OE_SYSCALL_flistxattr 196
#define OE_SYSCALL_removexattr 197
#define OE_SYSCALL_lremovexattr 198
#define OE_SYSCALL_fremovexattr 199
#define OE_SYSCALL_tkill 200
#define OE_SYSCALL_time 201
#define OE_SYSCALL_futex 202
#define OE_SYSCALL_sched_setaffinity 203
#define OE_SYSCALL_sched_getaffinity 204
#define OE_SYSCALL_set_thread_area 205
#define OE_SYSCALL_io_setup 206
#define OE_SYSCALL_io_destroy 207
#define OE_SYSCALL_io_getevents 208
#define OE_SYSCALL_io_submit 209
#define OE_SYSCALL_io_cancel 210
#define OE_SYSCALL_get_thread_area 211
#define OE_SYSCALL_lookup_dcookie 212
#define OE_SYSCALL_epoll_create 213
#define OE_SYSCALL_epoll_ctl_old 214
#define OE_SYSCALL_epoll_wait_old 215
#define OE_SYSCALL_remap_file_pages 216
#define OE_SYSCALL_getdents64 217
#define OE_SYSCALL_set_tid_address 218
#define OE_SYSCALL_restart_syscall 219
#define OE_SYSCALL_semtimedop 220
#define OE_SYSCALL_fadvise64 221
#define OE_SYSCALL_timer_create 222
#define OE_SYSCALL_timer_settime 223
#define OE_SYSCALL_timer_gettime 224
#define OE_SYSCALL_timer_getoverrun 225
#define OE_SYSCALL_timer_delete 226
#define OE_SYSCALL_clock_settime 227
#define OE_SYSCALL_clock_gettime 228
#define OE_SYSCALL_clock_getres 229
#define OE_SYSCALL_clock_nanosleep 230
#define OE_SYSCALL_exit_group 231
#define OE_SYSCALL_epoll_wait 232
#define OE_SYSCALL_epoll_ctl 233
#define OE_SYSCALL_tgkill 234
#define OE_SYSCALL_utimes 235
#define OE_SYSCALL_vserver 236
#define OE_SYSCALL_mbind 237
#define OE_SYSCALL_set_mempolicy 238
#define OE_SYSCALL_get_mempolicy 239
#define OE_SYSCALL_mq_open 240
#define OE_SYSCALL_mq_unlink 241
#define OE_SYSCALL_mq_timedsend 242
#define OE_SYSCALL_mq_timedreceive 243
#define OE_SYSCALL_mq_notify 244
#define OE_SYSCALL_mq_getsetattr 245
#define OE_SYSCALL_kexec_load 246
#define OE_SYSCALL_waitid 247
#define OE_SYSCALL_add_key 248
#define OE_SYSCALL_request_key 249
#define OE_SYSCALL_keyctl 250
#define OE_SYSCALL_ioprio_set 251
#define OE_SYSCALL_ioprio_get 252
#define OE_SYSCALL_inotify_init 253
#define OE_SYSCALL_inotify_add_watch 254
#define OE_SYSCALL_inotify_rm_watch 255
#define OE_SYSCALL_migrate_pages 256
#define OE_SYSCALL_openat 257
#define OE_SYSCALL_mkdirat 258
#define OE_SYSCALL_mknodat 259
#define OE_SYSCALL_fchownat 260
#define OE_SYSCALL_futimesat 261
#define OE_SYSCALL_newfstatat 262
#define OE_SYSCALL_unlinkat 263
#define OE_SYSCALL_renameat 264
#define OE_SYSCALL_linkat 265
#define OE_SYSCALL_symlinkat 266
#define OE_SYSCALL_readlinkat 267
#define OE_SYSCALL_fchmodat 268
#define OE_SYSCALL_faccessat 269
#define OE_SYSCALL_pselect6 270
#define OE_SYSCALL_ppoll 271
#define OE_SYSCALL_unshare 272
#define OE_SYSCALL_set_robust_list 273
#define OE_SYSCALL_get_robust_list 274
#define OE_SYSCALL_splice 275
#define OE_SYSCALL_tee 276
#define OE_SYSCALL_sync_file_range 277
#define OE_SYSCALL_vmsplice 278
#define OE_SYSCALL_move_pages 279
#define OE_SYSCALL_utimensat 280
#define OE_SYSCALL_epoll_pwait 281
#define OE_SYSCALL_signalfd 282
#define OE_SYSCALL_timerfd_create 283
#define OE_SYSCALL_eventfd 284
#define OE_SYSCALL_fallocate 285
#define OE_SYSCALL_timerfd_settime 286
#define OE_SYSCALL_timerfd_gettime 287
#define OE_SYSCALL_accept4 288
#define OE_SYSCALL_signalfd4 289
#define OE_SYSCALL_eventfd2 290
#define OE_SYSCALL_epoll_create1 291
#define OE_SYSCALL_dup3 292
#define OE_SYSCALL_pipe2 293
#define OE_SYSCALL_inotify_init1 294
#define OE_SYSCALL_preadv 295
#define OE_SYSCALL_pwritev 296
#define OE_SYSCALL_rt_tgsigqueueinfo 297
#define OE_SYSCALL_perf_event_open 298
#define OE_SYSCALL_recvmmsg 299
#define OE_SYSCALL_fanotify_init 300
#define OE_SYSCALL_fanotify_mark 301
#define OE_SYSCALL_prlimit64 302
#define OE_SYSCALL_name_to_handle_at 303
#define OE_SYSCALL_open_by_handle_at 304
#define OE_SYSCALL_clock_adjtime 305
#define OE_SYSCALL_syncfs 306
#define OE_SYSCALL_sendmmsg 307
#define OE_SYSCALL_setns 308
#define OE_SYSCALL_getcpu 309
#define OE_SYSCALL_process_vm_readv 310
#define OE_SYSCALL_process_vm_writev 311
#define OE_SYSCALL_kcmp 312
#define OE_SYSCALL_finit_module 313
#define OE_SYSCALL_sched_setattr 314
#define OE_SYSCALL_sched_getattr 315
#define OE_SYSCALL_renameat2 316
#define OE_SYSCALL_seccomp 317
#define OE_SYSCALL_getrandom 318
#define OE_SYSCALL_memfd_create 319
#define OE_SYSCALL_kexec_file_load 320
#define OE_SYSCALL_bpf 321
#define OE_SYSCALL_execveat 322
#define OE_SYSCALL_userfaultfd 323
#define OE_SYSCALL_membarrier 324
#define OE_SYSCALL_mlock2 325
#define OE_SYSCALL_copy_file_range 326
#define OE_SYSCALL_preadv2 327
#define OE_SYSCALL_pwritev2 328
#define OE_SYSCALL_pkey_mprotect 329
#define OE_SYSCALL_pkey_alloc 330
#define OE_SYSCALL_pkey_free 331
#define OE_SYSCALL_statx 332

typedef struct _oe_host_syscall_args
{
    long num;
    long ret;
    long err; /* errno value */

    union {
        struct
        {
            long arg1;
            long arg2;
            long arg3;
            long arg4;
            long arg5;
            long arg6;
        } generic;
        struct
        {
            const char* pathname;
            long flags;
            long mode;
        } open;
        struct
        {
            long fd;
            void* buf;
            long count;
        } write;
        struct
        {
            long fd;
            void* buf;
            long count;
        } read;
        struct
        {
            long fd;
            long offset;
            long whence;
        } lseek;
        struct
        {
            long fd;
        } close;
    } u;
} oe_host_syscall_args_t;

#endif /* _OE_HOSTSYSCALL_H */
