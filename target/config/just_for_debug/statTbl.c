/* mkstatbl.c - status code symbol table */

/* CREATED BY mkstatbl
 *       FROM 
 *         ON Fri Jul 15 11:30:24 中国标准时间 2016
 */

#include "vxWorks.h"
#include "symbol.h"
#include "rpc/rpctypes.h"
#include "xdr_nfs.h"
#include "rpc/xdr.h"
#include "rpc/auth.h"
#include "rpc/clnt.h"

#include "vwmodnum.h"

#define S_arpLib_INVALID_ARGUMENT		(M_arpLib | 1)
#define S_arpLib_INVALID_HOST 			(M_arpLib | 2)
#define S_arpLib_INVALID_ENET_ADDRESS 		(M_arpLib | 3)
#define S_arpLib_INVALID_FLAG			(M_arpLib | 4)
#define S_bootLoadLib_ROUTINE_NOT_INSTALLED		(M_bootLoadLib | 1)
#define S_bootpLib_INVALID_ARGUMENT	(M_bootpLib | 1)
#define S_bootpLib_INVALID_COOKIE	(M_bootpLib | 2)
#define S_bootpLib_NO_BROADCASTS	(M_bootpLib | 3)
#define S_bootpLib_PARSE_ERROR		(M_bootpLib | 4)
#define S_bootpLib_INVALID_TAG		(M_bootpLib | 5)
#define S_bootpLib_TIME_OUT		(M_bootpLib | 6)
#define S_bootpLib_MEM_ERROR		(M_bootpLib | 7)
#define S_bootpLib_NOT_INITIALIZED 	(M_bootpLib | 8)
#define S_bootpLib_BAD_DEVICE 		(M_bootpLib | 9)
#define S_cacheLib_INVALID_CACHE	(M_cacheLib | 1)
#define S_cbioLib_INVALID_CBIO_DEV_ID	(M_cbioLib | 1)
#define S_cdromFsLib_ALREADY_INIT			(M_cdromFsLib | 1)
#define S_cdromFsLib_DEVICE_REMOVED			(M_cdromFsLib | 3)
#define S_cdromFsLib_SUCH_PATH_TABLE_SIZE_NOT_SUPPORTED (M_cdromFsLib | 4)
#define S_cdromFsLib_ONE_OF_VALUES_NOT_POWER_OF_2	(M_cdromFsLib | 5)
#define S_cdromFsLib_UNNOWN_FILE_SYSTEM			(M_cdromFsLib | 6)
#define S_cdromFsLib_INVAL_VOL_DESCR			(M_cdromFsLib | 7)
#define S_cdromFsLib_INVALID_PATH_STRING		(M_cdromFsLib | 8)
#define S_cdromFsLib_MAX_DIR_HIERARCHY_LEVEL_OVERFLOW	(M_cdromFsLib | 9)
#define S_cdromFsLib_NO_SUCH_FILE_OR_DIRECTORY		(M_cdromFsLib | 10)
#define S_cdromFsLib_INVALID_DIRECTORY_STRUCTURE	(M_cdromFsLib | 11)
#define S_cdromFsLib_INVALID_FILE_DESCRIPTOR		(M_cdromFsLib | 12)
#define S_cdromFsLib_READ_ONLY_DEVICE			(M_cdromFsLib | 13)
#define S_cdromFsLib_END_OF_FILE			(M_cdromFsLib | 14)
#define S_cdromFsLib_INV_ARG_VALUE			(M_cdromFsLib | 15)
#define S_cdromFsLib_SEMTAKE_ERROR			(M_cdromFsLib | 16)
#define S_cdromFsLib_SEMGIVE_ERROR			(M_cdromFsLib | 17)
#define S_cdromFsLib_VOL_UNMOUNTED			(M_cdromFsLib | 18)
#define S_cdromFsLib_INVAL_DIR_OPER			(M_cdromFsLib | 19)
#define S_cdromFsLib_READING_FAILURE			(M_cdromFsLib | 20)
#define S_cdromFsLib_INVALID_DIR_REC_STRUCT             (M_cdromFsLib | 21)
#define S_classLib_CLASS_ID_ERROR		(M_classLib | 1)
#define S_classLib_NO_CLASS_DESTROY		(M_classLib | 2)
#define S_dhcpsLib_NOT_INITIALIZED           (M_dhcpsLib | 1)
#define S_dosFsLib_32BIT_OVERFLOW		(M_dosFsLib |  1)
#define S_dosFsLib_DISK_FULL			(M_dosFsLib |  2)
#define S_dosFsLib_FILE_NOT_FOUND		(M_dosFsLib |  3)
#define S_dosFsLib_NO_FREE_FILE_DESCRIPTORS 	(M_dosFsLib |  4)
#define S_dosFsLib_NOT_FILE			(M_dosFsLib |  5)
#define S_dosFsLib_NOT_SAME_VOLUME	        (M_dosFsLib |  6)
#define S_dosFsLib_NOT_DIRECTORY		(M_dosFsLib |  7)
#define S_dosFsLib_DIR_NOT_EMPTY		(M_dosFsLib |  8)
#define S_dosFsLib_FILE_EXISTS			(M_dosFsLib |  9)
#define S_dosFsLib_INVALID_PARAMETER		(M_dosFsLib | 10)
#define S_dosFsLib_NAME_TOO_LONG		(M_dosFsLib | 11)
#define S_dosFsLib_UNSUPPORTED			(M_dosFsLib | 12)
#define S_dosFsLib_VOLUME_NOT_AVAILABLE		(M_dosFsLib | 13)
#define S_dosFsLib_INVALID_NUMBER_OF_BYTES	(M_dosFsLib | 14)
#define S_dosFsLib_ILLEGAL_NAME			(M_dosFsLib | 15)
#define S_dosFsLib_CANT_DEL_ROOT		(M_dosFsLib | 16)
#define S_dosFsLib_READ_ONLY			(M_dosFsLib | 17)
#define S_dosFsLib_ROOT_DIR_FULL		(M_dosFsLib | 18)
#define S_dosFsLib_NO_LABEL			(M_dosFsLib | 19)
#define S_dosFsLib_NO_CONTIG_SPACE		(M_dosFsLib | 20)
#define S_dosFsLib_FD_OBSOLETE			(M_dosFsLib | 21)
#define S_dosFsLib_DELETED			(M_dosFsLib | 22)
#define S_dosFsLib_INTERNAL_ERROR		(M_dosFsLib | 23)
#define S_dosFsLib_WRITE_ONLY			(M_dosFsLib | 24)
#define S_dosFsLib_ILLEGAL_PATH			(M_dosFsLib | 25)
#define S_dosFsLib_ACCESS_BEYOND_EOF		(M_dosFsLib | 26)
#define S_dosFsLib_DIR_READ_ONLY		(M_dosFsLib | 27)
#define S_dosFsLib_UNKNOWN_VOLUME_FORMAT	(M_dosFsLib | 28)
#define S_dosFsLib_ILLEGAL_CLUSTER_NUMBER	(M_dosFsLib | 29)
#define S_errnoLib_NO_STAT_SYM_TBL      (M_errnoLib | 1)
#define S_eventLib_TIMEOUT			(M_eventLib | 0x0001)
#define S_eventLib_NOT_ALL_EVENTS		(M_eventLib | 0x0002)
#define S_eventLib_ALREADY_REGISTERED		(M_eventLib | 0x0003)
#define S_eventLib_EVENTSEND_FAILED		(M_eventLib | 0x0004)
#define S_eventLib_ZERO_EVENTS			(M_eventLib | 0x0005)
#define S_eventLib_TASK_NOT_REGISTERED		(M_eventLib | 0x0006)
#define S_eventLib_NULL_TASKID_AT_INT_LEVEL	(M_eventLib | 0x0007)
#define S_ftpLib_ILLEGAL_VALUE                  (M_ftpLib | 1)
#define S_ftpLib_TRANSIENT_RETRY_LIMIT_EXCEEDED (M_ftpLib | 2)
#define S_ftpLib_FATAL_TRANSIENT_RESPONSE       (M_ftpLib | 3)
#define S_ftpLib_REMOTE_SERVER_STATUS_221       (M_ftpLib | 221)
#define S_ftpLib_REMOTE_SERVER_STATUS_226       (M_ftpLib | 226)
#define S_ftpLib_REMOTE_SERVER_STATUS_257       (M_ftpLib | 257)
#define S_ftpLib_REMOTE_SERVER_ERROR_422        (M_ftpLib | 422)
#define S_ftpLib_REMOTE_SERVER_ERROR_425        (M_ftpLib | 425)
#define S_ftpLib_REMOTE_SERVER_ERROR_450        (M_ftpLib | 450)
#define S_ftpLib_REMOTE_SERVER_ERROR_451        (M_ftpLib | 451)
#define S_ftpLib_REMOTE_SERVER_ERROR_452        (M_ftpLib | 452)
#define S_ftpLib_REMOTE_SERVER_ERROR_500        (M_ftpLib | 500)
#define S_ftpLib_REMOTE_SERVER_ERROR_501        (M_ftpLib | 501)
#define S_ftpLib_REMOTE_SERVER_ERROR_502        (M_ftpLib | 502)
#define S_ftpLib_REMOTE_SERVER_ERROR_503        (M_ftpLib | 503)
#define S_ftpLib_REMOTE_SERVER_ERROR_504        (M_ftpLib | 504)
#define S_ftpLib_REMOTE_SERVER_ERROR_520        (M_ftpLib | 520)
#define S_ftpLib_REMOTE_SERVER_ERROR_521        (M_ftpLib | 521)
#define S_ftpLib_REMOTE_SERVER_ERROR_530        (M_ftpLib | 530)
#define S_ftpLib_REMOTE_SERVER_ERROR_550        (M_ftpLib | 550)
#define S_ftpLib_REMOTE_SERVER_ERROR_551        (M_ftpLib | 551)
#define S_ftpLib_REMOTE_SERVER_ERROR_552        (M_ftpLib | 552)
#define S_ftpLib_REMOTE_SERVER_ERROR_553        (M_ftpLib | 553)
#define S_ftpLib_REMOTE_SERVER_ERROR_554        (M_ftpLib | 554)
#define S_hashLib_KEY_CLASH		(M_hashLib | 1)
#define S_hostLib_UNKNOWN_HOST			(M_hostLib | 1)
#define S_hostLib_HOST_ALREADY_ENTERED		(M_hostLib | 2)
#define S_hostLib_INVALID_PARAMETER             (M_hostLib | 3)
#define S_icmpLib_TIMEOUT		(M_icmpLib | 1)
#define S_icmpLib_NO_BROADCAST		(M_icmpLib | 2)
#define S_icmpLib_INVALID_INTERFACE	(M_icmpLib | 3)
#define S_icmpLib_INVALID_ARGUMENT	(M_icmpLib | 4)
#define S_if_sl_INVALID_UNIT_NUMBER			(M_if_sl | 1)
#define S_if_sl_UNIT_UNINITIALIZED			(M_if_sl | 2)
#define S_if_sl_UNIT_ALREADY_INITIALIZED		(M_if_sl | 3)
#define S_inetLib_ILLEGAL_INTERNET_ADDRESS		(M_inetLib | 1)
#define S_inetLib_ILLEGAL_NETWORK_NUMBER		(M_inetLib | 2)
#define S_intLib_NOT_ISR_CALLABLE		(M_intLib | 1)
#define S_intLib_VEC_TABLE_WP_UNAVAILABLE	(M_intLib | 2)
#define S_ioLib_NO_DRIVER		(M_ioLib | 1)
#define S_ioLib_UNKNOWN_REQUEST		(M_ioLib | 2)
#define S_ioLib_DEVICE_ERROR		(M_ioLib | 3)
#define S_ioLib_DEVICE_TIMEOUT		(M_ioLib | 4)
#define S_ioLib_WRITE_PROTECTED		(M_ioLib | 5)
#define S_ioLib_DISK_NOT_PRESENT	(M_ioLib | 6)
#define S_ioLib_NO_FILENAME		(M_ioLib | 7)
#define S_ioLib_CANCELLED		(M_ioLib | 8)
#define S_ioLib_NO_DEVICE_NAME_IN_PATH	(M_ioLib | 9)
#define S_ioLib_NAME_TOO_LONG		(M_ioLib | 10)
#define S_ioLib_UNFORMATED		(M_ioLib | 11)
#define S_ioLib_CANT_OVERWRITE_DIR      (M_ioLib | 12)
#define S_iosLib_DEVICE_NOT_FOUND		(M_iosLib | 1)
#define S_iosLib_DRIVER_GLUT			(M_iosLib | 2)
#define S_iosLib_INVALID_FILE_DESCRIPTOR	(M_iosLib | 3)
#define S_iosLib_TOO_MANY_OPEN_FILES		(M_iosLib | 4)
#define S_iosLib_CONTROLLER_NOT_PRESENT		(M_iosLib | 5)
#define S_iosLib_DUPLICATE_DEVICE_NAME		(M_iosLib | 6)
#define S_iosLib_INVALID_ETHERNET_ADDRESS	(M_iosLib | 7)
#define S_loadAoutLib_TOO_MANY_SYMBOLS		(M_loadAoutLib | 1)
#define S_loadLib_FILE_READ_ERROR               (M_loadCoffLib | 1)
#define S_loadLib_REALLOC_ERROR                 (M_loadCoffLib | 2)
#define S_loadLib_JMPADDR_ERROR                 (M_loadCoffLib | 3)
#define S_loadLib_NO_REFLO_PAIR                 (M_loadCoffLib | 4)
#define S_loadLib_GPREL_REFERENCE               (M_loadCoffLib | 5)
#define S_loadLib_UNRECOGNIZED_RELOCENTRY       (M_loadCoffLib | 6)
#define S_loadLib_REFHALF_OVFL                  (M_loadCoffLib | 7)
#define S_loadLib_FILE_ENDIAN_ERROR             (M_loadCoffLib | 8)
#define S_loadLib_UNEXPECTED_SYM_CLASS          (M_loadCoffLib | 9)
#define S_loadLib_UNRECOGNIZED_SYM_CLASS        (M_loadCoffLib | 10)
#define S_loadLib_HDR_READ                      (M_loadCoffLib | 11)
#define S_loadLib_OPTHDR_READ                   (M_loadCoffLib | 12)
#define S_loadLib_SCNHDR_READ                   (M_loadCoffLib | 13)
#define S_loadLib_READ_SECTIONS                 (M_loadCoffLib | 14)
#define S_loadLib_LOAD_SECTIONS                 (M_loadCoffLib | 15)
#define S_loadLib_RELOC_READ                    (M_loadCoffLib | 16)
#define S_loadLib_SYMHDR_READ                   (M_loadCoffLib | 17)
#define S_loadLib_EXTSTR_READ                   (M_loadCoffLib | 18)
#define S_loadLib_EXTSYM_READ                   (M_loadCoffLib | 19)
#define S_loadLib_NO_RELOCATION_ROUTINE		(M_loadCoffLib | 20)
#define S_loadEcoffLib_HDR_READ			(M_loadEcoffLib | 1)
#define S_loadEcoffLib_OPTHDR_READ		(M_loadEcoffLib | 2)
#define S_loadEcoffLib_SCNHDR_READ		(M_loadEcoffLib | 3)
#define S_loadEcoffLib_READ_SECTIONS		(M_loadEcoffLib | 4)
#define S_loadEcoffLib_LOAD_SECTIONS		(M_loadEcoffLib | 5)
#define S_loadEcoffLib_RELOC_READ		(M_loadEcoffLib | 6)
#define S_loadEcoffLib_SYMHDR_READ		(M_loadEcoffLib | 7)
#define S_loadEcoffLib_EXTSTR_READ		(M_loadEcoffLib | 8)
#define S_loadEcoffLib_EXTSYM_READ		(M_loadEcoffLib | 9)
#define S_loadEcoffLib_GPREL_REFERENCE		(M_loadEcoffLib | 10)
#define S_loadEcoffLib_JMPADDR_ERROR		(M_loadEcoffLib | 11)
#define S_loadEcoffLib_NO_REFLO_PAIR		(M_loadEcoffLib | 12)
#define S_loadEcoffLib_UNRECOGNIZED_RELOCENTRY	(M_loadEcoffLib | 13)
#define S_loadEcoffLib_REFHALF_OVFL		(M_loadEcoffLib | 14)
#define S_loadEcoffLib_UNEXPECTED_SYM_CLASS	(M_loadEcoffLib | 15)
#define S_loadEcoffLib_UNRECOGNIZED_SYM_CLASS	(M_loadEcoffLib | 16)
#define S_loadEcoffLib_FILE_READ_ERROR		(M_loadEcoffLib | 17)
#define S_loadEcoffLib_FILE_ENDIAN_ERROR	(M_loadEcoffLib | 18)
#define S_loadElfLib_HDR_READ			(M_loadElfLib | 1)
#define S_loadElfLib_HDR_ERROR			(M_loadElfLib | 2)
#define S_loadElfLib_PHDR_MALLOC		(M_loadElfLib | 3)
#define S_loadElfLib_PHDR_READ			(M_loadElfLib | 4)
#define S_loadElfLib_SHDR_MALLOC		(M_loadElfLib | 5)
#define S_loadElfLib_SHDR_READ			(M_loadElfLib | 6)
#define S_loadElfLib_READ_SECTIONS		(M_loadElfLib | 7)
#define S_loadElfLib_LOAD_SECTIONS		(M_loadElfLib | 8)
#define S_loadElfLib_LOAD_PROG			(M_loadElfLib | 9)
#define S_loadElfLib_SYMTAB			(M_loadElfLib | 10)
#define S_loadElfLib_RELA_SECTION		(M_loadElfLib | 11)
#define S_loadElfLib_SCN_READ			(M_loadElfLib | 12)
#define S_loadElfLib_SDA_MALLOC			(M_loadElfLib | 13)
#define S_loadElfLib_SST_READ			(M_loadElfLib | 15)
#define S_loadElfLib_JMPADDR_ERROR		(M_loadElfLib | 20)
#define S_loadElfLib_GPREL_REFERENCE		(M_loadElfLib | 21)
#define S_loadElfLib_UNRECOGNIZED_RELOCENTRY	(M_loadElfLib | 22)
#define S_loadElfLib_RELOC			(M_loadElfLib | 23)
#define S_loadElfLib_UNSUPPORTED		(M_loadElfLib | 24)
#define S_loadElfLib_REL_SECTION		(M_loadElfLib | 25)
#define S_loadLib_ROUTINE_NOT_INSTALLED		(M_loadLib | 1)
#define S_loadLib_TOO_MANY_SYMBOLS		(M_loadLib | 2)
#define S_loadLib_FILE_READ_ERROR               (M_loadPecoffLib | 1)
#define S_loadLib_REALLOC_ERROR                 (M_loadPecoffLib | 2)
#define S_loadLib_JMPADDR_ERROR                 (M_loadPecoffLib | 3)
#define S_loadLib_NO_REFLO_PAIR                 (M_loadPecoffLib | 4)
#define S_loadLib_GPREL_REFERENCE               (M_loadPecoffLib | 5)
#define S_loadLib_UNRECOGNIZED_RELOCENTRY       (M_loadPecoffLib | 6)
#define S_loadLib_REFHALF_OVFL                  (M_loadPecoffLib | 7)
#define S_loadLib_FILE_ENDIAN_ERROR             (M_loadPecoffLib | 8)
#define S_loadLib_UNEXPECTED_SYM_CLASS          (M_loadPecoffLib | 9)
#define S_loadLib_UNRECOGNIZED_SYM_CLASS        (M_loadPecoffLib | 10)
#define S_loadLib_HDR_READ                      (M_loadPecoffLib | 11)
#define S_loadLib_OPTHDR_READ                   (M_loadPecoffLib | 12)
#define S_loadLib_SCNHDR_READ                   (M_loadPecoffLib | 13)
#define S_loadLib_READ_SECTIONS                 (M_loadPecoffLib | 14)
#define S_loadLib_LOAD_SECTIONS                 (M_loadPecoffLib | 15)
#define S_loadLib_RELOC_READ                    (M_loadPecoffLib | 16)
#define S_loadLib_SYMHDR_READ                   (M_loadPecoffLib | 17)
#define S_loadLib_EXTSTR_READ                   (M_loadPecoffLib | 18)
#define S_loadLib_EXTSYM_READ                   (M_loadPecoffLib | 19)
#define S_loadSomCoffLib_HDR_READ		(M_loadSomCoffLib | 1)
#define S_loadSomCoffLib_AUXHDR_READ		(M_loadSomCoffLib | 2)
#define S_loadSomCoffLib_SYM_READ		(M_loadSomCoffLib | 3)
#define S_loadSomCoffLib_SYMSTR_READ		(M_loadSomCoffLib | 4)
#define S_loadSomCoffLib_OBJ_FMT		(M_loadSomCoffLib | 5)
#define S_loadSomCoffLib_SPHDR_ALLOC		(M_loadSomCoffLib | 6)
#define S_loadSomCoffLib_SPHDR_READ		(M_loadSomCoffLib | 7)
#define S_loadSomCoffLib_SUBSPHDR_ALLOC	(M_loadSomCoffLib | 8)
#define S_loadSomCoffLib_SUBSPHDR_READ		(M_loadSomCoffLib | 9)
#define S_loadSomCoffLib_SPSTRING_ALLOC	(M_loadSomCoffLib | 10)
#define S_loadSomCoffLib_SPSTRING_READ		(M_loadSomCoffLib | 11)
#define S_loadSomCoffLib_INFO_ALLOC		(M_loadSomCoffLib | 12)
#define S_loadSomCoffLib_LOAD_SPACE		(M_loadSomCoffLib | 13)
#define S_loadSomCoffLib_RELOC_ALLOC		(M_loadSomCoffLib | 14)
#define S_loadSomCoffLib_RELOC_READ		(M_loadSomCoffLib | 15)
#define S_loadSomCoffLib_RELOC_NEW		(M_loadSomCoffLib | 16)
#define S_loadSomCoffLib_RELOC_VERSION		(M_loadSomCoffLib | 17)
#define S_loginLib_UNKNOWN_USER			(M_loginLib | 1)
#define S_loginLib_USER_ALREADY_EXISTS		(M_loginLib | 2)
#define S_loginLib_INVALID_PASSWORD		(M_loginLib | 3)
#define S_m2Lib_INVALID_PARAMETER               (M_m2Lib | 1)
#define S_m2Lib_ENTRY_NOT_FOUND			(M_m2Lib | 2)
#define S_m2Lib_TCPCONN_FD_NOT_FOUND		(M_m2Lib | 3)
#define S_m2Lib_INVALID_VAR_TO_SET		(M_m2Lib | 4)
#define S_m2Lib_CANT_CREATE_SYS_SEM		(M_m2Lib | 5)
#define S_m2Lib_CANT_CREATE_IF_SEM		(M_m2Lib | 6)
#define S_m2Lib_CANT_CREATE_ROUTE_SEM		(M_m2Lib | 7)
#define S_m2Lib_ARP_PHYSADDR_NOT_SPECIFIED	(M_m2Lib | 8)
#define S_m2Lib_IF_TBL_IS_EMPTY			(M_m2Lib | 9)
#define S_m2Lib_IF_CNFG_CHANGED			(M_m2Lib | 10)
#define S_m2Lib_TOO_BIG                         (M_m2Lib | 11)
#define S_m2Lib_BAD_VALUE                       (M_m2Lib | 12)
#define S_m2Lib_READ_ONLY                       (M_m2Lib | 13)
#define S_m2Lib_GEN_ERR                         (M_m2Lib | 14)
#define S_memLib_NOT_ENOUGH_MEMORY		(M_memLib | 1)
#define S_memLib_INVALID_NBYTES			(M_memLib | 2)
#define S_memLib_BLOCK_ERROR			(M_memLib | 3)
#define S_memLib_NO_PARTITION_DESTROY		(M_memLib | 4)
#define S_memLib_PAGE_SIZE_UNAVAILABLE		(M_memLib | 5)
#define S_miiLib_PHY_LINK_DOWN			(M_miiLib | 1)
#define S_miiLib_PHY_NULL		        (M_miiLib | 2)
#define S_miiLib_PHY_NO_ABLE             	(M_miiLib | 3)
#define S_miiLib_PHY_AN_FAIL             	(M_miiLib | 4)
#define S_moduleLib_MODULE_NOT_FOUND             (M_moduleLib | 1)
#define S_moduleLib_HOOK_NOT_FOUND               (M_moduleLib | 2)
#define S_moduleLib_BAD_CHECKSUM                 (M_moduleLib | 3)
#define S_moduleLib_MAX_MODULES_LOADED           (M_moduleLib | 4)
#define S_mountLib_ILLEGAL_MODE             (M_mountLib | 1)
#define S_msgQLib_INVALID_MSG_LENGTH		(M_msgQLib | 1)
#define S_msgQLib_NON_ZERO_TIMEOUT_AT_INT_LEVEL	(M_msgQLib | 2)
#define S_msgQLib_INVALID_QUEUE_TYPE		(M_msgQLib | 3)
#define S_muxLib_LOAD_FAILED                  (M_muxLib | 1)
#define S_muxLib_NO_DEVICE                    (M_muxLib | 2)
#define S_muxLib_INVALID_ARGS                 (M_muxLib | 3)
#define S_muxLib_ALLOC_FAILED                 (M_muxLib | 4)
#define S_muxLib_ALREADY_BOUND                (M_muxLib | 5)
#define S_muxLib_UNLOAD_FAILED                (M_muxLib | 6)
#define S_muxLib_NOT_A_TK_DEVICE              (M_muxLib | 7)
#define S_muxLib_NO_TK_DEVICE                 (M_muxLib | 8)
#define S_muxLib_END_BIND_FAILED              (M_muxLib | 9)
#define S_netBufLib_MBLK_INVALID	(M_netBufLib | 7)
#define S_netBufLib_NETPOOL_INVALID	(M_netBufLib | 8)
#define S_netBufLib_INVALID_ARGUMENT	(M_netBufLib | 9)
#define S_netBufLib_NO_POOL_MEMORY	(M_netBufLib | 10)
#define S_netDrv_INVALID_NUMBER_OF_BYTES	(M_netDrv | 1)
#define S_netDrv_SEND_ERROR			(M_netDrv | 2)
#define S_netDrv_RECV_ERROR			(M_netDrv | 3)
#define S_netDrv_UNKNOWN_REQUEST		(M_netDrv | 4)
#define S_netDrv_BAD_SEEK			(M_netDrv | 5)
#define S_netDrv_SEEK_PAST_EOF_ERROR		(M_netDrv | 6)
#define S_netDrv_BAD_EOF_POSITION		(M_netDrv | 7)
#define S_netDrv_SEEK_FATAL_ERROR		(M_netDrv | 8)
#define S_netDrv_WRITE_ERROR			(M_netDrv | 12)
#define S_netDrv_NO_SUCH_FILE_OR_DIR		(M_netDrv | 13)
#define S_netDrv_PERMISSION_DENIED		(M_netDrv | 14)
#define S_netDrv_IS_A_DIRECTORY			(M_netDrv | 15)
#define S_netDrv_UNIX_FILE_ERROR		(M_netDrv | 16)
#define S_netDrv_ILLEGAL_VALUE			(M_netDrv | 17)
#define S_nfsDrv_INVALID_NUMBER_OF_BYTES	(M_nfsDrv | 1)
#define S_nfsDrv_BAD_FLAG_MODE			(M_nfsDrv | 2)
#define S_nfsDrv_CREATE_NO_FILE_NAME		(M_nfsDrv | 3)
#define S_nfsLib_NFS_OK			(M_nfsStat | (int) NFS_OK)
#define S_nfsLib_NFSERR_PERM		(M_nfsStat | (int) NFSERR_PERM)
#define S_nfsLib_NFSERR_NOENT		(M_nfsStat | (int) NFSERR_NOENT)
#define S_nfsLib_NFSERR_IO		(M_nfsStat | (int) NFSERR_IO)
#define S_nfsLib_NFSERR_NXIO		(M_nfsStat | (int) NFSERR_NXIO)
#define S_nfsLib_NFSERR_ACCES		(M_nfsStat | (int) NFSERR_ACCES)
#define S_nfsLib_NFSERR_EXIST		(M_nfsStat | (int) NFSERR_EXIST)
#define S_nfsLib_NFSERR_NODEV		(M_nfsStat | (int) NFSERR_NODEV)
#define S_nfsLib_NFSERR_NOTDIR		(M_nfsStat | (int) NFSERR_NOTDIR)
#define S_nfsLib_NFSERR_ISDIR		(M_nfsStat | (int) NFSERR_ISDIR)
#define S_nfsLib_NFSERR_FBIG		(M_nfsStat | (int) NFSERR_FBIG)
#define S_nfsLib_NFSERR_NOSPC		(M_nfsStat | (int) NFSERR_NOSPC)
#define S_nfsLib_NFSERR_ROFS		(M_nfsStat | (int) NFSERR_ROFS)
#define S_nfsLib_NFSERR_NAMETOOLONG	(M_nfsStat | (int) NFSERR_NAMETOOLONG)
#define S_nfsLib_NFSERR_NOTEMPTY	(M_nfsStat | (int) NFSERR_NOTEMPTY)
#define S_nfsLib_NFSERR_DQUOT		(M_nfsStat | (int) NFSERR_DQUOT)
#define S_nfsLib_NFSERR_STALE		(M_nfsStat | (int) NFSERR_STALE)
#define S_nfsLib_NFSERR_WFLUSH		(M_nfsStat | (int) NFSERR_WFLUSH)
#define S_objLib_OBJ_ID_ERROR			(M_objLib | 1)
#define S_objLib_OBJ_UNAVAILABLE		(M_objLib | 2)
#define S_objLib_OBJ_DELETED			(M_objLib | 3)
#define S_objLib_OBJ_TIMEOUT			(M_objLib | 4)
#define S_objLib_OBJ_NO_METHOD			(M_objLib | 5)
#define S_proxyArpLib_INVALID_INTERFACE		(M_proxyArpLib | 2)
#define S_proxyArpLib_INVALID_ADDRESS		(M_proxyArpLib | 5)
#define S_proxyArpLib_TIMEOUT			(M_proxyArpLib | 6)
#define S_qLib_Q_CLASS_ID_ERROR			(M_qLib | 1)
#define S_qPriBMapLib_NULL_BMAP_LIST		(M_qPriBMapLib | 1)
#define S_qPriHeapLib_NULL_HEAP_ARRAY		(M_qPriHeapLib | 1)
#define S_rawFsLib_VOLUME_NOT_AVAILABLE		(M_rawFsLib | 1)
#define S_rawFsLib_END_OF_DEVICE		(M_rawFsLib | 2)
#define S_rawFsLib_NO_FREE_FILE_DESCRIPTORS	(M_rawFsLib | 3)
#define S_rawFsLib_INVALID_NUMBER_OF_BYTES	(M_rawFsLib | 4)
#define S_rawFsLib_ILLEGAL_NAME			(M_rawFsLib | 5)
#define S_rawFsLib_NOT_FILE			(M_rawFsLib | 6)
#define S_rawFsLib_READ_ONLY			(M_rawFsLib | 7)
#define S_rawFsLib_FD_OBSOLETE			(M_rawFsLib | 8)
#define S_rawFsLib_NO_BLOCK_DEVICE		(M_rawFsLib | 9)
#define S_rawFsLib_BAD_SEEK			(M_rawFsLib | 10)
#define S_rawFsLib_INVALID_PARAMETER		(M_rawFsLib | 11)
#define S_rawFsLib_32BIT_OVERFLOW		(M_rawFsLib | 12)
#define S_remLib_ALL_PORTS_IN_USE		(M_remLib | 1)
#define S_remLib_RSH_ERROR			(M_remLib | 2)
#define S_remLib_IDENTITY_TOO_BIG		(M_remLib | 3)
#define S_remLib_RSH_STDERR_SETUP_FAILED	(M_remLib | 4)
#define S_resolvLib_NO_RECOVERY       (M_resolvLib | 1)
#define S_resolvLib_TRY_AGAIN         (M_resolvLib | 2)
#define S_resolvLib_HOST_NOT_FOUND    (M_resolvLib | 3)
#define S_resolvLib_NO_DATA           (M_resolvLib | 4)
#define S_resolvLib_BUFFER_2_SMALL    (M_resolvLib | 5)
#define S_resolvLib_INVALID_PARAMETER (M_resolvLib | 6)
#define S_resolvLib_INVALID_ADDRESS   (M_resolvLib | 7)
#define S_routeLib_ILLEGAL_INTERNET_ADDRESS		(M_routeLib | 1)
#define S_routeLib_ILLEGAL_NETWORK_NUMBER		(M_routeLib | 2)
#define S_rpcLib_RPC_SUCCESS		(M_rpcClntStat | (int) RPC_SUCCESS)
#define S_rpcLib_RPC_CANTENCODEARGS	(M_rpcClntStat | (int) RPC_CANTENCODEARGS)
#define S_rpcLib_RPC_CANTDECODERES	(M_rpcClntStat | (int) RPC_CANTDECODERES)
#define S_rpcLib_RPC_CANTSEND		(M_rpcClntStat | (int) RPC_CANTSEND)
#define S_rpcLib_RPC_CANTRECV		(M_rpcClntStat | (int) RPC_CANTRECV)
#define S_rpcLib_RPC_TIMEDOUT		(M_rpcClntStat | (int) RPC_TIMEDOUT)
#define S_rpcLib_RPC_VERSMISMATCH	(M_rpcClntStat | (int) RPC_VERSMISMATCH)
#define S_rpcLib_RPC_AUTHERROR		(M_rpcClntStat | (int) RPC_AUTHERROR)
#define S_rpcLib_RPC_PROGUNAVAIL	(M_rpcClntStat | (int) RPC_PROGUNAVAIL)
#define S_rpcLib_RPC_PROGVERSMISMATCH	(M_rpcClntStat | (int) RPC_PROGVERSMISMATCH)
#define S_rpcLib_RPC_PROCUNAVAIL	(M_rpcClntStat | (int) RPC_PROCUNAVAIL)
#define S_rpcLib_RPC_CANTDECODEARGS	(M_rpcClntStat | (int) RPC_CANTDECODEARGS)
#define S_rpcLib_RPC_SYSTEMERROR	(M_rpcClntStat | (int) RPC_SYSTEMERROR)
#define S_rpcLib_RPC_UNKNOWNHOST	(M_rpcClntStat | (int) RPC_UNKNOWNHOST)
#define S_rpcLib_RPC_PMAPFAILURE	(M_rpcClntStat | (int) RPC_PMAPFAILURE)
#define S_rpcLib_RPC_PROGNOTREGISTERED	(M_rpcClntStat | (int) RPC_PROGNOTREGISTERED)
#define S_rpcLib_RPC_FAILED		(M_rpcClntStat | (int) RPC_FAILED)
#define S_rt11FsLib_VOLUME_NOT_AVAILABLE		(M_rt11FsLib | 1)
#define S_rt11FsLib_DISK_FULL				(M_rt11FsLib | 2)
#define S_rt11FsLib_FILE_NOT_FOUND			(M_rt11FsLib | 3)
#define S_rt11FsLib_NO_FREE_FILE_DESCRIPTORS		(M_rt11FsLib | 4)
#define S_rt11FsLib_INVALID_NUMBER_OF_BYTES		(M_rt11FsLib | 5)
#define S_rt11FsLib_FILE_ALREADY_EXISTS			(M_rt11FsLib | 6)
#define S_rt11FsLib_BEYOND_FILE_LIMIT			(M_rt11FsLib | 7)
#define S_rt11FsLib_INVALID_DEVICE_PARAMETERS		(M_rt11FsLib | 8)
#define S_rt11FsLib_NO_MORE_FILES_ALLOWED_ON_DISK	(M_rt11FsLib | 9)
#define S_scsiLib_DEV_NOT_READY		(M_scsiLib | 1)
#define S_scsiLib_WRITE_PROTECTED	(M_scsiLib | 2)
#define S_scsiLib_MEDIUM_ERROR		(M_scsiLib | 3)
#define S_scsiLib_HARDWARE_ERROR	(M_scsiLib | 4)
#define S_scsiLib_ILLEGAL_REQUEST	(M_scsiLib | 5)
#define S_scsiLib_BLANK_CHECK		(M_scsiLib | 6)
#define S_scsiLib_ABORTED_COMMAND	(M_scsiLib | 7)
#define S_scsiLib_VOLUME_OVERFLOW	(M_scsiLib | 8)
#define S_scsiLib_UNIT_ATTENTION	(M_scsiLib | 9)
#define S_scsiLib_SELECT_TIMEOUT	(M_scsiLib | 10)
#define S_scsiLib_LUN_NOT_PRESENT	(M_scsiLib | 11)
#define S_scsiLib_ILLEGAL_BUS_ID	(M_scsiLib | 12)
#define S_scsiLib_NO_CONTROLLER		(M_scsiLib | 13)
#define S_scsiLib_REQ_SENSE_ERROR	(M_scsiLib | 14)
#define S_scsiLib_DEV_UNSUPPORTED	(M_scsiLib | 15)
#define S_scsiLib_ILLEGAL_PARAMETER	(M_scsiLib | 16)
#define S_scsiLib_EARLY_PHASE_CHANGE	(M_scsiLib | 17)
#define S_scsiLib_PHASE_CHANGE_TIMEOUT	(M_scsiLib | 18)
#define S_scsiLib_ILLEGAL_OPERATION	(M_scsiLib | 19)
#define S_scsiLib_DEVICE_EXIST          (M_scsiLib | 20)
#define S_scsiLib_SYNC_UNSUPPORTED      (M_scsiLib | 21)
#define S_scsiLib_SYNC_VAL_UNSUPPORTED  (M_scsiLib | 22)
#define S_scsiLib_DEV_NOT_READY		(M_scsiLib | 1)
#define S_scsiLib_WRITE_PROTECTED	(M_scsiLib | 2)
#define S_scsiLib_MEDIUM_ERROR		(M_scsiLib | 3)
#define S_scsiLib_HARDWARE_ERROR	(M_scsiLib | 4)
#define S_scsiLib_ILLEGAL_REQUEST	(M_scsiLib | 5)
#define S_scsiLib_BLANK_CHECK		(M_scsiLib | 6)
#define S_scsiLib_ABORTED_COMMAND	(M_scsiLib | 7)
#define S_scsiLib_VOLUME_OVERFLOW	(M_scsiLib | 8)
#define S_scsiLib_UNIT_ATTENTION	(M_scsiLib | 9)
#define S_scsiLib_SELECT_TIMEOUT	(M_scsiLib | 10)
#define S_scsiLib_LUN_NOT_PRESENT	(M_scsiLib | 11)
#define S_scsiLib_ILLEGAL_BUS_ID	(M_scsiLib | 12)
#define S_scsiLib_NO_CONTROLLER		(M_scsiLib | 13)
#define S_scsiLib_REQ_SENSE_ERROR	(M_scsiLib | 14)
#define S_scsiLib_DEV_UNSUPPORTED	(M_scsiLib | 15)
#define S_scsiLib_ILLEGAL_PARAMETER	(M_scsiLib | 16)
#define S_scsiLib_INVALID_PHASE		(M_scsiLib | 17)
#define S_scsiLib_ABORTED   		(M_scsiLib | 18)
#define S_scsiLib_ILLEGAL_OPERATION	(M_scsiLib | 19)
#define S_scsiLib_DEVICE_EXIST	    	(M_scsiLib | 20)
#define S_scsiLib_DISCONNECTED	    	(M_scsiLib | 21)
#define S_scsiLib_BUS_RESET	    	(M_scsiLib | 22)
#define S_selectLib_NO_SELECT_SUPPORT_IN_DRIVER  (M_selectLib | 1)
#define S_selectLib_NO_SELECT_CONTEXT		 (M_selectLib | 2)
#define S_selectLib_WIDTH_OUT_OF_RANGE		 (M_selectLib | 3)
#define S_semLib_INVALID_STATE			(M_semLib | 101)
#define S_semLib_INVALID_OPTION			(M_semLib | 102)
#define S_semLib_INVALID_QUEUE_TYPE		(M_semLib | 103)
#define S_semLib_INVALID_OPERATION		(M_semLib | 104)
#define S_smLib_MEMORY_ERROR			(M_smLib | 1)
#define S_smLib_INVALID_CPU_NUMBER		(M_smLib | 2)
#define S_smLib_NOT_ATTACHED			(M_smLib | 3)
#define S_smLib_NO_REGIONS			(M_smLib | 4)
#define S_smNameLib_NOT_INITIALIZED           (M_smNameLib | 1)
#define S_smNameLib_NAME_TOO_LONG             (M_smNameLib | 2)
#define S_smNameLib_NAME_NOT_FOUND            (M_smNameLib | 3)
#define S_smNameLib_VALUE_NOT_FOUND           (M_smNameLib | 4)
#define S_smNameLib_NAME_ALREADY_EXIST        (M_smNameLib | 5)
#define S_smNameLib_DATABASE_FULL             (M_smNameLib | 6)
#define S_smNameLib_INVALID_WAIT_TYPE         (M_smNameLib | 7)
#define S_smObjLib_NOT_INITIALIZED        (M_smObjLib | 1)
#define S_smObjLib_NOT_A_GLOBAL_ADRS      (M_smObjLib | 2)
#define S_smObjLib_NOT_A_LOCAL_ADRS       (M_smObjLib | 3)
#define S_smObjLib_SHARED_MEM_TOO_SMALL   (M_smObjLib | 4)
#define S_smObjLib_TOO_MANY_CPU           (M_smObjLib | 5)
#define S_smObjLib_LOCK_TIMEOUT           (M_smObjLib | 6)
#define S_smObjLib_NO_OBJECT_DESTROY      (M_smObjLib | 7)
#define S_smPktLib_SHARED_MEM_TOO_SMALL		(M_smPktLib | 1)
#define S_smPktLib_MEMORY_ERROR			(M_smPktLib | 2)
#define S_smPktLib_DOWN				(M_smPktLib | 3)
#define S_smPktLib_NOT_ATTACHED			(M_smPktLib | 4)
#define S_smPktLib_INVALID_PACKET		(M_smPktLib | 5)
#define S_smPktLib_PACKET_TOO_BIG		(M_smPktLib | 6)
#define S_smPktLib_INVALID_CPU_NUMBER		(M_smPktLib | 7)
#define S_smPktLib_DEST_NOT_ATTACHED		(M_smPktLib | 8)
#define S_smPktLib_INCOMPLETE_BROADCAST		(M_smPktLib | 9)
#define S_smPktLib_LIST_FULL			(M_smPktLib | 10)
#define S_smPktLib_LOCK_TIMEOUT			(M_smPktLib | 11)
#define S_snmpdLib_VIEW_CREATE_FAILURE    (M_snmpdLib | 1)
#define S_snmpdLib_VIEW_INSTALL_FAILURE   (M_snmpdLib | 2)
#define S_snmpdLib_VIEW_MASK_FAILURE      (M_snmpdLib | 3)
#define S_snmpdLib_VIEW_DEINSTALL_FAILURE (M_snmpdLib | 4)
#define S_snmpdLib_VIEW_LOOKUP_FAILURE    (M_snmpdLib | 5)
#define S_snmpdLib_MIB_ADDITION_FAILURE   (M_snmpdLib | 6)
#define S_snmpdLib_NODE_NOT_FOUND         (M_snmpdLib | 7)
#define S_snmpdLib_INVALID_SNMP_VERSION   (M_snmpdLib | 8)
#define S_snmpdLib_TRAP_CREATE_FAILURE    (M_snmpdLib | 9)
#define S_snmpdLib_TRAP_BIND_FAILURE      (M_snmpdLib | 10)
#define S_snmpdLib_TRAP_ENCODE_FAILURE    (M_snmpdLib | 11)
#define S_snmpdLib_INVALID_OID_SYNTAX     (M_snmpdLib | 12)
#define S_sntpcLib_INVALID_PARAMETER         (M_sntpcLib | 1)
#define S_sntpcLib_INVALID_ADDRESS           (M_sntpcLib | 2)
#define S_sntpcLib_TIMEOUT                   (M_sntpcLib | 3)
#define S_sntpcLib_VERSION_UNSUPPORTED       (M_sntpcLib | 4)
#define S_sntpcLib_SERVER_UNSYNC             (M_sntpcLib | 5)
#define S_sntpsLib_INVALID_PARAMETER         (M_sntpsLib | 1)
#define S_sntpsLib_INVALID_ADDRESS           (M_sntpsLib | 2)
#define S_symLib_SYMBOL_NOT_FOUND	(M_symLib | 1)
#define S_symLib_NAME_CLASH		(M_symLib | 2)
#define S_symLib_TABLE_NOT_EMPTY	(M_symLib | 3)
#define S_symLib_SYMBOL_STILL_IN_TABLE	(M_symLib | 4)
#define S_symLib_INVALID_SYMTAB_ID	(M_symLib | 12)
#define S_symLib_INVALID_SYM_ID_PTR     (M_symLib | 13)
#define S_tapeFsLib_NO_SEQ_DEV 			(M_tapeFsLib | 1)
#define S_tapeFsLib_ILLEGAL_TAPE_CONFIG_PARM 	(M_tapeFsLib | 2)
#define S_tapeFsLib_SERVICE_NOT_AVAILABLE	(M_tapeFsLib | 3)
#define S_tapeFsLib_INVALID_BLOCK_SIZE		(M_tapeFsLib | 4)
#define S_tapeFsLib_ILLEGAL_FILE_SYSTEM_NAME	(M_tapeFsLib | 5)
#define S_tapeFsLib_ILLEGAL_FLAGS		(M_tapeFsLib | 6)
#define S_tapeFsLib_FILE_DESCRIPTOR_BUSY	(M_tapeFsLib | 7)
#define S_tapeFsLib_VOLUME_NOT_AVAILABLE	(M_tapeFsLib | 8)
#define S_tapeFsLib_BLOCK_SIZE_MISMATCH		(M_tapeFsLib | 9)
#define S_tapeFsLib_INVALID_NUMBER_OF_BYTES	(M_tapeFsLib | 10)
#define S_taskLib_NAME_NOT_FOUND		(M_taskLib | 101)
#define S_taskLib_TASK_HOOK_TABLE_FULL		(M_taskLib | 102)
#define S_taskLib_TASK_HOOK_NOT_FOUND		(M_taskLib | 103)
#define S_taskLib_TASK_SWAP_HOOK_REFERENCED	(M_taskLib | 104)
#define S_taskLib_TASK_SWAP_HOOK_SET		(M_taskLib | 105)
#define S_taskLib_TASK_SWAP_HOOK_CLEAR		(M_taskLib | 106)
#define S_taskLib_TASK_VAR_NOT_FOUND		(M_taskLib | 107)
#define S_taskLib_TASK_UNDELAYED		(M_taskLib | 108)
#define S_taskLib_ILLEGAL_PRIORITY		(M_taskLib | 109)
#define S_tftpLib_INVALID_ARGUMENT	(M_tftpLib | 1)
#define S_tftpLib_INVALID_DESCRIPTOR	(M_tftpLib | 2)
#define S_tftpLib_INVALID_COMMAND	(M_tftpLib | 3)
#define S_tftpLib_INVALID_MODE		(M_tftpLib | 4)
#define S_tftpLib_UNKNOWN_HOST		(M_tftpLib | 5)
#define S_tftpLib_NOT_CONNECTED		(M_tftpLib | 6)
#define S_tftpLib_TIMED_OUT		(M_tftpLib | 7)
#define S_tftpLib_TFTP_ERROR		(M_tftpLib | 8)
#define S_unldLib_MODULE_NOT_FOUND             (M_unldLib | 1)
#define S_unldLib_TEXT_IN_USE                  (M_unldLib | 2)
#define S_usrLib_NOT_ENOUGH_ARGS	(M_usrLib | 1)
#define S_vmLib_NOT_PAGE_ALIGNED		(M_vmLib | 1)
#define S_vmLib_BAD_STATE_PARAM			(M_vmLib | 2)
#define S_vmLib_BAD_MASK_PARAM			(M_vmLib | 3)
#define S_vmLib_ADDR_IN_GLOBAL_SPACE		(M_vmLib | 4)
#define S_vmLib_TEXT_PROTECTION_UNAVAILABLE	(M_vmLib | 5)
#define S_vmLib_NO_FREE_REGIONS			(M_vmLib | 6)
#define S_vmLib_ADDRS_NOT_EQUAL			(M_vmLib | 7)

SYMBOL statTbl [] =
{
	{{NULL},"S_arpLib_INVALID_ARGUMENT", (char *) S_arpLib_INVALID_ARGUMENT,0},
	{{NULL},"S_arpLib_INVALID_HOST", (char *) S_arpLib_INVALID_HOST,0},
	{{NULL},"S_arpLib_INVALID_ENET_ADDRESS", (char *) S_arpLib_INVALID_ENET_ADDRESS,0},
	{{NULL},"S_arpLib_INVALID_FLAG", (char *) S_arpLib_INVALID_FLAG,0},
	{{NULL},"S_bootLoadLib_ROUTINE_NOT_INSTALLED", (char *) S_bootLoadLib_ROUTINE_NOT_INSTALLED,0},
	{{NULL},"S_bootpLib_INVALID_ARGUMENT", (char *) S_bootpLib_INVALID_ARGUMENT,0},
	{{NULL},"S_bootpLib_INVALID_COOKIE", (char *) S_bootpLib_INVALID_COOKIE,0},
	{{NULL},"S_bootpLib_NO_BROADCASTS", (char *) S_bootpLib_NO_BROADCASTS,0},
	{{NULL},"S_bootpLib_PARSE_ERROR", (char *) S_bootpLib_PARSE_ERROR,0},
	{{NULL},"S_bootpLib_INVALID_TAG", (char *) S_bootpLib_INVALID_TAG,0},
	{{NULL},"S_bootpLib_TIME_OUT", (char *) S_bootpLib_TIME_OUT,0},
	{{NULL},"S_bootpLib_MEM_ERROR", (char *) S_bootpLib_MEM_ERROR,0},
	{{NULL},"S_bootpLib_NOT_INITIALIZED", (char *) S_bootpLib_NOT_INITIALIZED,0},
	{{NULL},"S_bootpLib_BAD_DEVICE", (char *) S_bootpLib_BAD_DEVICE,0},
	{{NULL},"S_cacheLib_INVALID_CACHE", (char *) S_cacheLib_INVALID_CACHE,0},
	{{NULL},"S_cbioLib_INVALID_CBIO_DEV_ID", (char *) S_cbioLib_INVALID_CBIO_DEV_ID,0},
	{{NULL},"S_cdromFsLib_ALREADY_INIT", (char *) S_cdromFsLib_ALREADY_INIT,0},
	{{NULL},"S_cdromFsLib_DEVICE_REMOVED", (char *) S_cdromFsLib_DEVICE_REMOVED,0},
	{{NULL},"S_cdromFsLib_SUCH_PATH_TABLE_SIZE_NOT_SUPPORTED", (char *) S_cdromFsLib_SUCH_PATH_TABLE_SIZE_NOT_SUPPORTED,0},
	{{NULL},"S_cdromFsLib_ONE_OF_VALUES_NOT_POWER_OF_2", (char *) S_cdromFsLib_ONE_OF_VALUES_NOT_POWER_OF_2,0},
	{{NULL},"S_cdromFsLib_UNNOWN_FILE_SYSTEM", (char *) S_cdromFsLib_UNNOWN_FILE_SYSTEM,0},
	{{NULL},"S_cdromFsLib_INVAL_VOL_DESCR", (char *) S_cdromFsLib_INVAL_VOL_DESCR,0},
	{{NULL},"S_cdromFsLib_INVALID_PATH_STRING", (char *) S_cdromFsLib_INVALID_PATH_STRING,0},
	{{NULL},"S_cdromFsLib_MAX_DIR_HIERARCHY_LEVEL_OVERFLOW", (char *) S_cdromFsLib_MAX_DIR_HIERARCHY_LEVEL_OVERFLOW,0},
	{{NULL},"S_cdromFsLib_NO_SUCH_FILE_OR_DIRECTORY", (char *) S_cdromFsLib_NO_SUCH_FILE_OR_DIRECTORY,0},
	{{NULL},"S_cdromFsLib_INVALID_DIRECTORY_STRUCTURE", (char *) S_cdromFsLib_INVALID_DIRECTORY_STRUCTURE,0},
	{{NULL},"S_cdromFsLib_INVALID_FILE_DESCRIPTOR", (char *) S_cdromFsLib_INVALID_FILE_DESCRIPTOR,0},
	{{NULL},"S_cdromFsLib_READ_ONLY_DEVICE", (char *) S_cdromFsLib_READ_ONLY_DEVICE,0},
	{{NULL},"S_cdromFsLib_END_OF_FILE", (char *) S_cdromFsLib_END_OF_FILE,0},
	{{NULL},"S_cdromFsLib_INV_ARG_VALUE", (char *) S_cdromFsLib_INV_ARG_VALUE,0},
	{{NULL},"S_cdromFsLib_SEMTAKE_ERROR", (char *) S_cdromFsLib_SEMTAKE_ERROR,0},
	{{NULL},"S_cdromFsLib_SEMGIVE_ERROR", (char *) S_cdromFsLib_SEMGIVE_ERROR,0},
	{{NULL},"S_cdromFsLib_VOL_UNMOUNTED", (char *) S_cdromFsLib_VOL_UNMOUNTED,0},
	{{NULL},"S_cdromFsLib_INVAL_DIR_OPER", (char *) S_cdromFsLib_INVAL_DIR_OPER,0},
	{{NULL},"S_cdromFsLib_READING_FAILURE", (char *) S_cdromFsLib_READING_FAILURE,0},
	{{NULL},"S_cdromFsLib_INVALID_DIR_REC_STRUCT", (char *) S_cdromFsLib_INVALID_DIR_REC_STRUCT,0},
	{{NULL},"S_classLib_CLASS_ID_ERROR", (char *) S_classLib_CLASS_ID_ERROR,0},
	{{NULL},"S_classLib_NO_CLASS_DESTROY", (char *) S_classLib_NO_CLASS_DESTROY,0},
	{{NULL},"S_dhcpsLib_NOT_INITIALIZED", (char *) S_dhcpsLib_NOT_INITIALIZED,0},
	{{NULL},"S_dosFsLib_32BIT_OVERFLOW", (char *) S_dosFsLib_32BIT_OVERFLOW,0},
	{{NULL},"S_dosFsLib_DISK_FULL", (char *) S_dosFsLib_DISK_FULL,0},
	{{NULL},"S_dosFsLib_FILE_NOT_FOUND", (char *) S_dosFsLib_FILE_NOT_FOUND,0},
	{{NULL},"S_dosFsLib_NO_FREE_FILE_DESCRIPTORS", (char *) S_dosFsLib_NO_FREE_FILE_DESCRIPTORS,0},
	{{NULL},"S_dosFsLib_NOT_FILE", (char *) S_dosFsLib_NOT_FILE,0},
	{{NULL},"S_dosFsLib_NOT_SAME_VOLUME", (char *) S_dosFsLib_NOT_SAME_VOLUME,0},
	{{NULL},"S_dosFsLib_NOT_DIRECTORY", (char *) S_dosFsLib_NOT_DIRECTORY,0},
	{{NULL},"S_dosFsLib_DIR_NOT_EMPTY", (char *) S_dosFsLib_DIR_NOT_EMPTY,0},
	{{NULL},"S_dosFsLib_FILE_EXISTS", (char *) S_dosFsLib_FILE_EXISTS,0},
	{{NULL},"S_dosFsLib_INVALID_PARAMETER", (char *) S_dosFsLib_INVALID_PARAMETER,0},
	{{NULL},"S_dosFsLib_NAME_TOO_LONG", (char *) S_dosFsLib_NAME_TOO_LONG,0},
	{{NULL},"S_dosFsLib_UNSUPPORTED", (char *) S_dosFsLib_UNSUPPORTED,0},
	{{NULL},"S_dosFsLib_VOLUME_NOT_AVAILABLE", (char *) S_dosFsLib_VOLUME_NOT_AVAILABLE,0},
	{{NULL},"S_dosFsLib_INVALID_NUMBER_OF_BYTES", (char *) S_dosFsLib_INVALID_NUMBER_OF_BYTES,0},
	{{NULL},"S_dosFsLib_ILLEGAL_NAME", (char *) S_dosFsLib_ILLEGAL_NAME,0},
	{{NULL},"S_dosFsLib_CANT_DEL_ROOT", (char *) S_dosFsLib_CANT_DEL_ROOT,0},
	{{NULL},"S_dosFsLib_READ_ONLY", (char *) S_dosFsLib_READ_ONLY,0},
	{{NULL},"S_dosFsLib_ROOT_DIR_FULL", (char *) S_dosFsLib_ROOT_DIR_FULL,0},
	{{NULL},"S_dosFsLib_NO_LABEL", (char *) S_dosFsLib_NO_LABEL,0},
	{{NULL},"S_dosFsLib_NO_CONTIG_SPACE", (char *) S_dosFsLib_NO_CONTIG_SPACE,0},
	{{NULL},"S_dosFsLib_FD_OBSOLETE", (char *) S_dosFsLib_FD_OBSOLETE,0},
	{{NULL},"S_dosFsLib_DELETED", (char *) S_dosFsLib_DELETED,0},
	{{NULL},"S_dosFsLib_INTERNAL_ERROR", (char *) S_dosFsLib_INTERNAL_ERROR,0},
	{{NULL},"S_dosFsLib_WRITE_ONLY", (char *) S_dosFsLib_WRITE_ONLY,0},
	{{NULL},"S_dosFsLib_ILLEGAL_PATH", (char *) S_dosFsLib_ILLEGAL_PATH,0},
	{{NULL},"S_dosFsLib_ACCESS_BEYOND_EOF", (char *) S_dosFsLib_ACCESS_BEYOND_EOF,0},
	{{NULL},"S_dosFsLib_DIR_READ_ONLY", (char *) S_dosFsLib_DIR_READ_ONLY,0},
	{{NULL},"S_dosFsLib_UNKNOWN_VOLUME_FORMAT", (char *) S_dosFsLib_UNKNOWN_VOLUME_FORMAT,0},
	{{NULL},"S_dosFsLib_ILLEGAL_CLUSTER_NUMBER", (char *) S_dosFsLib_ILLEGAL_CLUSTER_NUMBER,0},
	{{NULL},"S_errnoLib_NO_STAT_SYM_TBL", (char *) S_errnoLib_NO_STAT_SYM_TBL,0},
	{{NULL},"S_eventLib_TIMEOUT", (char *) S_eventLib_TIMEOUT,0},
	{{NULL},"S_eventLib_NOT_ALL_EVENTS", (char *) S_eventLib_NOT_ALL_EVENTS,0},
	{{NULL},"S_eventLib_ALREADY_REGISTERED", (char *) S_eventLib_ALREADY_REGISTERED,0},
	{{NULL},"S_eventLib_EVENTSEND_FAILED", (char *) S_eventLib_EVENTSEND_FAILED,0},
	{{NULL},"S_eventLib_ZERO_EVENTS", (char *) S_eventLib_ZERO_EVENTS,0},
	{{NULL},"S_eventLib_TASK_NOT_REGISTERED", (char *) S_eventLib_TASK_NOT_REGISTERED,0},
	{{NULL},"S_eventLib_NULL_TASKID_AT_INT_LEVEL", (char *) S_eventLib_NULL_TASKID_AT_INT_LEVEL,0},
	{{NULL},"S_ftpLib_ILLEGAL_VALUE", (char *) S_ftpLib_ILLEGAL_VALUE,0},
	{{NULL},"S_ftpLib_TRANSIENT_RETRY_LIMIT_EXCEEDED", (char *) S_ftpLib_TRANSIENT_RETRY_LIMIT_EXCEEDED,0},
	{{NULL},"S_ftpLib_FATAL_TRANSIENT_RESPONSE", (char *) S_ftpLib_FATAL_TRANSIENT_RESPONSE,0},
	{{NULL},"S_ftpLib_REMOTE_SERVER_STATUS_221", (char *) S_ftpLib_REMOTE_SERVER_STATUS_221,0},
	{{NULL},"S_ftpLib_REMOTE_SERVER_STATUS_226", (char *) S_ftpLib_REMOTE_SERVER_STATUS_226,0},
	{{NULL},"S_ftpLib_REMOTE_SERVER_STATUS_257", (char *) S_ftpLib_REMOTE_SERVER_STATUS_257,0},
	{{NULL},"S_ftpLib_REMOTE_SERVER_ERROR_422", (char *) S_ftpLib_REMOTE_SERVER_ERROR_422,0},
	{{NULL},"S_ftpLib_REMOTE_SERVER_ERROR_425", (char *) S_ftpLib_REMOTE_SERVER_ERROR_425,0},
	{{NULL},"S_ftpLib_REMOTE_SERVER_ERROR_450", (char *) S_ftpLib_REMOTE_SERVER_ERROR_450,0},
	{{NULL},"S_ftpLib_REMOTE_SERVER_ERROR_451", (char *) S_ftpLib_REMOTE_SERVER_ERROR_451,0},
	{{NULL},"S_ftpLib_REMOTE_SERVER_ERROR_452", (char *) S_ftpLib_REMOTE_SERVER_ERROR_452,0},
	{{NULL},"S_ftpLib_REMOTE_SERVER_ERROR_500", (char *) S_ftpLib_REMOTE_SERVER_ERROR_500,0},
	{{NULL},"S_ftpLib_REMOTE_SERVER_ERROR_501", (char *) S_ftpLib_REMOTE_SERVER_ERROR_501,0},
	{{NULL},"S_ftpLib_REMOTE_SERVER_ERROR_502", (char *) S_ftpLib_REMOTE_SERVER_ERROR_502,0},
	{{NULL},"S_ftpLib_REMOTE_SERVER_ERROR_503", (char *) S_ftpLib_REMOTE_SERVER_ERROR_503,0},
	{{NULL},"S_ftpLib_REMOTE_SERVER_ERROR_504", (char *) S_ftpLib_REMOTE_SERVER_ERROR_504,0},
	{{NULL},"S_ftpLib_REMOTE_SERVER_ERROR_520", (char *) S_ftpLib_REMOTE_SERVER_ERROR_520,0},
	{{NULL},"S_ftpLib_REMOTE_SERVER_ERROR_521", (char *) S_ftpLib_REMOTE_SERVER_ERROR_521,0},
	{{NULL},"S_ftpLib_REMOTE_SERVER_ERROR_530", (char *) S_ftpLib_REMOTE_SERVER_ERROR_530,0},
	{{NULL},"S_ftpLib_REMOTE_SERVER_ERROR_550", (char *) S_ftpLib_REMOTE_SERVER_ERROR_550,0},
	{{NULL},"S_ftpLib_REMOTE_SERVER_ERROR_551", (char *) S_ftpLib_REMOTE_SERVER_ERROR_551,0},
	{{NULL},"S_ftpLib_REMOTE_SERVER_ERROR_552", (char *) S_ftpLib_REMOTE_SERVER_ERROR_552,0},
	{{NULL},"S_ftpLib_REMOTE_SERVER_ERROR_553", (char *) S_ftpLib_REMOTE_SERVER_ERROR_553,0},
	{{NULL},"S_ftpLib_REMOTE_SERVER_ERROR_554", (char *) S_ftpLib_REMOTE_SERVER_ERROR_554,0},
	{{NULL},"S_hashLib_KEY_CLASH", (char *) S_hashLib_KEY_CLASH,0},
	{{NULL},"S_hostLib_UNKNOWN_HOST", (char *) S_hostLib_UNKNOWN_HOST,0},
	{{NULL},"S_hostLib_HOST_ALREADY_ENTERED", (char *) S_hostLib_HOST_ALREADY_ENTERED,0},
	{{NULL},"S_hostLib_INVALID_PARAMETER", (char *) S_hostLib_INVALID_PARAMETER,0},
	{{NULL},"S_icmpLib_TIMEOUT", (char *) S_icmpLib_TIMEOUT,0},
	{{NULL},"S_icmpLib_NO_BROADCAST", (char *) S_icmpLib_NO_BROADCAST,0},
	{{NULL},"S_icmpLib_INVALID_INTERFACE", (char *) S_icmpLib_INVALID_INTERFACE,0},
	{{NULL},"S_icmpLib_INVALID_ARGUMENT", (char *) S_icmpLib_INVALID_ARGUMENT,0},
	{{NULL},"S_if_sl_INVALID_UNIT_NUMBER", (char *) S_if_sl_INVALID_UNIT_NUMBER,0},
	{{NULL},"S_if_sl_UNIT_UNINITIALIZED", (char *) S_if_sl_UNIT_UNINITIALIZED,0},
	{{NULL},"S_if_sl_UNIT_ALREADY_INITIALIZED", (char *) S_if_sl_UNIT_ALREADY_INITIALIZED,0},
	{{NULL},"S_inetLib_ILLEGAL_INTERNET_ADDRESS", (char *) S_inetLib_ILLEGAL_INTERNET_ADDRESS,0},
	{{NULL},"S_inetLib_ILLEGAL_NETWORK_NUMBER", (char *) S_inetLib_ILLEGAL_NETWORK_NUMBER,0},
	{{NULL},"S_intLib_NOT_ISR_CALLABLE", (char *) S_intLib_NOT_ISR_CALLABLE,0},
	{{NULL},"S_intLib_VEC_TABLE_WP_UNAVAILABLE", (char *) S_intLib_VEC_TABLE_WP_UNAVAILABLE,0},
	{{NULL},"S_ioLib_NO_DRIVER", (char *) S_ioLib_NO_DRIVER,0},
	{{NULL},"S_ioLib_UNKNOWN_REQUEST", (char *) S_ioLib_UNKNOWN_REQUEST,0},
	{{NULL},"S_ioLib_DEVICE_ERROR", (char *) S_ioLib_DEVICE_ERROR,0},
	{{NULL},"S_ioLib_DEVICE_TIMEOUT", (char *) S_ioLib_DEVICE_TIMEOUT,0},
	{{NULL},"S_ioLib_WRITE_PROTECTED", (char *) S_ioLib_WRITE_PROTECTED,0},
	{{NULL},"S_ioLib_DISK_NOT_PRESENT", (char *) S_ioLib_DISK_NOT_PRESENT,0},
	{{NULL},"S_ioLib_NO_FILENAME", (char *) S_ioLib_NO_FILENAME,0},
	{{NULL},"S_ioLib_CANCELLED", (char *) S_ioLib_CANCELLED,0},
	{{NULL},"S_ioLib_NO_DEVICE_NAME_IN_PATH", (char *) S_ioLib_NO_DEVICE_NAME_IN_PATH,0},
	{{NULL},"S_ioLib_NAME_TOO_LONG", (char *) S_ioLib_NAME_TOO_LONG,0},
	{{NULL},"S_ioLib_UNFORMATED", (char *) S_ioLib_UNFORMATED,0},
	{{NULL},"S_ioLib_CANT_OVERWRITE_DIR", (char *) S_ioLib_CANT_OVERWRITE_DIR,0},
	{{NULL},"S_iosLib_DEVICE_NOT_FOUND", (char *) S_iosLib_DEVICE_NOT_FOUND,0},
	{{NULL},"S_iosLib_DRIVER_GLUT", (char *) S_iosLib_DRIVER_GLUT,0},
	{{NULL},"S_iosLib_INVALID_FILE_DESCRIPTOR", (char *) S_iosLib_INVALID_FILE_DESCRIPTOR,0},
	{{NULL},"S_iosLib_TOO_MANY_OPEN_FILES", (char *) S_iosLib_TOO_MANY_OPEN_FILES,0},
	{{NULL},"S_iosLib_CONTROLLER_NOT_PRESENT", (char *) S_iosLib_CONTROLLER_NOT_PRESENT,0},
	{{NULL},"S_iosLib_DUPLICATE_DEVICE_NAME", (char *) S_iosLib_DUPLICATE_DEVICE_NAME,0},
	{{NULL},"S_iosLib_INVALID_ETHERNET_ADDRESS", (char *) S_iosLib_INVALID_ETHERNET_ADDRESS,0},
	{{NULL},"S_loadAoutLib_TOO_MANY_SYMBOLS", (char *) S_loadAoutLib_TOO_MANY_SYMBOLS,0},
	{{NULL},"S_loadLib_FILE_READ_ERROR", (char *) S_loadLib_FILE_READ_ERROR,0},
	{{NULL},"S_loadLib_REALLOC_ERROR", (char *) S_loadLib_REALLOC_ERROR,0},
	{{NULL},"S_loadLib_JMPADDR_ERROR", (char *) S_loadLib_JMPADDR_ERROR,0},
	{{NULL},"S_loadLib_NO_REFLO_PAIR", (char *) S_loadLib_NO_REFLO_PAIR,0},
	{{NULL},"S_loadLib_GPREL_REFERENCE", (char *) S_loadLib_GPREL_REFERENCE,0},
	{{NULL},"S_loadLib_UNRECOGNIZED_RELOCENTRY", (char *) S_loadLib_UNRECOGNIZED_RELOCENTRY,0},
	{{NULL},"S_loadLib_REFHALF_OVFL", (char *) S_loadLib_REFHALF_OVFL,0},
	{{NULL},"S_loadLib_FILE_ENDIAN_ERROR", (char *) S_loadLib_FILE_ENDIAN_ERROR,0},
	{{NULL},"S_loadLib_UNEXPECTED_SYM_CLASS", (char *) S_loadLib_UNEXPECTED_SYM_CLASS,0},
	{{NULL},"S_loadLib_UNRECOGNIZED_SYM_CLASS", (char *) S_loadLib_UNRECOGNIZED_SYM_CLASS,0},
	{{NULL},"S_loadLib_HDR_READ", (char *) S_loadLib_HDR_READ,0},
	{{NULL},"S_loadLib_OPTHDR_READ", (char *) S_loadLib_OPTHDR_READ,0},
	{{NULL},"S_loadLib_SCNHDR_READ", (char *) S_loadLib_SCNHDR_READ,0},
	{{NULL},"S_loadLib_READ_SECTIONS", (char *) S_loadLib_READ_SECTIONS,0},
	{{NULL},"S_loadLib_LOAD_SECTIONS", (char *) S_loadLib_LOAD_SECTIONS,0},
	{{NULL},"S_loadLib_RELOC_READ", (char *) S_loadLib_RELOC_READ,0},
	{{NULL},"S_loadLib_SYMHDR_READ", (char *) S_loadLib_SYMHDR_READ,0},
	{{NULL},"S_loadLib_EXTSTR_READ", (char *) S_loadLib_EXTSTR_READ,0},
	{{NULL},"S_loadLib_EXTSYM_READ", (char *) S_loadLib_EXTSYM_READ,0},
	{{NULL},"S_loadLib_NO_RELOCATION_ROUTINE", (char *) S_loadLib_NO_RELOCATION_ROUTINE,0},
	{{NULL},"S_loadEcoffLib_HDR_READ", (char *) S_loadEcoffLib_HDR_READ,0},
	{{NULL},"S_loadEcoffLib_OPTHDR_READ", (char *) S_loadEcoffLib_OPTHDR_READ,0},
	{{NULL},"S_loadEcoffLib_SCNHDR_READ", (char *) S_loadEcoffLib_SCNHDR_READ,0},
	{{NULL},"S_loadEcoffLib_READ_SECTIONS", (char *) S_loadEcoffLib_READ_SECTIONS,0},
	{{NULL},"S_loadEcoffLib_LOAD_SECTIONS", (char *) S_loadEcoffLib_LOAD_SECTIONS,0},
	{{NULL},"S_loadEcoffLib_RELOC_READ", (char *) S_loadEcoffLib_RELOC_READ,0},
	{{NULL},"S_loadEcoffLib_SYMHDR_READ", (char *) S_loadEcoffLib_SYMHDR_READ,0},
	{{NULL},"S_loadEcoffLib_EXTSTR_READ", (char *) S_loadEcoffLib_EXTSTR_READ,0},
	{{NULL},"S_loadEcoffLib_EXTSYM_READ", (char *) S_loadEcoffLib_EXTSYM_READ,0},
	{{NULL},"S_loadEcoffLib_GPREL_REFERENCE", (char *) S_loadEcoffLib_GPREL_REFERENCE,0},
	{{NULL},"S_loadEcoffLib_JMPADDR_ERROR", (char *) S_loadEcoffLib_JMPADDR_ERROR,0},
	{{NULL},"S_loadEcoffLib_NO_REFLO_PAIR", (char *) S_loadEcoffLib_NO_REFLO_PAIR,0},
	{{NULL},"S_loadEcoffLib_UNRECOGNIZED_RELOCENTRY", (char *) S_loadEcoffLib_UNRECOGNIZED_RELOCENTRY,0},
	{{NULL},"S_loadEcoffLib_REFHALF_OVFL", (char *) S_loadEcoffLib_REFHALF_OVFL,0},
	{{NULL},"S_loadEcoffLib_UNEXPECTED_SYM_CLASS", (char *) S_loadEcoffLib_UNEXPECTED_SYM_CLASS,0},
	{{NULL},"S_loadEcoffLib_UNRECOGNIZED_SYM_CLASS", (char *) S_loadEcoffLib_UNRECOGNIZED_SYM_CLASS,0},
	{{NULL},"S_loadEcoffLib_FILE_READ_ERROR", (char *) S_loadEcoffLib_FILE_READ_ERROR,0},
	{{NULL},"S_loadEcoffLib_FILE_ENDIAN_ERROR", (char *) S_loadEcoffLib_FILE_ENDIAN_ERROR,0},
	{{NULL},"S_loadElfLib_HDR_READ", (char *) S_loadElfLib_HDR_READ,0},
	{{NULL},"S_loadElfLib_HDR_ERROR", (char *) S_loadElfLib_HDR_ERROR,0},
	{{NULL},"S_loadElfLib_PHDR_MALLOC", (char *) S_loadElfLib_PHDR_MALLOC,0},
	{{NULL},"S_loadElfLib_PHDR_READ", (char *) S_loadElfLib_PHDR_READ,0},
	{{NULL},"S_loadElfLib_SHDR_MALLOC", (char *) S_loadElfLib_SHDR_MALLOC,0},
	{{NULL},"S_loadElfLib_SHDR_READ", (char *) S_loadElfLib_SHDR_READ,0},
	{{NULL},"S_loadElfLib_READ_SECTIONS", (char *) S_loadElfLib_READ_SECTIONS,0},
	{{NULL},"S_loadElfLib_LOAD_SECTIONS", (char *) S_loadElfLib_LOAD_SECTIONS,0},
	{{NULL},"S_loadElfLib_LOAD_PROG", (char *) S_loadElfLib_LOAD_PROG,0},
	{{NULL},"S_loadElfLib_SYMTAB", (char *) S_loadElfLib_SYMTAB,0},
	{{NULL},"S_loadElfLib_RELA_SECTION", (char *) S_loadElfLib_RELA_SECTION,0},
	{{NULL},"S_loadElfLib_SCN_READ", (char *) S_loadElfLib_SCN_READ,0},
	{{NULL},"S_loadElfLib_SDA_MALLOC", (char *) S_loadElfLib_SDA_MALLOC,0},
	{{NULL},"S_loadElfLib_SST_READ", (char *) S_loadElfLib_SST_READ,0},
	{{NULL},"S_loadElfLib_JMPADDR_ERROR", (char *) S_loadElfLib_JMPADDR_ERROR,0},
	{{NULL},"S_loadElfLib_GPREL_REFERENCE", (char *) S_loadElfLib_GPREL_REFERENCE,0},
	{{NULL},"S_loadElfLib_UNRECOGNIZED_RELOCENTRY", (char *) S_loadElfLib_UNRECOGNIZED_RELOCENTRY,0},
	{{NULL},"S_loadElfLib_RELOC", (char *) S_loadElfLib_RELOC,0},
	{{NULL},"S_loadElfLib_UNSUPPORTED", (char *) S_loadElfLib_UNSUPPORTED,0},
	{{NULL},"S_loadElfLib_REL_SECTION", (char *) S_loadElfLib_REL_SECTION,0},
	{{NULL},"S_loadLib_ROUTINE_NOT_INSTALLED", (char *) S_loadLib_ROUTINE_NOT_INSTALLED,0},
	{{NULL},"S_loadLib_TOO_MANY_SYMBOLS", (char *) S_loadLib_TOO_MANY_SYMBOLS,0},
	{{NULL},"S_loadLib_FILE_READ_ERROR", (char *) S_loadLib_FILE_READ_ERROR,0},
	{{NULL},"S_loadLib_REALLOC_ERROR", (char *) S_loadLib_REALLOC_ERROR,0},
	{{NULL},"S_loadLib_JMPADDR_ERROR", (char *) S_loadLib_JMPADDR_ERROR,0},
	{{NULL},"S_loadLib_NO_REFLO_PAIR", (char *) S_loadLib_NO_REFLO_PAIR,0},
	{{NULL},"S_loadLib_GPREL_REFERENCE", (char *) S_loadLib_GPREL_REFERENCE,0},
	{{NULL},"S_loadLib_UNRECOGNIZED_RELOCENTRY", (char *) S_loadLib_UNRECOGNIZED_RELOCENTRY,0},
	{{NULL},"S_loadLib_REFHALF_OVFL", (char *) S_loadLib_REFHALF_OVFL,0},
	{{NULL},"S_loadLib_FILE_ENDIAN_ERROR", (char *) S_loadLib_FILE_ENDIAN_ERROR,0},
	{{NULL},"S_loadLib_UNEXPECTED_SYM_CLASS", (char *) S_loadLib_UNEXPECTED_SYM_CLASS,0},
	{{NULL},"S_loadLib_UNRECOGNIZED_SYM_CLASS", (char *) S_loadLib_UNRECOGNIZED_SYM_CLASS,0},
	{{NULL},"S_loadLib_HDR_READ", (char *) S_loadLib_HDR_READ,0},
	{{NULL},"S_loadLib_OPTHDR_READ", (char *) S_loadLib_OPTHDR_READ,0},
	{{NULL},"S_loadLib_SCNHDR_READ", (char *) S_loadLib_SCNHDR_READ,0},
	{{NULL},"S_loadLib_READ_SECTIONS", (char *) S_loadLib_READ_SECTIONS,0},
	{{NULL},"S_loadLib_LOAD_SECTIONS", (char *) S_loadLib_LOAD_SECTIONS,0},
	{{NULL},"S_loadLib_RELOC_READ", (char *) S_loadLib_RELOC_READ,0},
	{{NULL},"S_loadLib_SYMHDR_READ", (char *) S_loadLib_SYMHDR_READ,0},
	{{NULL},"S_loadLib_EXTSTR_READ", (char *) S_loadLib_EXTSTR_READ,0},
	{{NULL},"S_loadLib_EXTSYM_READ", (char *) S_loadLib_EXTSYM_READ,0},
	{{NULL},"S_loadSomCoffLib_HDR_READ", (char *) S_loadSomCoffLib_HDR_READ,0},
	{{NULL},"S_loadSomCoffLib_AUXHDR_READ", (char *) S_loadSomCoffLib_AUXHDR_READ,0},
	{{NULL},"S_loadSomCoffLib_SYM_READ", (char *) S_loadSomCoffLib_SYM_READ,0},
	{{NULL},"S_loadSomCoffLib_SYMSTR_READ", (char *) S_loadSomCoffLib_SYMSTR_READ,0},
	{{NULL},"S_loadSomCoffLib_OBJ_FMT", (char *) S_loadSomCoffLib_OBJ_FMT,0},
	{{NULL},"S_loadSomCoffLib_SPHDR_ALLOC", (char *) S_loadSomCoffLib_SPHDR_ALLOC,0},
	{{NULL},"S_loadSomCoffLib_SPHDR_READ", (char *) S_loadSomCoffLib_SPHDR_READ,0},
	{{NULL},"S_loadSomCoffLib_SUBSPHDR_ALLOC", (char *) S_loadSomCoffLib_SUBSPHDR_ALLOC,0},
	{{NULL},"S_loadSomCoffLib_SUBSPHDR_READ", (char *) S_loadSomCoffLib_SUBSPHDR_READ,0},
	{{NULL},"S_loadSomCoffLib_SPSTRING_ALLOC", (char *) S_loadSomCoffLib_SPSTRING_ALLOC,0},
	{{NULL},"S_loadSomCoffLib_SPSTRING_READ", (char *) S_loadSomCoffLib_SPSTRING_READ,0},
	{{NULL},"S_loadSomCoffLib_INFO_ALLOC", (char *) S_loadSomCoffLib_INFO_ALLOC,0},
	{{NULL},"S_loadSomCoffLib_LOAD_SPACE", (char *) S_loadSomCoffLib_LOAD_SPACE,0},
	{{NULL},"S_loadSomCoffLib_RELOC_ALLOC", (char *) S_loadSomCoffLib_RELOC_ALLOC,0},
	{{NULL},"S_loadSomCoffLib_RELOC_READ", (char *) S_loadSomCoffLib_RELOC_READ,0},
	{{NULL},"S_loadSomCoffLib_RELOC_NEW", (char *) S_loadSomCoffLib_RELOC_NEW,0},
	{{NULL},"S_loadSomCoffLib_RELOC_VERSION", (char *) S_loadSomCoffLib_RELOC_VERSION,0},
	{{NULL},"S_loginLib_UNKNOWN_USER", (char *) S_loginLib_UNKNOWN_USER,0},
	{{NULL},"S_loginLib_USER_ALREADY_EXISTS", (char *) S_loginLib_USER_ALREADY_EXISTS,0},
	{{NULL},"S_loginLib_INVALID_PASSWORD", (char *) S_loginLib_INVALID_PASSWORD,0},
	{{NULL},"S_m2Lib_INVALID_PARAMETER", (char *) S_m2Lib_INVALID_PARAMETER,0},
	{{NULL},"S_m2Lib_ENTRY_NOT_FOUND", (char *) S_m2Lib_ENTRY_NOT_FOUND,0},
	{{NULL},"S_m2Lib_TCPCONN_FD_NOT_FOUND", (char *) S_m2Lib_TCPCONN_FD_NOT_FOUND,0},
	{{NULL},"S_m2Lib_INVALID_VAR_TO_SET", (char *) S_m2Lib_INVALID_VAR_TO_SET,0},
	{{NULL},"S_m2Lib_CANT_CREATE_SYS_SEM", (char *) S_m2Lib_CANT_CREATE_SYS_SEM,0},
	{{NULL},"S_m2Lib_CANT_CREATE_IF_SEM", (char *) S_m2Lib_CANT_CREATE_IF_SEM,0},
	{{NULL},"S_m2Lib_CANT_CREATE_ROUTE_SEM", (char *) S_m2Lib_CANT_CREATE_ROUTE_SEM,0},
	{{NULL},"S_m2Lib_ARP_PHYSADDR_NOT_SPECIFIED", (char *) S_m2Lib_ARP_PHYSADDR_NOT_SPECIFIED,0},
	{{NULL},"S_m2Lib_IF_TBL_IS_EMPTY", (char *) S_m2Lib_IF_TBL_IS_EMPTY,0},
	{{NULL},"S_m2Lib_IF_CNFG_CHANGED", (char *) S_m2Lib_IF_CNFG_CHANGED,0},
	{{NULL},"S_m2Lib_TOO_BIG", (char *) S_m2Lib_TOO_BIG,0},
	{{NULL},"S_m2Lib_BAD_VALUE", (char *) S_m2Lib_BAD_VALUE,0},
	{{NULL},"S_m2Lib_READ_ONLY", (char *) S_m2Lib_READ_ONLY,0},
	{{NULL},"S_m2Lib_GEN_ERR", (char *) S_m2Lib_GEN_ERR,0},
	{{NULL},"S_memLib_NOT_ENOUGH_MEMORY", (char *) S_memLib_NOT_ENOUGH_MEMORY,0},
	{{NULL},"S_memLib_INVALID_NBYTES", (char *) S_memLib_INVALID_NBYTES,0},
	{{NULL},"S_memLib_BLOCK_ERROR", (char *) S_memLib_BLOCK_ERROR,0},
	{{NULL},"S_memLib_NO_PARTITION_DESTROY", (char *) S_memLib_NO_PARTITION_DESTROY,0},
	{{NULL},"S_memLib_PAGE_SIZE_UNAVAILABLE", (char *) S_memLib_PAGE_SIZE_UNAVAILABLE,0},
	{{NULL},"S_miiLib_PHY_LINK_DOWN", (char *) S_miiLib_PHY_LINK_DOWN,0},
	{{NULL},"S_miiLib_PHY_NULL", (char *) S_miiLib_PHY_NULL,0},
	{{NULL},"S_miiLib_PHY_NO_ABLE", (char *) S_miiLib_PHY_NO_ABLE,0},
	{{NULL},"S_miiLib_PHY_AN_FAIL", (char *) S_miiLib_PHY_AN_FAIL,0},
	{{NULL},"S_moduleLib_MODULE_NOT_FOUND", (char *) S_moduleLib_MODULE_NOT_FOUND,0},
	{{NULL},"S_moduleLib_HOOK_NOT_FOUND", (char *) S_moduleLib_HOOK_NOT_FOUND,0},
	{{NULL},"S_moduleLib_BAD_CHECKSUM", (char *) S_moduleLib_BAD_CHECKSUM,0},
	{{NULL},"S_moduleLib_MAX_MODULES_LOADED", (char *) S_moduleLib_MAX_MODULES_LOADED,0},
	{{NULL},"S_mountLib_ILLEGAL_MODE", (char *) S_mountLib_ILLEGAL_MODE,0},
	{{NULL},"S_msgQLib_INVALID_MSG_LENGTH", (char *) S_msgQLib_INVALID_MSG_LENGTH,0},
	{{NULL},"S_msgQLib_NON_ZERO_TIMEOUT_AT_INT_LEVEL", (char *) S_msgQLib_NON_ZERO_TIMEOUT_AT_INT_LEVEL,0},
	{{NULL},"S_msgQLib_INVALID_QUEUE_TYPE", (char *) S_msgQLib_INVALID_QUEUE_TYPE,0},
	{{NULL},"S_muxLib_LOAD_FAILED", (char *) S_muxLib_LOAD_FAILED,0},
	{{NULL},"S_muxLib_NO_DEVICE", (char *) S_muxLib_NO_DEVICE,0},
	{{NULL},"S_muxLib_INVALID_ARGS", (char *) S_muxLib_INVALID_ARGS,0},
	{{NULL},"S_muxLib_ALLOC_FAILED", (char *) S_muxLib_ALLOC_FAILED,0},
	{{NULL},"S_muxLib_ALREADY_BOUND", (char *) S_muxLib_ALREADY_BOUND,0},
	{{NULL},"S_muxLib_UNLOAD_FAILED", (char *) S_muxLib_UNLOAD_FAILED,0},
	{{NULL},"S_muxLib_NOT_A_TK_DEVICE", (char *) S_muxLib_NOT_A_TK_DEVICE,0},
	{{NULL},"S_muxLib_NO_TK_DEVICE", (char *) S_muxLib_NO_TK_DEVICE,0},
	{{NULL},"S_muxLib_END_BIND_FAILED", (char *) S_muxLib_END_BIND_FAILED,0},
	{{NULL},"S_netBufLib_MBLK_INVALID", (char *) S_netBufLib_MBLK_INVALID,0},
	{{NULL},"S_netBufLib_NETPOOL_INVALID", (char *) S_netBufLib_NETPOOL_INVALID,0},
	{{NULL},"S_netBufLib_INVALID_ARGUMENT", (char *) S_netBufLib_INVALID_ARGUMENT,0},
	{{NULL},"S_netBufLib_NO_POOL_MEMORY", (char *) S_netBufLib_NO_POOL_MEMORY,0},
	{{NULL},"S_netDrv_INVALID_NUMBER_OF_BYTES", (char *) S_netDrv_INVALID_NUMBER_OF_BYTES,0},
	{{NULL},"S_netDrv_SEND_ERROR", (char *) S_netDrv_SEND_ERROR,0},
	{{NULL},"S_netDrv_RECV_ERROR", (char *) S_netDrv_RECV_ERROR,0},
	{{NULL},"S_netDrv_UNKNOWN_REQUEST", (char *) S_netDrv_UNKNOWN_REQUEST,0},
	{{NULL},"S_netDrv_BAD_SEEK", (char *) S_netDrv_BAD_SEEK,0},
	{{NULL},"S_netDrv_SEEK_PAST_EOF_ERROR", (char *) S_netDrv_SEEK_PAST_EOF_ERROR,0},
	{{NULL},"S_netDrv_BAD_EOF_POSITION", (char *) S_netDrv_BAD_EOF_POSITION,0},
	{{NULL},"S_netDrv_SEEK_FATAL_ERROR", (char *) S_netDrv_SEEK_FATAL_ERROR,0},
	{{NULL},"S_netDrv_WRITE_ERROR", (char *) S_netDrv_WRITE_ERROR,0},
	{{NULL},"S_netDrv_NO_SUCH_FILE_OR_DIR", (char *) S_netDrv_NO_SUCH_FILE_OR_DIR,0},
	{{NULL},"S_netDrv_PERMISSION_DENIED", (char *) S_netDrv_PERMISSION_DENIED,0},
	{{NULL},"S_netDrv_IS_A_DIRECTORY", (char *) S_netDrv_IS_A_DIRECTORY,0},
	{{NULL},"S_netDrv_UNIX_FILE_ERROR", (char *) S_netDrv_UNIX_FILE_ERROR,0},
	{{NULL},"S_netDrv_ILLEGAL_VALUE", (char *) S_netDrv_ILLEGAL_VALUE,0},
	{{NULL},"S_nfsDrv_INVALID_NUMBER_OF_BYTES", (char *) S_nfsDrv_INVALID_NUMBER_OF_BYTES,0},
	{{NULL},"S_nfsDrv_BAD_FLAG_MODE", (char *) S_nfsDrv_BAD_FLAG_MODE,0},
	{{NULL},"S_nfsDrv_CREATE_NO_FILE_NAME", (char *) S_nfsDrv_CREATE_NO_FILE_NAME,0},
	{{NULL},"S_nfsLib_NFS_OK", (char *) S_nfsLib_NFS_OK,0},
	{{NULL},"S_nfsLib_NFSERR_PERM", (char *) S_nfsLib_NFSERR_PERM,0},
	{{NULL},"S_nfsLib_NFSERR_NOENT", (char *) S_nfsLib_NFSERR_NOENT,0},
	{{NULL},"S_nfsLib_NFSERR_IO", (char *) S_nfsLib_NFSERR_IO,0},
	{{NULL},"S_nfsLib_NFSERR_NXIO", (char *) S_nfsLib_NFSERR_NXIO,0},
	{{NULL},"S_nfsLib_NFSERR_ACCES", (char *) S_nfsLib_NFSERR_ACCES,0},
	{{NULL},"S_nfsLib_NFSERR_EXIST", (char *) S_nfsLib_NFSERR_EXIST,0},
	{{NULL},"S_nfsLib_NFSERR_NODEV", (char *) S_nfsLib_NFSERR_NODEV,0},
	{{NULL},"S_nfsLib_NFSERR_NOTDIR", (char *) S_nfsLib_NFSERR_NOTDIR,0},
	{{NULL},"S_nfsLib_NFSERR_ISDIR", (char *) S_nfsLib_NFSERR_ISDIR,0},
	{{NULL},"S_nfsLib_NFSERR_FBIG", (char *) S_nfsLib_NFSERR_FBIG,0},
	{{NULL},"S_nfsLib_NFSERR_NOSPC", (char *) S_nfsLib_NFSERR_NOSPC,0},
	{{NULL},"S_nfsLib_NFSERR_ROFS", (char *) S_nfsLib_NFSERR_ROFS,0},
	{{NULL},"S_nfsLib_NFSERR_NAMETOOLONG", (char *) S_nfsLib_NFSERR_NAMETOOLONG,0},
	{{NULL},"S_nfsLib_NFSERR_NOTEMPTY", (char *) S_nfsLib_NFSERR_NOTEMPTY,0},
	{{NULL},"S_nfsLib_NFSERR_DQUOT", (char *) S_nfsLib_NFSERR_DQUOT,0},
	{{NULL},"S_nfsLib_NFSERR_STALE", (char *) S_nfsLib_NFSERR_STALE,0},
	{{NULL},"S_nfsLib_NFSERR_WFLUSH", (char *) S_nfsLib_NFSERR_WFLUSH,0},
	{{NULL},"S_objLib_OBJ_ID_ERROR", (char *) S_objLib_OBJ_ID_ERROR,0},
	{{NULL},"S_objLib_OBJ_UNAVAILABLE", (char *) S_objLib_OBJ_UNAVAILABLE,0},
	{{NULL},"S_objLib_OBJ_DELETED", (char *) S_objLib_OBJ_DELETED,0},
	{{NULL},"S_objLib_OBJ_TIMEOUT", (char *) S_objLib_OBJ_TIMEOUT,0},
	{{NULL},"S_objLib_OBJ_NO_METHOD", (char *) S_objLib_OBJ_NO_METHOD,0},
	{{NULL},"S_proxyArpLib_INVALID_INTERFACE", (char *) S_proxyArpLib_INVALID_INTERFACE,0},
	{{NULL},"S_proxyArpLib_INVALID_ADDRESS", (char *) S_proxyArpLib_INVALID_ADDRESS,0},
	{{NULL},"S_proxyArpLib_TIMEOUT", (char *) S_proxyArpLib_TIMEOUT,0},
	{{NULL},"S_qLib_Q_CLASS_ID_ERROR", (char *) S_qLib_Q_CLASS_ID_ERROR,0},
	{{NULL},"S_qPriBMapLib_NULL_BMAP_LIST", (char *) S_qPriBMapLib_NULL_BMAP_LIST,0},
	{{NULL},"S_qPriHeapLib_NULL_HEAP_ARRAY", (char *) S_qPriHeapLib_NULL_HEAP_ARRAY,0},
	{{NULL},"S_rawFsLib_VOLUME_NOT_AVAILABLE", (char *) S_rawFsLib_VOLUME_NOT_AVAILABLE,0},
	{{NULL},"S_rawFsLib_END_OF_DEVICE", (char *) S_rawFsLib_END_OF_DEVICE,0},
	{{NULL},"S_rawFsLib_NO_FREE_FILE_DESCRIPTORS", (char *) S_rawFsLib_NO_FREE_FILE_DESCRIPTORS,0},
	{{NULL},"S_rawFsLib_INVALID_NUMBER_OF_BYTES", (char *) S_rawFsLib_INVALID_NUMBER_OF_BYTES,0},
	{{NULL},"S_rawFsLib_ILLEGAL_NAME", (char *) S_rawFsLib_ILLEGAL_NAME,0},
	{{NULL},"S_rawFsLib_NOT_FILE", (char *) S_rawFsLib_NOT_FILE,0},
	{{NULL},"S_rawFsLib_READ_ONLY", (char *) S_rawFsLib_READ_ONLY,0},
	{{NULL},"S_rawFsLib_FD_OBSOLETE", (char *) S_rawFsLib_FD_OBSOLETE,0},
	{{NULL},"S_rawFsLib_NO_BLOCK_DEVICE", (char *) S_rawFsLib_NO_BLOCK_DEVICE,0},
	{{NULL},"S_rawFsLib_BAD_SEEK", (char *) S_rawFsLib_BAD_SEEK,0},
	{{NULL},"S_rawFsLib_INVALID_PARAMETER", (char *) S_rawFsLib_INVALID_PARAMETER,0},
	{{NULL},"S_rawFsLib_32BIT_OVERFLOW", (char *) S_rawFsLib_32BIT_OVERFLOW,0},
	{{NULL},"S_remLib_ALL_PORTS_IN_USE", (char *) S_remLib_ALL_PORTS_IN_USE,0},
	{{NULL},"S_remLib_RSH_ERROR", (char *) S_remLib_RSH_ERROR,0},
	{{NULL},"S_remLib_IDENTITY_TOO_BIG", (char *) S_remLib_IDENTITY_TOO_BIG,0},
	{{NULL},"S_remLib_RSH_STDERR_SETUP_FAILED", (char *) S_remLib_RSH_STDERR_SETUP_FAILED,0},
	{{NULL},"S_resolvLib_NO_RECOVERY", (char *) S_resolvLib_NO_RECOVERY,0},
	{{NULL},"S_resolvLib_TRY_AGAIN", (char *) S_resolvLib_TRY_AGAIN,0},
	{{NULL},"S_resolvLib_HOST_NOT_FOUND", (char *) S_resolvLib_HOST_NOT_FOUND,0},
	{{NULL},"S_resolvLib_NO_DATA", (char *) S_resolvLib_NO_DATA,0},
	{{NULL},"S_resolvLib_BUFFER_2_SMALL", (char *) S_resolvLib_BUFFER_2_SMALL,0},
	{{NULL},"S_resolvLib_INVALID_PARAMETER", (char *) S_resolvLib_INVALID_PARAMETER,0},
	{{NULL},"S_resolvLib_INVALID_ADDRESS", (char *) S_resolvLib_INVALID_ADDRESS,0},
	{{NULL},"S_routeLib_ILLEGAL_INTERNET_ADDRESS", (char *) S_routeLib_ILLEGAL_INTERNET_ADDRESS,0},
	{{NULL},"S_routeLib_ILLEGAL_NETWORK_NUMBER", (char *) S_routeLib_ILLEGAL_NETWORK_NUMBER,0},
	{{NULL},"S_rpcLib_RPC_SUCCESS", (char *) S_rpcLib_RPC_SUCCESS,0},
	{{NULL},"S_rpcLib_RPC_CANTENCODEARGS", (char *) S_rpcLib_RPC_CANTENCODEARGS,0},
	{{NULL},"S_rpcLib_RPC_CANTDECODERES", (char *) S_rpcLib_RPC_CANTDECODERES,0},
	{{NULL},"S_rpcLib_RPC_CANTSEND", (char *) S_rpcLib_RPC_CANTSEND,0},
	{{NULL},"S_rpcLib_RPC_CANTRECV", (char *) S_rpcLib_RPC_CANTRECV,0},
	{{NULL},"S_rpcLib_RPC_TIMEDOUT", (char *) S_rpcLib_RPC_TIMEDOUT,0},
	{{NULL},"S_rpcLib_RPC_VERSMISMATCH", (char *) S_rpcLib_RPC_VERSMISMATCH,0},
	{{NULL},"S_rpcLib_RPC_AUTHERROR", (char *) S_rpcLib_RPC_AUTHERROR,0},
	{{NULL},"S_rpcLib_RPC_PROGUNAVAIL", (char *) S_rpcLib_RPC_PROGUNAVAIL,0},
	{{NULL},"S_rpcLib_RPC_PROGVERSMISMATCH", (char *) S_rpcLib_RPC_PROGVERSMISMATCH,0},
	{{NULL},"S_rpcLib_RPC_PROCUNAVAIL", (char *) S_rpcLib_RPC_PROCUNAVAIL,0},
	{{NULL},"S_rpcLib_RPC_CANTDECODEARGS", (char *) S_rpcLib_RPC_CANTDECODEARGS,0},
	{{NULL},"S_rpcLib_RPC_SYSTEMERROR", (char *) S_rpcLib_RPC_SYSTEMERROR,0},
	{{NULL},"S_rpcLib_RPC_UNKNOWNHOST", (char *) S_rpcLib_RPC_UNKNOWNHOST,0},
	{{NULL},"S_rpcLib_RPC_PMAPFAILURE", (char *) S_rpcLib_RPC_PMAPFAILURE,0},
	{{NULL},"S_rpcLib_RPC_PROGNOTREGISTERED", (char *) S_rpcLib_RPC_PROGNOTREGISTERED,0},
	{{NULL},"S_rpcLib_RPC_FAILED", (char *) S_rpcLib_RPC_FAILED,0},
	{{NULL},"S_rt11FsLib_VOLUME_NOT_AVAILABLE", (char *) S_rt11FsLib_VOLUME_NOT_AVAILABLE,0},
	{{NULL},"S_rt11FsLib_DISK_FULL", (char *) S_rt11FsLib_DISK_FULL,0},
	{{NULL},"S_rt11FsLib_FILE_NOT_FOUND", (char *) S_rt11FsLib_FILE_NOT_FOUND,0},
	{{NULL},"S_rt11FsLib_NO_FREE_FILE_DESCRIPTORS", (char *) S_rt11FsLib_NO_FREE_FILE_DESCRIPTORS,0},
	{{NULL},"S_rt11FsLib_INVALID_NUMBER_OF_BYTES", (char *) S_rt11FsLib_INVALID_NUMBER_OF_BYTES,0},
	{{NULL},"S_rt11FsLib_FILE_ALREADY_EXISTS", (char *) S_rt11FsLib_FILE_ALREADY_EXISTS,0},
	{{NULL},"S_rt11FsLib_BEYOND_FILE_LIMIT", (char *) S_rt11FsLib_BEYOND_FILE_LIMIT,0},
	{{NULL},"S_rt11FsLib_INVALID_DEVICE_PARAMETERS", (char *) S_rt11FsLib_INVALID_DEVICE_PARAMETERS,0},
	{{NULL},"S_rt11FsLib_NO_MORE_FILES_ALLOWED_ON_DISK", (char *) S_rt11FsLib_NO_MORE_FILES_ALLOWED_ON_DISK,0},
	{{NULL},"S_scsiLib_DEV_NOT_READY", (char *) S_scsiLib_DEV_NOT_READY,0},
	{{NULL},"S_scsiLib_WRITE_PROTECTED", (char *) S_scsiLib_WRITE_PROTECTED,0},
	{{NULL},"S_scsiLib_MEDIUM_ERROR", (char *) S_scsiLib_MEDIUM_ERROR,0},
	{{NULL},"S_scsiLib_HARDWARE_ERROR", (char *) S_scsiLib_HARDWARE_ERROR,0},
	{{NULL},"S_scsiLib_ILLEGAL_REQUEST", (char *) S_scsiLib_ILLEGAL_REQUEST,0},
	{{NULL},"S_scsiLib_BLANK_CHECK", (char *) S_scsiLib_BLANK_CHECK,0},
	{{NULL},"S_scsiLib_ABORTED_COMMAND", (char *) S_scsiLib_ABORTED_COMMAND,0},
	{{NULL},"S_scsiLib_VOLUME_OVERFLOW", (char *) S_scsiLib_VOLUME_OVERFLOW,0},
	{{NULL},"S_scsiLib_UNIT_ATTENTION", (char *) S_scsiLib_UNIT_ATTENTION,0},
	{{NULL},"S_scsiLib_SELECT_TIMEOUT", (char *) S_scsiLib_SELECT_TIMEOUT,0},
	{{NULL},"S_scsiLib_LUN_NOT_PRESENT", (char *) S_scsiLib_LUN_NOT_PRESENT,0},
	{{NULL},"S_scsiLib_ILLEGAL_BUS_ID", (char *) S_scsiLib_ILLEGAL_BUS_ID,0},
	{{NULL},"S_scsiLib_NO_CONTROLLER", (char *) S_scsiLib_NO_CONTROLLER,0},
	{{NULL},"S_scsiLib_REQ_SENSE_ERROR", (char *) S_scsiLib_REQ_SENSE_ERROR,0},
	{{NULL},"S_scsiLib_DEV_UNSUPPORTED", (char *) S_scsiLib_DEV_UNSUPPORTED,0},
	{{NULL},"S_scsiLib_ILLEGAL_PARAMETER", (char *) S_scsiLib_ILLEGAL_PARAMETER,0},
	{{NULL},"S_scsiLib_EARLY_PHASE_CHANGE", (char *) S_scsiLib_EARLY_PHASE_CHANGE,0},
	{{NULL},"S_scsiLib_PHASE_CHANGE_TIMEOUT", (char *) S_scsiLib_PHASE_CHANGE_TIMEOUT,0},
	{{NULL},"S_scsiLib_ILLEGAL_OPERATION", (char *) S_scsiLib_ILLEGAL_OPERATION,0},
	{{NULL},"S_scsiLib_DEVICE_EXIST", (char *) S_scsiLib_DEVICE_EXIST,0},
	{{NULL},"S_scsiLib_SYNC_UNSUPPORTED", (char *) S_scsiLib_SYNC_UNSUPPORTED,0},
	{{NULL},"S_scsiLib_SYNC_VAL_UNSUPPORTED", (char *) S_scsiLib_SYNC_VAL_UNSUPPORTED,0},
	{{NULL},"S_scsiLib_DEV_NOT_READY", (char *) S_scsiLib_DEV_NOT_READY,0},
	{{NULL},"S_scsiLib_WRITE_PROTECTED", (char *) S_scsiLib_WRITE_PROTECTED,0},
	{{NULL},"S_scsiLib_MEDIUM_ERROR", (char *) S_scsiLib_MEDIUM_ERROR,0},
	{{NULL},"S_scsiLib_HARDWARE_ERROR", (char *) S_scsiLib_HARDWARE_ERROR,0},
	{{NULL},"S_scsiLib_ILLEGAL_REQUEST", (char *) S_scsiLib_ILLEGAL_REQUEST,0},
	{{NULL},"S_scsiLib_BLANK_CHECK", (char *) S_scsiLib_BLANK_CHECK,0},
	{{NULL},"S_scsiLib_ABORTED_COMMAND", (char *) S_scsiLib_ABORTED_COMMAND,0},
	{{NULL},"S_scsiLib_VOLUME_OVERFLOW", (char *) S_scsiLib_VOLUME_OVERFLOW,0},
	{{NULL},"S_scsiLib_UNIT_ATTENTION", (char *) S_scsiLib_UNIT_ATTENTION,0},
	{{NULL},"S_scsiLib_SELECT_TIMEOUT", (char *) S_scsiLib_SELECT_TIMEOUT,0},
	{{NULL},"S_scsiLib_LUN_NOT_PRESENT", (char *) S_scsiLib_LUN_NOT_PRESENT,0},
	{{NULL},"S_scsiLib_ILLEGAL_BUS_ID", (char *) S_scsiLib_ILLEGAL_BUS_ID,0},
	{{NULL},"S_scsiLib_NO_CONTROLLER", (char *) S_scsiLib_NO_CONTROLLER,0},
	{{NULL},"S_scsiLib_REQ_SENSE_ERROR", (char *) S_scsiLib_REQ_SENSE_ERROR,0},
	{{NULL},"S_scsiLib_DEV_UNSUPPORTED", (char *) S_scsiLib_DEV_UNSUPPORTED,0},
	{{NULL},"S_scsiLib_ILLEGAL_PARAMETER", (char *) S_scsiLib_ILLEGAL_PARAMETER,0},
	{{NULL},"S_scsiLib_INVALID_PHASE", (char *) S_scsiLib_INVALID_PHASE,0},
	{{NULL},"S_scsiLib_ABORTED", (char *) S_scsiLib_ABORTED,0},
	{{NULL},"S_scsiLib_ILLEGAL_OPERATION", (char *) S_scsiLib_ILLEGAL_OPERATION,0},
	{{NULL},"S_scsiLib_DEVICE_EXIST", (char *) S_scsiLib_DEVICE_EXIST,0},
	{{NULL},"S_scsiLib_DISCONNECTED", (char *) S_scsiLib_DISCONNECTED,0},
	{{NULL},"S_scsiLib_BUS_RESET", (char *) S_scsiLib_BUS_RESET,0},
	{{NULL},"S_selectLib_NO_SELECT_SUPPORT_IN_DRIVER", (char *) S_selectLib_NO_SELECT_SUPPORT_IN_DRIVER,0},
	{{NULL},"S_selectLib_NO_SELECT_CONTEXT", (char *) S_selectLib_NO_SELECT_CONTEXT,0},
	{{NULL},"S_selectLib_WIDTH_OUT_OF_RANGE", (char *) S_selectLib_WIDTH_OUT_OF_RANGE,0},
	{{NULL},"S_semLib_INVALID_STATE", (char *) S_semLib_INVALID_STATE,0},
	{{NULL},"S_semLib_INVALID_OPTION", (char *) S_semLib_INVALID_OPTION,0},
	{{NULL},"S_semLib_INVALID_QUEUE_TYPE", (char *) S_semLib_INVALID_QUEUE_TYPE,0},
	{{NULL},"S_semLib_INVALID_OPERATION", (char *) S_semLib_INVALID_OPERATION,0},
	{{NULL},"S_smLib_MEMORY_ERROR", (char *) S_smLib_MEMORY_ERROR,0},
	{{NULL},"S_smLib_INVALID_CPU_NUMBER", (char *) S_smLib_INVALID_CPU_NUMBER,0},
	{{NULL},"S_smLib_NOT_ATTACHED", (char *) S_smLib_NOT_ATTACHED,0},
	{{NULL},"S_smLib_NO_REGIONS", (char *) S_smLib_NO_REGIONS,0},
	{{NULL},"S_smNameLib_NOT_INITIALIZED", (char *) S_smNameLib_NOT_INITIALIZED,0},
	{{NULL},"S_smNameLib_NAME_TOO_LONG", (char *) S_smNameLib_NAME_TOO_LONG,0},
	{{NULL},"S_smNameLib_NAME_NOT_FOUND", (char *) S_smNameLib_NAME_NOT_FOUND,0},
	{{NULL},"S_smNameLib_VALUE_NOT_FOUND", (char *) S_smNameLib_VALUE_NOT_FOUND,0},
	{{NULL},"S_smNameLib_NAME_ALREADY_EXIST", (char *) S_smNameLib_NAME_ALREADY_EXIST,0},
	{{NULL},"S_smNameLib_DATABASE_FULL", (char *) S_smNameLib_DATABASE_FULL,0},
	{{NULL},"S_smNameLib_INVALID_WAIT_TYPE", (char *) S_smNameLib_INVALID_WAIT_TYPE,0},
	{{NULL},"S_smObjLib_NOT_INITIALIZED", (char *) S_smObjLib_NOT_INITIALIZED,0},
	{{NULL},"S_smObjLib_NOT_A_GLOBAL_ADRS", (char *) S_smObjLib_NOT_A_GLOBAL_ADRS,0},
	{{NULL},"S_smObjLib_NOT_A_LOCAL_ADRS", (char *) S_smObjLib_NOT_A_LOCAL_ADRS,0},
	{{NULL},"S_smObjLib_SHARED_MEM_TOO_SMALL", (char *) S_smObjLib_SHARED_MEM_TOO_SMALL,0},
	{{NULL},"S_smObjLib_TOO_MANY_CPU", (char *) S_smObjLib_TOO_MANY_CPU,0},
	{{NULL},"S_smObjLib_LOCK_TIMEOUT", (char *) S_smObjLib_LOCK_TIMEOUT,0},
	{{NULL},"S_smObjLib_NO_OBJECT_DESTROY", (char *) S_smObjLib_NO_OBJECT_DESTROY,0},
	{{NULL},"S_smPktLib_SHARED_MEM_TOO_SMALL", (char *) S_smPktLib_SHARED_MEM_TOO_SMALL,0},
	{{NULL},"S_smPktLib_MEMORY_ERROR", (char *) S_smPktLib_MEMORY_ERROR,0},
	{{NULL},"S_smPktLib_DOWN", (char *) S_smPktLib_DOWN,0},
	{{NULL},"S_smPktLib_NOT_ATTACHED", (char *) S_smPktLib_NOT_ATTACHED,0},
	{{NULL},"S_smPktLib_INVALID_PACKET", (char *) S_smPktLib_INVALID_PACKET,0},
	{{NULL},"S_smPktLib_PACKET_TOO_BIG", (char *) S_smPktLib_PACKET_TOO_BIG,0},
	{{NULL},"S_smPktLib_INVALID_CPU_NUMBER", (char *) S_smPktLib_INVALID_CPU_NUMBER,0},
	{{NULL},"S_smPktLib_DEST_NOT_ATTACHED", (char *) S_smPktLib_DEST_NOT_ATTACHED,0},
	{{NULL},"S_smPktLib_INCOMPLETE_BROADCAST", (char *) S_smPktLib_INCOMPLETE_BROADCAST,0},
	{{NULL},"S_smPktLib_LIST_FULL", (char *) S_smPktLib_LIST_FULL,0},
	{{NULL},"S_smPktLib_LOCK_TIMEOUT", (char *) S_smPktLib_LOCK_TIMEOUT,0},
	{{NULL},"S_snmpdLib_VIEW_CREATE_FAILURE", (char *) S_snmpdLib_VIEW_CREATE_FAILURE,0},
	{{NULL},"S_snmpdLib_VIEW_INSTALL_FAILURE", (char *) S_snmpdLib_VIEW_INSTALL_FAILURE,0},
	{{NULL},"S_snmpdLib_VIEW_MASK_FAILURE", (char *) S_snmpdLib_VIEW_MASK_FAILURE,0},
	{{NULL},"S_snmpdLib_VIEW_DEINSTALL_FAILURE", (char *) S_snmpdLib_VIEW_DEINSTALL_FAILURE,0},
	{{NULL},"S_snmpdLib_VIEW_LOOKUP_FAILURE", (char *) S_snmpdLib_VIEW_LOOKUP_FAILURE,0},
	{{NULL},"S_snmpdLib_MIB_ADDITION_FAILURE", (char *) S_snmpdLib_MIB_ADDITION_FAILURE,0},
	{{NULL},"S_snmpdLib_NODE_NOT_FOUND", (char *) S_snmpdLib_NODE_NOT_FOUND,0},
	{{NULL},"S_snmpdLib_INVALID_SNMP_VERSION", (char *) S_snmpdLib_INVALID_SNMP_VERSION,0},
	{{NULL},"S_snmpdLib_TRAP_CREATE_FAILURE", (char *) S_snmpdLib_TRAP_CREATE_FAILURE,0},
	{{NULL},"S_snmpdLib_TRAP_BIND_FAILURE", (char *) S_snmpdLib_TRAP_BIND_FAILURE,0},
	{{NULL},"S_snmpdLib_TRAP_ENCODE_FAILURE", (char *) S_snmpdLib_TRAP_ENCODE_FAILURE,0},
	{{NULL},"S_snmpdLib_INVALID_OID_SYNTAX", (char *) S_snmpdLib_INVALID_OID_SYNTAX,0},
	{{NULL},"S_sntpcLib_INVALID_PARAMETER", (char *) S_sntpcLib_INVALID_PARAMETER,0},
	{{NULL},"S_sntpcLib_INVALID_ADDRESS", (char *) S_sntpcLib_INVALID_ADDRESS,0},
	{{NULL},"S_sntpcLib_TIMEOUT", (char *) S_sntpcLib_TIMEOUT,0},
	{{NULL},"S_sntpcLib_VERSION_UNSUPPORTED", (char *) S_sntpcLib_VERSION_UNSUPPORTED,0},
	{{NULL},"S_sntpcLib_SERVER_UNSYNC", (char *) S_sntpcLib_SERVER_UNSYNC,0},
	{{NULL},"S_sntpsLib_INVALID_PARAMETER", (char *) S_sntpsLib_INVALID_PARAMETER,0},
	{{NULL},"S_sntpsLib_INVALID_ADDRESS", (char *) S_sntpsLib_INVALID_ADDRESS,0},
	{{NULL},"S_symLib_SYMBOL_NOT_FOUND", (char *) S_symLib_SYMBOL_NOT_FOUND,0},
	{{NULL},"S_symLib_NAME_CLASH", (char *) S_symLib_NAME_CLASH,0},
	{{NULL},"S_symLib_TABLE_NOT_EMPTY", (char *) S_symLib_TABLE_NOT_EMPTY,0},
	{{NULL},"S_symLib_SYMBOL_STILL_IN_TABLE", (char *) S_symLib_SYMBOL_STILL_IN_TABLE,0},
	{{NULL},"S_symLib_INVALID_SYMTAB_ID", (char *) S_symLib_INVALID_SYMTAB_ID,0},
	{{NULL},"S_symLib_INVALID_SYM_ID_PTR", (char *) S_symLib_INVALID_SYM_ID_PTR,0},
	{{NULL},"S_tapeFsLib_NO_SEQ_DEV", (char *) S_tapeFsLib_NO_SEQ_DEV,0},
	{{NULL},"S_tapeFsLib_ILLEGAL_TAPE_CONFIG_PARM", (char *) S_tapeFsLib_ILLEGAL_TAPE_CONFIG_PARM,0},
	{{NULL},"S_tapeFsLib_SERVICE_NOT_AVAILABLE", (char *) S_tapeFsLib_SERVICE_NOT_AVAILABLE,0},
	{{NULL},"S_tapeFsLib_INVALID_BLOCK_SIZE", (char *) S_tapeFsLib_INVALID_BLOCK_SIZE,0},
	{{NULL},"S_tapeFsLib_ILLEGAL_FILE_SYSTEM_NAME", (char *) S_tapeFsLib_ILLEGAL_FILE_SYSTEM_NAME,0},
	{{NULL},"S_tapeFsLib_ILLEGAL_FLAGS", (char *) S_tapeFsLib_ILLEGAL_FLAGS,0},
	{{NULL},"S_tapeFsLib_FILE_DESCRIPTOR_BUSY", (char *) S_tapeFsLib_FILE_DESCRIPTOR_BUSY,0},
	{{NULL},"S_tapeFsLib_VOLUME_NOT_AVAILABLE", (char *) S_tapeFsLib_VOLUME_NOT_AVAILABLE,0},
	{{NULL},"S_tapeFsLib_BLOCK_SIZE_MISMATCH", (char *) S_tapeFsLib_BLOCK_SIZE_MISMATCH,0},
	{{NULL},"S_tapeFsLib_INVALID_NUMBER_OF_BYTES", (char *) S_tapeFsLib_INVALID_NUMBER_OF_BYTES,0},
	{{NULL},"S_taskLib_NAME_NOT_FOUND", (char *) S_taskLib_NAME_NOT_FOUND,0},
	{{NULL},"S_taskLib_TASK_HOOK_TABLE_FULL", (char *) S_taskLib_TASK_HOOK_TABLE_FULL,0},
	{{NULL},"S_taskLib_TASK_HOOK_NOT_FOUND", (char *) S_taskLib_TASK_HOOK_NOT_FOUND,0},
	{{NULL},"S_taskLib_TASK_SWAP_HOOK_REFERENCED", (char *) S_taskLib_TASK_SWAP_HOOK_REFERENCED,0},
	{{NULL},"S_taskLib_TASK_SWAP_HOOK_SET", (char *) S_taskLib_TASK_SWAP_HOOK_SET,0},
	{{NULL},"S_taskLib_TASK_SWAP_HOOK_CLEAR", (char *) S_taskLib_TASK_SWAP_HOOK_CLEAR,0},
	{{NULL},"S_taskLib_TASK_VAR_NOT_FOUND", (char *) S_taskLib_TASK_VAR_NOT_FOUND,0},
	{{NULL},"S_taskLib_TASK_UNDELAYED", (char *) S_taskLib_TASK_UNDELAYED,0},
	{{NULL},"S_taskLib_ILLEGAL_PRIORITY", (char *) S_taskLib_ILLEGAL_PRIORITY,0},
	{{NULL},"S_tftpLib_INVALID_ARGUMENT", (char *) S_tftpLib_INVALID_ARGUMENT,0},
	{{NULL},"S_tftpLib_INVALID_DESCRIPTOR", (char *) S_tftpLib_INVALID_DESCRIPTOR,0},
	{{NULL},"S_tftpLib_INVALID_COMMAND", (char *) S_tftpLib_INVALID_COMMAND,0},
	{{NULL},"S_tftpLib_INVALID_MODE", (char *) S_tftpLib_INVALID_MODE,0},
	{{NULL},"S_tftpLib_UNKNOWN_HOST", (char *) S_tftpLib_UNKNOWN_HOST,0},
	{{NULL},"S_tftpLib_NOT_CONNECTED", (char *) S_tftpLib_NOT_CONNECTED,0},
	{{NULL},"S_tftpLib_TIMED_OUT", (char *) S_tftpLib_TIMED_OUT,0},
	{{NULL},"S_tftpLib_TFTP_ERROR", (char *) S_tftpLib_TFTP_ERROR,0},
	{{NULL},"S_unldLib_MODULE_NOT_FOUND", (char *) S_unldLib_MODULE_NOT_FOUND,0},
	{{NULL},"S_unldLib_TEXT_IN_USE", (char *) S_unldLib_TEXT_IN_USE,0},
	{{NULL},"S_usrLib_NOT_ENOUGH_ARGS", (char *) S_usrLib_NOT_ENOUGH_ARGS,0},
	{{NULL},"S_vmLib_NOT_PAGE_ALIGNED", (char *) S_vmLib_NOT_PAGE_ALIGNED,0},
	{{NULL},"S_vmLib_BAD_STATE_PARAM", (char *) S_vmLib_BAD_STATE_PARAM,0},
	{{NULL},"S_vmLib_BAD_MASK_PARAM", (char *) S_vmLib_BAD_MASK_PARAM,0},
	{{NULL},"S_vmLib_ADDR_IN_GLOBAL_SPACE", (char *) S_vmLib_ADDR_IN_GLOBAL_SPACE,0},
	{{NULL},"S_vmLib_TEXT_PROTECTION_UNAVAILABLE", (char *) S_vmLib_TEXT_PROTECTION_UNAVAILABLE,0},
	{{NULL},"S_vmLib_NO_FREE_REGIONS", (char *) S_vmLib_NO_FREE_REGIONS,0},
	{{NULL},"S_vmLib_ADDRS_NOT_EQUAL", (char *) S_vmLib_ADDRS_NOT_EQUAL,0},
};

ULONG statTblSize = NELEMENTS (statTbl);
