#ifndef _BOSS_CONF_H_
#define _BOSS_CONF_H_
/*
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*                                                                             *
*                 RT-BOSS (Config File)         [ ATmega128 ]                 *
*                                                                             *
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*/

/*===========================================================================*/
/*                            RT-BOSS ��������                               */
/*---------------------------------------------------------------------------*/
typedef unsigned char       boss_u08_t;     /* unsigned  8bit �������� */
typedef unsigned int        boss_u16_t;     /* unsigned 16bit �������� */
typedef unsigned long int   boss_u32_t;     /* unsigned 32bit �������� */

//typedef boss_u08_t          boss_byte_t;      /* Byte                 */
typedef boss_u08_t          boss_reg_t;       /* MCU �������� ũ��      */
typedef boss_u16_t          boss_uptr_t;      /* unsigned ������ ũ��   */
typedef boss_u08_t          boss_stk_t;       /* ����                   */

typedef boss_u16_t          boss_sigs_t;      /* �ñ׳�                 */
typedef boss_u32_t          boss_tmr_ms_t;    /* Ÿ�̸� ī��Ʈ(ms)      */

typedef boss_u16_t          boss_mem_align_t; /* �޸� ����            */

/*===========================================================================*/
/*   IRQ (Interrupt request) / ISR (Interrupt Service Routine)               */
/*---------------------------------------------------------------------------*/
#define BOSS_IRQ_DISABLE()    do { \
                                boss_reg_t _irq_storage_ = SREG; \
                                asm volatile("cli")
                                    
#define BOSS_IRQ_RESTORE()      SREG = _irq_storage_; \
                              } while(0)

/*----------------------------------------------------------------------*/
#define BOSS_IRQ_LOCK_SR( _sr_ )  \
                    do { _sr_ = SREG; asm volatile("cli"); } while(0)
            
#define BOSS_IRQ_FREE_SR( _sr_ )      do { SREG = _sr_; } while(0)

/*----------------------------------------------------------------------*/
#define _BOSS_IRQ_()    ( (SREG & (1 << SREG_I)) ? 0 : !0 ) /* 0 = Enable / !0 = Disable */

#define _BOSS_ISR_()    _mcu_isr_()    /* !0 = ISR Active  */

/*----------------------------------------------------------------------*/
#define _BOSS_ISR_BEGIN()   do { _Boss_sched_lock(); _mcu_isr_begin()
#define _BOSS_ISR_FINIS()   _mcu_isr_finis(); _Boss_sched_free(); } while(0)

boss_reg_t _mcu_isr_(void);
void _mcu_isr_begin(void);
void _mcu_isr_finis(void);


/*===========================================================================*/
/*                           RT-BOSS ����� ����                             */
/*---------------------------------------------------------------------------*/
//#define _BOSS_TCB_EXT_                /* TCB Ȯ��(extend) */
#define _BOSS_MEM_INFO_                 /* �޸� ����� ���� */

#define _BOSS_TICK_MS_          10      /* Tick (ms)  */
#define _IDLE_STACK_BYTES       128     /* Bytes      */
#define _BOSS_MEM_POOL_SIZE     1024    /* Bytes      */


/*===========================================================================*/
/*                                    ASSERT                                 */
/*---------------------------------------------------------------------------*/
#define BOSS_ASSERT(expr)  do { if(!(expr)) _assert(PSTR(__FILE__), __LINE__); } while(0)
void _assert(const char *file, unsigned int line);


/*===========================================================================*/
/*                             �½�ũ �켱����                               */
/*---------------------------------------------------------------------------*/
typedef enum {
  PRIO_BOSS_KERNEL = 0,   /* ���� �켱���� */
  
  AA_PRIO_1,              /* 1. TOP */
  BB_PRIO_2,              /* 2 */
  
  PRIO_BOSS_IDLE = 255,   /* ���� �켱���� */
} boss_prio_t;


/*===========================================================================*/
/*                           �޽��� ť ���ɾ�                                */
/*---------------------------------------------------------------------------*/
typedef enum {
  M_CMD_EMPTY = 0,

  M_CMD_1,
  M_CMD_2,
  /* ... */
  
  M_CMD_MAX = 255,
} msg_cmd_t;


/*===========================================================================*/
/*                               INCLUDE FILE                                */
/*---------------------------------------------------------------------------*/
#include <avr/io.h>

#include <stdio.h>        // "printf ����� ����"
#include <avr/pgmspace.h>

#include "Boss.h"
#include "Boss_Tmr.h"
#include "Boss_Mem.h"
#include "Boss_Q_Msg.h"
#include "Boss_Q_MBox.h"
#include "Boss_Sem.h"


/*===========================================================================*/
/*                     USER DEFINE & FUNCTION PROTOTYPES                     */
/*---------------------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
/*                       �ñ׳� ��Ʈ ����                          */

/* SIG_15_BIT ~ SIG_12_BIT (���� 4��Ʈ�� RT-BOSS���� �����)      */
#define SIG_11_BIT          (boss_sigs_t)(1 << 11)      /* 0x0800 */
#define SIG_10_BIT          (boss_sigs_t)(1 << 10)      /* 0x0400 */
#define SIG_09_BIT          (boss_sigs_t)(1 << 9)       /* 0x0200 */
#define SIG_08_BIT          (boss_sigs_t)(1 << 8)       /* 0x0100 */

#define SIG_07_BIT          (boss_sigs_t)(1 << 7)       /* 0x0080 */
#define SIG_06_BIT          (boss_sigs_t)(1 << 6)       /* 0x0040 */
#define SIG_05_BIT          (boss_sigs_t)(1 << 5)       /* 0x0020 */
#define SIG_04_BIT          (boss_sigs_t)(1 << 4)       /* 0x0010 */
#define SIG_03_BIT          (boss_sigs_t)(1 << 3)       /* 0x0008 */
#define SIG_02_BIT          (boss_sigs_t)(1 << 2)       /* 0x0004 */
#define SIG_01_BIT          (boss_sigs_t)(1 << 1)       /* 0x0002 */
#define SIG_00_BIT          (boss_sigs_t)(1 << 0)       /* 0x0001 */

//#define PRINTF(...)         printf_P(__VA_ARGS__)
#define PRINTF(fmt, args...)      printf_P( PSTR(fmt), ##args )


#endif  /* _BOSS_CONF_H_ */