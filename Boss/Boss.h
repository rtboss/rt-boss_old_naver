/*
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*                                                                             *
*                              RT-BOSS (Kernel)                               *
*                                                                             *
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*/

/*===========================================================================*/
/*                               INCLUDE FILE                                */
/*---------------------------------------------------------------------------*/
#include "Boss_Conf.h"

#ifndef _BOSS_KERNEL_H_
#define _BOSS_KERNEL_H_

/*===========================================================================*/
/*                      DEFINITIONS & TYPEDEFS & MACROS                      */
/*---------------------------------------------------------------------------*/
#define _BOSS_NULL          ((void *) 0)

#define _BOSS_FALSE         ( 0)
#define _BOSS_TRUE          (!_BOSS_FALSE)

#define _BOSS_SUCCESS       ( 0)
#define _BOSS_FAILURE       (-1)    /* (32bit=0xFFFFFFFF) / (16bit=0xFFFF) */

/*---------------------------------------------------------------------------*/
/*                         RT-BOSS �ñ׳� ��Ʈ ����                          */
/*                                                                           */
/*    �ñ׳� ���������� 32Bit�� ��� "BOSS_SIG_MSB_BIT" �� "0x8000 0000"     */
/*    �ñ׳� ���������� 16bit�� ��� "BOSS_SIG_MSB_BIT" �� "0x8000"          */
/*---------------------------------------------------------------------------*/
#define BOSS_SIG_MSB_BIT  (boss_sigs_t)((boss_sigs_t)1 << ((sizeof(boss_sigs_t)*8)-1))

#define BOSS_SIG_SLEEP            (boss_sigs_t)(BOSS_SIG_MSB_BIT >> 0)  /* ����     */
#define BOSS_SIG_SEM_OBTAIN       (boss_sigs_t)(BOSS_SIG_MSB_BIT >> 1)  /* �������� */
#define BOSS_SIG_MBOX_PEND_DONE   (boss_sigs_t)(BOSS_SIG_MSB_BIT >> 2)  /* MBox �Ϸ� */
// ����(Reserve)    BOSS_SIG_RESERVE    (BOSS_SIG_MSB_BIT >> 3)   /* ����     */

/*---------------------------------------------------------------------------*/
/*                        TCB (Task Control Block)                           */
/*---------------------------------------------------------------------------*/
typedef enum {
  _TCB_WAITING  =  _BOSS_FALSE,     /* �ñ׳� ��� ���� */
  _TCB_LISTING  =  _BOSS_TRUE,      /* ������ ������    */
} _tcb_state_t;

typedef struct boss_tcb_struct {  /* [ TCB (Task Control Block) ����ü ] */  
  struct boss_tcb_struct *next;           /* �����ٷ� ����Ʈ ��ũ */
  _tcb_state_t      state;                /* �½�ũ ���� (Waiting or Listing) */
  
  boss_prio_t       prio;                 /* �켱����     */
  
  boss_sigs_t       sigs;                 /* ���� �ñ׳�  */
  boss_sigs_t       wait;                 /* ��� �ñ׳�  */
  
  boss_stk_t        *sp;                  /* ���� ������  */
  
  #ifdef _BOSS_SPY_
  boss_uptr_t       context_num;
  
  boss_stk_t        *sp_finis;
  boss_stk_t        *sp_peak;
  boss_stk_t        *sp_begin;
  
  boss_u32_t        cpu_enter;
  boss_u32_t        cpu_total;
  #endif
  
  #ifdef _BOSS_TCB_NAME_SIZE
  char        name[_BOSS_TCB_NAME_SIZE];  /* TCB Name */
  #endif
} boss_tcb_t;


/*===========================================================================*/
/*                            FUNCTION PROTOTYPES                            */
/*---------------------------------------------------------------------------*/
void Boss_init(void (*idle_task)(void *), boss_tcb_t *idle_tcb,
                                  boss_stk_t *sp_base, boss_uptr_t stk_bytes);

void Boss_start(void);

boss_tcb_t *Boss_self(void);

boss_sigs_t Boss_wait(boss_sigs_t wait_sigs);
void Boss_sigs_send(boss_tcb_t *p_tcb, boss_sigs_t sigs);
void Boss_sigs_clear(boss_tcb_t *p_tcb, boss_sigs_t sigs);
boss_sigs_t Boss_sigs_receive(void);

void Boss_task_create( void (*task)(void *p_arg), void *p_arg, 
                        boss_tcb_t *p_tcb, boss_prio_t prio, 
                        boss_stk_t *sp_base, boss_uptr_t stk_bytes,
                        const char *name );


void Boss_task_delete(void);
void Boss_task_priority(boss_tcb_t *p_tcb, boss_prio_t new_prio);

void _Boss_sched_lock(void);
void _Boss_sched_free(void);
boss_reg_t Boss_sched_locking(void);

#endif  /* _BOSS_KERNEL_H_ */
