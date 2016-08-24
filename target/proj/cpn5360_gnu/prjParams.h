/* prjParams.h - dynamically generated configuration header */


/*
GENERATED: Tue Oct 01 01:38:45 PDT 2002
DO NOT EDIT - file is regenerated whenever the project changes
*/

#ifndef INCprjParamsh
#define INCprjParamsh


/*** INCLUDED COMPONENTS ***/

#define INCLUDE_ANSI_ASSERT
#define INCLUDE_ANSI_CTYPE
#define INCLUDE_ANSI_LOCALE
#define INCLUDE_ANSI_MATH
#define INCLUDE_ANSI_STDIO
#define INCLUDE_ANSI_STDIO_EXTRA
#define INCLUDE_ANSI_STDLIB
#define INCLUDE_ANSI_STRING
#define INCLUDE_ANSI_TIME
#define INCLUDE_ARP_API
#define INCLUDE_BOOT_LINE_INIT
#define INCLUDE_BSD
#define INCLUDE_BSD_BOOT
#define INCLUDE_BSD_SOCKET
#define INCLUDE_BUF_MGR
#define INCLUDE_CACHE_ENABLE
#define INCLUDE_CACHE_SUPPORT
#define INCLUDE_CBIO
#define INCLUDE_CTORS_DTORS
#define INCLUDE_DHCPC_LEASE_CLEAN
#define INCLUDE_DISK_CACHE
#define INCLUDE_DLL
#define INCLUDE_DOSFS
#define INCLUDE_DOSFS_CHKDSK
#define INCLUDE_DOSFS_DIR_FIXED
#define INCLUDE_DOSFS_DIR_VFAT
#define INCLUDE_DOSFS_FAT
#define INCLUDE_DOSFS_FMT
#define INCLUDE_DOSFS_MAIN
#define INCLUDE_END
#define INCLUDE_END_BOOT
#define INCLUDE_ENV_VARS
#define INCLUDE_EXC_HANDLING
#define INCLUDE_EXC_SHOW
#define INCLUDE_EXC_TASK
#define INCLUDE_FD
#define INCLUDE_FLOATING_POINT
#define INCLUDE_FORMATTED_IO
#define INCLUDE_FTP
#define INCLUDE_GNU_INTRINSICS
#define INCLUDE_HASH
#define INCLUDE_HOST_TBL
#define INCLUDE_HW_FP
#define INCLUDE_ICMP
#define INCLUDE_IGMP
#define INCLUDE_IO_SYSTEM
#define INCLUDE_IP
#define INCLUDE_KERNEL
#define INCLUDE_LOGGING
#define INCLUDE_LOOPBACK
#define INCLUDE_LSTLIB
#define INCLUDE_MEMORY_CONFIG
#define INCLUDE_MEM_MGR_BASIC
#define INCLUDE_MEM_MGR_FULL
#define INCLUDE_MMU_BASIC
#define INCLUDE_MSG_Q
#define INCLUDE_MSG_Q_SHOW
#define INCLUDE_MUX
#define INCLUDE_NETDEV_CONFIG
#define INCLUDE_NETDEV_NAMEGET
#define INCLUDE_NETMASK_GET
#define INCLUDE_NETWORK
#define INCLUDE_NET_HOST_SETUP
#define INCLUDE_NET_INIT
#define INCLUDE_NET_LIB
#define INCLUDE_NET_REM_IO
#define INCLUDE_NET_SETUP
#define INCLUDE_NET_SHOW
#define INCLUDE_PIPES
#define INCLUDE_POSIX_CLOCKS
#define INCLUDE_RNG_BUF
#define INCLUDE_SECOND_SMNET
#define INCLUDE_SELECT
#define INCLUDE_SELECT_SUPPORT
#define INCLUDE_SEM_BINARY
#define INCLUDE_SEM_COUNTING
#define INCLUDE_SEM_MUTEX
#define INCLUDE_SIGNALS
#define INCLUDE_SIO
#define INCLUDE_SM_COMMON
#define INCLUDE_SM_NET
#define INCLUDE_SM_NET_ADDRGET
#define INCLUDE_STDIO
#define INCLUDE_SYM_TBL
#define INCLUDE_SYSCLK_INIT
#define INCLUDE_SYSHW_INIT
#define INCLUDE_SYS_START
#define INCLUDE_TASK_HOOKS
#define INCLUDE_TASK_VARS
#define INCLUDE_TCP
#define INCLUDE_TFTP_CLIENT
#define INCLUDE_TIMEX
#define INCLUDE_TTY_DEV
#define INCLUDE_UDP
#define INCLUDE_USER_APPL
#define INCLUDE_VXEVENTS
#define INCLUDE_WATCHDOGS
#define INCLUDE_WDB
#define INCLUDE_WDB_BANNER
#define INCLUDE_WDB_BP
#define INCLUDE_WDB_COMM_END
#define INCLUDE_WDB_CTXT
#define INCLUDE_WDB_DIRECT_CALL
#define INCLUDE_WDB_EVENTPOINTS
#define INCLUDE_WDB_EVENTS
#define INCLUDE_WDB_EXC_NOTIFY
#define INCLUDE_WDB_EXIT_NOTIFY
#define INCLUDE_WDB_FUNC_CALL
#define INCLUDE_WDB_GOPHER
#define INCLUDE_WDB_HW_FP
#define INCLUDE_WDB_MEM
#define INCLUDE_WDB_REG
#define INCLUDE_WDB_START_NOTIFY
#define INCLUDE_WDB_SYS
#define INCLUDE_WDB_SYS_HW_FP
#define INCLUDE_WDB_TASK
#define INCLUDE_WDB_TASK_BP
#define INCLUDE_WDB_TASK_HW_FP
#define INCLUDE_WDB_USER_EVENT
#define INCLUDE_WDB_VIO
#define INCLUDE_WDB_VIO_LIB
#undef INCLUDE_PROTECT_TEXT
#undef INCLUDE_PROTECT_VEC_TABLE
#undef INCLUDE_PCI_PARAMS
#undef INCLUDE_VME_PARAMS
#undef INCLUDE_PCMCIA
#undef INCLUDE_SCSI
#undef INCLUDE_LPT
#undef INCLUDE_DSP
#undef INCLUDE_IDE
#undef INCLUDE_ATA
#undef INCLUDE_AUX_CLK
#undef INCLUDE_TIMESTAMP
#undef INCLUDE_PC_CONSOLE
#undef INCLUDE_TFFS
#undef INCLUDE_TFFS_SHOW
#undef INCLUDE_MTD_AMD
#undef INCLUDE_MTD_I28F008
#undef INCLUDE_MTD_I28F008BAJA
#undef INCLUDE_MTD_I28F016
#undef INCLUDE_MTD_WAMDMTD
#undef INCLUDE_MTD_CFIAMD
#undef INCLUDE_MTD_CFISCS
#undef INCLUDE_TL_FTL
#undef INCLUDE_TL_SSFDC
#undef INCLUDE_MMU_FULL
#undef INCLUDE_MMU_MPU
#undef INCLUDE_TYCODRV_5_2
#undef INCLUDE_SW_FP
#undef INCLUDE_DC
#undef INCLUDE_EGL
#undef INCLUDE_EI
#undef INCLUDE_FEI
#undef INCLUDE_EX
#undef INCLUDE_ENP
#undef INCLUDE_IE
#undef INCLUDE_ILAC
#undef INCLUDE_LN
#undef INCLUDE_LNSGI
#undef INCLUDE_NIC
#undef INCLUDE_NIC_EVB
#undef INCLUDE_MED
#undef INCLUDE_ELC
#undef INCLUDE_ULTRA
#undef INCLUDE_EEX
#undef INCLUDE_ELT
#undef INCLUDE_QU
#undef INCLUDE_ENE
#undef INCLUDE_ESMC
#undef INCLUDE_SN
#undef INCLUDE_OLI
#undef INCLUDE_USR_ENTRIES
#undef INCLUDE_IF_USR
#undef INCLUDE_LNEBSA
#undef INCLUDE_FN
#undef INCLUDE_RPC
#undef INCLUDE_ZBUF_SOCK
#undef INCLUDE_PPP
#undef INCLUDE_PPP_BOOT
#undef INCLUDE_PPP_CRYPT
#undef INCLUDE_SLIP
#undef INCLUDE_SLIP_BOOT
#undef INCLUDE_RIP
#undef INCLUDE_ROUTE_SOCK
#undef INCLUDE_DNS_RESOLVER
#undef INCLUDE_DHCPC
#undef INCLUDE_DHCPR
#undef INCLUDE_DHCPS
#undef INCLUDE_SNTPC
#undef INCLUDE_SNTPS
#undef INCLUDE_PING
#undef INCLUDE_TELNET
#undef INCLUDE_RLOGIN
#undef INCLUDE_SECURITY
#undef INCLUDE_FTP_SERVER
#undef INCLUDE_FTPD_SECURITY
#undef INCLUDE_NFS
#undef INCLUDE_NFS_MOUNT_ALL
#undef INCLUDE_NFS_SERVER
#undef INCLUDE_TFTP_SERVER
#undef INCLUDE_PROXY_CLIENT
#undef INCLUDE_PROXY_SERVER
#undef INCLUDE_TCP_DEBUG
#undef INCLUDE_IP_FILTER
#undef BSD43_COMPATIBLE
#undef INCLUDE_MCAST_ROUTING
#undef INCLUDE_SM_SEQ_ADDR
#undef INCLUDE_PROXY_DEFAULT_ADDR
#undef INCLUDE_MIB2_ALL
#undef INCLUDE_MIB2_AT
#undef INCLUDE_MIB2_ICMP
#undef INCLUDE_MIB2_IF
#undef INCLUDE_MIB2_IP
#undef INCLUDE_MIB2_SYSTEM
#undef INCLUDE_MIB2_TCP
#undef INCLUDE_MIB2_UDP
#undef INCLUDE_SNMPD
#undef INCLUDE_DHCPC_SHOW
#undef INCLUDE_UDP_SHOW
#undef INCLUDE_TCP_SHOW
#undef INCLUDE_ICMP_SHOW
#undef INCLUDE_IGMP_SHOW
#undef INCLUDE_SM_NET_SHOW
#undef INCLUDE_DHCPC_LEASE_TEST
#undef INCLUDE_DHCPC_LEASE_SAVE
#undef INCLUDE_DEFER_NET_INIT
#undef INCLUDE_DHCPC_LEASE_GET
#undef INCLUDE_CPLUS
#undef INCLUDE_CPLUS_LANG
#undef INCLUDE_DIAB_INTRINSICS
#undef INCLUDE_NO_INTRINSICS
#undef INCLUDE_CPLUS_STRING
#undef INCLUDE_CPLUS_COMPLEX
#undef INCLUDE_CPLUS_STL
#undef INCLUDE_CPLUS_IOSTREAMS
#undef INCLUDE_CPLUS_IOSTREAMS_FULL
#undef INCLUDE_CPLUS_STRING_IO
#undef INCLUDE_CPLUS_COMPLEX_IO
#undef INCLUDE_POSIX_AIO
#undef INCLUDE_POSIX_AIO_SYSDRV
#undef INCLUDE_POSIX_FTRUNC
#undef INCLUDE_POSIX_MEM
#undef INCLUDE_POSIX_MQ
#undef INCLUDE_POSIX_PTHREADS
#undef INCLUDE_POSIX_SCHED
#undef INCLUDE_POSIX_SEM
#undef INCLUDE_POSIX_SIGNALS
#undef INCLUDE_POSIX_TIMERS
#undef INCLUDE_CLASS_SHOW
#undef INCLUDE_MEM_SHOW
#undef INCLUDE_MMU_FULL_SHOW
#undef INCLUDE_POSIX_AIO_SHOW
#undef INCLUDE_POSIX_MQ_SHOW
#undef INCLUDE_POSIX_SEM_SHOW
#undef INCLUDE_SEM_SHOW
#undef INCLUDE_STDIO_SHOW
#undef INCLUDE_SYM_TBL_SHOW
#undef INCLUDE_TASK_HOOKS_SHOW
#undef INCLUDE_TASK_SHOW
#undef INCLUDE_WATCHDOGS_SHOW
#undef INCLUDE_TRIGGER_SHOW
#undef INCLUDE_RBUFF_SHOW
#undef INCLUDE_ATA_SHOW
#undef INCLUDE_PCI_CFGSHOW
#undef INCLUDE_HW_FP_SHOW
#undef INCLUDE_DSP_SHOW
#undef INCLUDE_TRIGGERING
#undef INCLUDE_RBUFF
#undef INCLUDE_SPY
#undef INCLUDE_CODETEST
#undef INCLUDE_DEBUG
#undef INCLUDE_SHELL_BANNER
#undef INCLUDE_STARTUP_SCRIPT
#undef INCLUDE_SHELL
#undef INCLUDE_STAT_SYM_TBL
#undef INCLUDE_SYM_TBL_SYNC
#undef INCLUDE_CPLUS_DEMANGLER
#undef INCLUDE_SYM_TBL_INIT
#undef INCLUDE_STANDALONE_SYM_TBL
#undef INCLUDE_NET_SYM_TBL
#undef INCLUDE_LOADER
#undef INCLUDE_UNLOADER
#undef INCLUDE_MODULE_MANAGER
#undef INCLUDE_WDB_DSP
#undef INCLUDE_WDB_TASK_DSP
#undef INCLUDE_WDB_SYS_DSP
#undef INCLUDE_WDB_TSFS
#undef INCLUDE_WDB_COMM_SERIAL
#undef INCLUDE_WDB_COMM_TYCODRV_5_2
#undef INCLUDE_WDB_COMM_NETWORK
#undef INCLUDE_WDB_COMM_NETROM
#undef INCLUDE_WDB_COMM_VTMD
#undef INCLUDE_WDB_COMM_CUSTOM
#undef INCLUDE_WDB_COMM_PIPE
#undef INCLUDE_CDROMFS
#undef INCLUDE_DISK_UTIL
#undef INCLUDE_RT11FS
#undef INCLUDE_RAWFS
#undef INCLUDE_RAMDRV
#undef INCLUDE_WINDVIEW
#undef INCLUDE_WINDVIEW_CLASS
#undef INCLUDE_WVNET
#undef INCLUDE_WVUPLOAD_FILE
#undef INCLUDE_WVUPLOAD_SOCK
#undef INCLUDE_WVUPLOAD_TSFSSOCK
#undef INCLUDE_SYS_TIMESTAMP
#undef INCLUDE_USER_TIMESTAMP
#undef INCLUDE_SEQ_TIMESTAMP
#undef INCLUDE_WV_BUFF_USER
#undef INCLUDE_HTTP
#undef INCLUDE_AUDIO_WAVE
#undef INCLUDE_AUDIO_AU
#undef INCLUDE_JAVA
#undef INCLUDE_JAVA_FILE_CONFIG
#undef INCLUDE_JAVA_MEM_CONFIG
#undef INCLUDE_JAVA_JVM_CONFIG
#undef INCLUDE_JAVA_DNS_CONFIG
#undef JAVA_FILE_CONFIG_TSFS
#undef JAVA_FILE_CONFIG_NFS
#undef JAVA_FILE_CONFIG_NETDRV
#undef INCLUDE_BPF
#undef INCLUDE_2D_UGL
#undef INCLUDE_UGL_ALL
#undef UGL_CONFIG_INCLUDE_INCREMENT
#undef INCLUDE_UGL2_0
#undef INCLUDE_SM_OBJ
#undef INCLUDE_HTML
#undef INCLUDE_HTML_SSL
#undef INCLUDE_VXFUSION_DIST_MSG_Q_SHOW
#undef INCLUDE_VXFUSION_GRP_MSG_Q_SHOW
#undef INCLUDE_VXFUSION_DIST_NAME_DB_SHOW
#undef INCLUDE_VXFUSION_IF_SHOW
#undef INCLUDE_VXFUSION
#undef INCLUDE_ENAVIGATOR
#undef INCLUDE_HTML_MAIL
#undef INCLUDE_HTMLWORKS
#undef INCLUDE_HTTP_SYMBOL_TABLE
#undef INCLUDE_HTTP_LANGUAGE_NEGOTIATION
#undef INCLUDE_HTTP_COOKIE_SUPPORT
#undef HTTP_DEBUG
#undef INCLUDE_HTTP_MIME_HDR_PROC
#undef INCLUDE_HTTP_PERSISTENT_CONNECTION
#undef INCLUDE_HTTP_SOCKET_LIGHT
#undef INCLUDE_HTTP_FS
#undef INCLUDE_HTTP_BASICMEMFS
#undef INCLUDE_HTTP_SYSTEMMEMFS
#undef INCLUDE_HTTP_SERVER_UNCOMPRESS
#undef INCLUDE_HTTP_ACCESS_CTRL
#undef INCLUDE_HTTP_ALIASING
#undef INCLUDE_HTTP_IP_CHECKING
#undef INCLUDE_HTTP_LOG
#undef INCLUDE_HTTP_MIME_GUESSTYPE
#undef INCLUDE_HTTP_PWD_CHECKING
#undef INCLUDE_HTTP_FUNC_MULTIPLEXER
#undef INCLUDE_HTTP_UPLOAD
#undef INCLUDE_HTTP_CGI
#undef INCLUDE_HTTP_EVALUATION
#undef INCLUDE_HTTP_SHOWAUTH
#undef INCLUDE_HTTP_SSI
#undef INCLUDE_HTTP_SYMBOL_SHOW
#undef INCLUDE_HTTP_LIVE_CONTROL
#undef INCLUDE_HTTP_DAEMON
#undef DEBUG_INFO
#undef DEBUG_ERROR
#undef INCLUDE_DISK_PART
#undef INCLUDE_TAR
#undef INCLUDE_RAM_DISK
#undef INCLUDE_USB
#undef INCLUDE_OHCI
#undef INCLUDE_UHCI
#undef INCLUDE_USB_TARG
#undef INCLUDE_KBD_EMULATOR
#undef INCLUDE_PRN_EMULATOR
#undef INCLUDE_D12_EMULATOR
#undef INCLUDE_USB_MOUSE
#undef INCLUDE_USB_KEYBOARD
#undef INCLUDE_USB_PRINTER
#undef INCLUDE_USB_SPEAKER
#undef INCLUDE_USB_PEGASUS_END
#undef INCLUDE_USB_MS_BULKONLY
#undef INCLUDE_USB_MS_CBI
#undef INCLUDE_USB_INIT
#undef INCLUDE_UHCI_INIT
#undef INCLUDE_OHCI_INIT
#undef INCLUDE_USBTOOL
#undef INCLUDE_USB_AUDIO_DEMO
#undef INCLUDE_USB_MOUSE_INIT
#undef INCLUDE_USB_KEYBOARD_INIT
#undef INCLUDE_USB_PRINTER_INIT
#undef INCLUDE_USB_SPEAKER_INIT
#undef INCLUDE_USB_MS_BULKONLY_INIT
#undef INCLUDE_USB_MS_CBI_INIT
#undef INCLUDE_USB_PEGASUS_END_INIT
#undef INCLUDE_COM_CORE
#undef INCLUDE_COM_SHOW
#undef INCLUDE_COM_NTP_TIME
#undef INCLUDE_COM
#undef INCLUDE_DCOM
#undef INCLUDE_DCOM_PROXY
#undef INCLUDE_DCOM_OPC
#undef INCLUDE_DCOM_SHOW


/*** PARAMETERS ***/

#undef  LOCAL_MEM_LOCAL_ADRS
#define LOCAL_MEM_LOCAL_ADRS 0x00100000
#undef  LOCAL_MEM_SIZE
#define LOCAL_MEM_SIZE 0x00800000
#undef  LOCAL_MEM_AUTOSIZE
#define LOCAL_MEM_AUTOSIZE
#undef  USER_RESERVED_MEM
#define USER_RESERVED_MEM 0
#undef  NV_RAM_SIZE
#define NV_RAM_SIZE NONE
#undef  NV_BOOT_OFFSET
#define NV_BOOT_OFFSET 0x80
#undef  VEC_BASE_ADRS
#define VEC_BASE_ADRS ((char *) LOCAL_MEM_LOCAL_ADRS)
#undef  EXC_MSG_OFFSET
#define EXC_MSG_OFFSET 0x1300
#undef  EXC_MSG_ADRS
#define EXC_MSG_ADRS ((char *) (LOCAL_MEM_LOCAL_ADRS+EXC_MSG_OFFSET))
#undef  BOOT_LINE_SIZE
#define BOOT_LINE_SIZE 255
#undef  BOOT_LINE_ADRS
#define BOOT_LINE_ADRS ((char *) (LOCAL_MEM_LOCAL_ADRS+BOOT_LINE_OFFSET))
#undef  BOOT_LINE_OFFSET
#define BOOT_LINE_OFFSET 0x1200
#undef  DEFAULT_BOOT_LINE
#define DEFAULT_BOOT_LINE "fd=0,0(0,0)host:/fd0/vxWorks.st h=90.0.0.3 e=90.0.0.50 u=target"
#undef  RESERVED
#define RESERVED 0
#undef  FREE_RAM_ADRS
#define FREE_RAM_ADRS (end)
#undef  ROM_WARM_ADRS
#define ROM_WARM_ADRS ("unused")	
#undef  STACK_SAVE
#define STACK_SAVE 0x40
#undef  RAM_HIGH_ADRS
#define RAM_HIGH_ADRS 0x00108000
#undef  RAM_LOW_ADRS
#define RAM_LOW_ADRS 0x00308000
#undef  ROM_BASE_ADRS
#define ROM_BASE_ADRS 0x00008000
#undef  ROM_TEXT_ADRS
#define ROM_TEXT_ADRS (ROM_BASE_ADRS)
#undef  ROM_SIZE
#define ROM_SIZE 0x00090000
#undef  USER_I_CACHE_MODE
#define USER_I_CACHE_MODE CACHE_WRITETHROUGH
#undef  USER_D_CACHE_MODE
#define USER_D_CACHE_MODE (CACHE_COPYBACK | CACHE_SNOOP_ENABLE)
#undef  USER_I_CACHE_ENABLE
#define USER_I_CACHE_ENABLE
#undef  USER_D_CACHE_ENABLE
#define USER_D_CACHE_ENABLE
#undef  VM_PAGE_SIZE
#define VM_PAGE_SIZE PAGE_SIZE_4KB
#undef  USER_D_MMU_ENABLE
#define USER_D_MMU_ENABLE
#undef  USER_I_MMU_ENABLE
#define USER_I_MMU_ENABLE
#undef  FD_INT_VEC
#define FD_INT_VEC (INT_VEC_GET (FD_INT_LVL))
#undef  FD_INT_LVL
#define FD_INT_LVL 0x06
#undef  SYS_CLK_RATE
#define SYS_CLK_RATE 60
#undef  SYS_CLK_RATE_MIN
#define SYS_CLK_RATE_MIN 19
#undef  SYS_CLK_RATE_MAX
#define SYS_CLK_RATE_MAX (PIT_CLOCK/256)
#undef  NUM_TTY
#define NUM_TTY (N_UART_CHANNELS)
#undef  CONSOLE_TTY
#define CONSOLE_TTY 0
#undef  CONSOLE_BAUD_RATE
#define CONSOLE_BAUD_RATE 9600
#undef  RT_BUFFERED_DISPLAY
#define RT_BUFFERED_DISPLAY FALSE
#undef  RT_DISPLAY_MEMORY
#define RT_DISPLAY_MEMORY 10000
#undef  SM_PKTS_SIZE
#define SM_PKTS_SIZE 0
#undef  SM_MAX_PKTS
#define SM_MAX_PKTS 200
#undef  IP_FLAGS_DFLT
#define IP_FLAGS_DFLT (IP_DO_FORWARDING | IP_DO_REDIRECT |                          IP_DO_CHECKSUM_SND | IP_DO_CHECKSUM_RCV)
#undef  IP_TTL_DFLT
#define IP_TTL_DFLT 64
#undef  IP_QLEN_DFLT
#define IP_QLEN_DFLT 50
#undef  IP_FRAG_TTL_DFLT
#define IP_FRAG_TTL_DFLT 60
#undef  ICMP_FLAGS_DFLT
#define ICMP_FLAGS_DFLT (ICMP_NO_MASK_REPLY)
#undef  UDP_FLAGS_DFLT
#define UDP_FLAGS_DFLT (UDP_DO_CKSUM_SND | UDP_DO_CKSUM_RCV)
#undef  UDP_SND_SIZE_DFLT
#define UDP_SND_SIZE_DFLT 9216
#undef  UDP_RCV_SIZE_DFLT
#define UDP_RCV_SIZE_DFLT 41600
#undef  TCP_FLAGS_DFLT
#define TCP_FLAGS_DFLT (TCP_DO_RFC1323)
#undef  TCP_SND_SIZE_DFLT
#define TCP_SND_SIZE_DFLT 8192
#undef  TCP_RCV_SIZE_DFLT
#define TCP_RCV_SIZE_DFLT 8192
#undef  TCP_CON_TIMEO_DFLT
#define TCP_CON_TIMEO_DFLT 150
#undef  TCP_REXMT_THLD_DFLT
#define TCP_REXMT_THLD_DFLT 3
#undef  TCP_MSS_DFLT
#define TCP_MSS_DFLT 512
#undef  TCP_RND_TRIP_DFLT
#define TCP_RND_TRIP_DFLT 3
#undef  TCP_IDLE_TIMEO_DFLT
#define TCP_IDLE_TIMEO_DFLT 14400
#undef  TCP_MAX_PROBE_DFLT
#define TCP_MAX_PROBE_DFLT 8
#undef  NUM_FILES
#define NUM_FILES 50
#undef  FTP_TRANSIENT_MAX_RETRY_COUNT
#define FTP_TRANSIENT_MAX_RETRY_COUNT 100
#undef  FTP_TRANSIENT_RETRY_INTERVAL
#define FTP_TRANSIENT_RETRY_INTERVAL 0
#undef  FTP_TRANSIENT_FATAL
#define FTP_TRANSIENT_FATAL ftpTransientFatal
#undef  FTP_DEBUG_OPTIONS
#define FTP_DEBUG_OPTIONS 0
#undef  NUM_NET_MBLKS
#define NUM_NET_MBLKS 400
#undef  NUM_CL_BLKS
#define NUM_CL_BLKS (NUM_64 + NUM_128 + NUM_256 +      					 NUM_512 + NUM_1024 + NUM_2048)
#undef  NUM_64
#define NUM_64 100
#undef  NUM_128
#define NUM_128 100
#undef  NUM_256
#define NUM_256 40
#undef  NUM_512
#define NUM_512 40
#undef  NUM_1024
#define NUM_1024 25
#undef  NUM_2048
#define NUM_2048 25
#undef  NUM_SYS_MBLKS
#define NUM_SYS_MBLKS (2 * NUM_SYS_CL_BLKS)
#undef  NUM_SYS_CL_BLKS
#define NUM_SYS_CL_BLKS (NUM_SYS_64  + NUM_SYS_128 + 					 NUM_SYS_256 + NUM_SYS_512)
#undef  NUM_SYS_64
#define NUM_SYS_64 64
#undef  NUM_SYS_128
#define NUM_SYS_128 64
#undef  NUM_SYS_256
#define NUM_SYS_256 64
#undef  NUM_SYS_512
#define NUM_SYS_512 64
#undef  IP_MAX_UNITS
#define IP_MAX_UNITS 1
#undef  ARP_MAX_ENTRIES
#define ARP_MAX_ENTRIES 20
#undef  CLEAR_BSS
#define CLEAR_BSS
#undef  WDB_STACK_SIZE
#define WDB_STACK_SIZE 0x1000
#undef  WDB_BP_MAX
#define WDB_BP_MAX 50
#undef  WDB_SPAWN_PRI
#define WDB_SPAWN_PRI 100
#undef  WDB_SPAWN_OPTS
#define WDB_SPAWN_OPTS VX_FP_TASK
#undef  WDB_TASK_PRIORITY
#define WDB_TASK_PRIORITY 3
#undef  WDB_TASK_OPTIONS
#define WDB_TASK_OPTIONS VX_UNBREAKABLE | VX_FP_TASK
#undef  WDB_RESTART_TIME
#define WDB_RESTART_TIME 10
#undef  WDB_MAX_RESTARTS
#define WDB_MAX_RESTARTS 5
#undef  WDB_MTU
#define WDB_MTU 1500
#undef  INCLUDE_CONSTANT_RDY_Q
#define INCLUDE_CONSTANT_RDY_Q
#undef  ROOT_STACK_SIZE
#define ROOT_STACK_SIZE 10000
#undef  ISR_STACK_SIZE
#define ISR_STACK_SIZE 1000
#undef  INT_LOCK_LEVEL
#define INT_LOCK_LEVEL 0x0
#undef  ENV_VAR_USE_HOOKS
#define ENV_VAR_USE_HOOKS TRUE
#undef  NUM_DOSFS_FILES
#define NUM_DOSFS_FILES 20
#undef  NUM_DRIVERS
#define NUM_DRIVERS 20
#undef  MAX_LOG_MSGS
#define MAX_LOG_MSGS 50
#undef  MUX_MAX_BINDS
#define MUX_MAX_BINDS 16
#undef  USR_MAX_LINK_HDR
#define USR_MAX_LINK_HDR 16
#undef  SM_MASTER
#define SM_MASTER 0
#undef  SM_CPUS_MAX
#define SM_CPUS_MAX 0
#undef  SM_OFF_BOARD
#define SM_OFF_BOARD TRUE
#undef  SM_ANCHOR_OFFSET
#define SM_ANCHOR_OFFSET 0x1100
#undef  SM_ANCHOR_ADRS
#define SM_ANCHOR_ADRS (sysSmAnchorAdrs())
#undef  SM_INT_TYPE
#define SM_INT_TYPE SM_INT_MAILBOX_1
#undef  SM_INT_ARG1
#define SM_INT_ARG1 (smIntArg1)
#undef  SM_INT_ARG2
#define SM_INT_ARG2 (smIntArg2)
#undef  SM_INT_ARG3
#define SM_INT_ARG3 (1 << (DEC2155X_SM_DOORBELL_BIT % 8))
#undef  SM_MAX_WAIT
#define SM_MAX_WAIT 3000
#undef  SM_MEM_ADRS
#define SM_MEM_ADRS 0x00100000
#undef  SM_MEM_SIZE
#define SM_MEM_SIZE 0x00010000
#undef  SM_TAS_TYPE
#define SM_TAS_TYPE SM_TAS_HARD
#undef  SM_OBJ_MEM_SIZE
#define SM_OBJ_MEM_SIZE 0x00010000
#undef  FD_DEV_NAME
#define FD_DEV_NAME "/fd"	 
#undef  FD_CACHE_SIZE
#define FD_CACHE_SIZE (128*1024)
#undef  FD_0_TYPE
#define FD_0_TYPE 0
#undef  FD_1_TYPE
#define FD_1_TYPE NONE

#endif /* INCprjParamsh */
