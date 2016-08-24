/* os060Lib.h - MC68060 os dependent header */

/* Copyright 1984-1994 Wind River Systems, Inc. */

/*
modification history
--------------------
01a,27jun94,tpr	written.
*/

#ifndef __INCos060Libh
#define __INCos060Libh

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__STDC__) || defined(__cplusplus)

extern void	_060_dmem_write (void);
extern void	_060_imem_read (void);
extern void	_060_dmem_read (void);
extern void	_060_dmem_read_word (void);
extern void	_060_dmem_read_byte (void);
extern void	_060_dmem_read_long (void);
extern void	_060_dmem_write_byte (void);
extern void	_060_dmem_write_word (void);
extern void	_060_dmem_write_long (void);
extern void	_060_imem_read_word (void);
extern void	_060_imem_read_long (void);
extern void	_060_real_trace (void);
extern void	_060_real_access (void);

#else   /* __STDC__ */

extern void	_060_dmem_write ();
extern void	_060_imem_read ();
extern void	_060_dmem_read ();
extern void	_060_dmem_read_word ();
extern void	_060_dmem_read_byte ();
extern void	_060_dmem_read_long ();
extern void	_060_dmem_write_byte ();
extern void	_060_dmem_write_word ();
extern void	_060_dmem_write_long ();
extern void	_060_imem_read_word ();
extern void	_060_imem_read_long ();
extern void	_060_real_trace ();
extern void	_060_real_access ();

#endif  /* __STDC__ */


#ifdef __cplusplus
}
#endif

#endif /* __INCos060Libh */
