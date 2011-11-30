#ifndef _BOSS_Q_MAIL_H_
#define _BOSS_Q_MAIL_H_
/*
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*                                                                             *
*                              RT-BOSS (Mail Queue)                           *
*                                                                             *
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*/

/*===========================================================================*/
/*                               INCLUDE FILE                                */
/*---------------------------------------------------------------------------*/
#include "Boss_Kernel.h"

/*===========================================================================*/
/*                      DEFINITIONS & TYPEDEFS & MACROS                      */
/*---------------------------------------------------------------------------*/
#define _MBOX_FAIL_TIMEOUT  (boss_uptr_t)(0xFFFFFFFF)   /* (32bit=0xFFFFFFFF) / (16bit=0xFFFF) */
#define _MBOX_FAIL_EXECUTE  (boss_uptr_t)(0xFFFFFFFE)   /* (32bit=0xFFFFFFFE) / (16bit=0xFFFE) */
#define _MBOX_FAILURE       (boss_uptr_t)(0xFFFFFFFD)   /* (32bit=0xFFFFFFFD) / (16bit=0xFFFD) */

typedef enum {
  _MAIL_NONE  = 0,
  _MAIL_EXE_READY,
  _MAIL_EXECUTING,
  _MAIL_EXE_STOP,
} _mail_cond_t;


typedef struct _mbox_head_struct {
  _mail_cond_t      mail_cond;
  boss_prio_t       prio;
  boss_tcb_t        *sender;
  boss_uptr_t       *p_result;      /* p_result != NULL ? Pend : Send */
  
  struct _mbox_head_struct  *next;
} _mbox_head_t;


typedef struct {
  boss_tcb_t      *mail_q_tcb;
  boss_sigs_t     mail_q_sig;

  _mbox_head_t    *mbox_list;
} boss_mail_q_t;

/*===========================================================================*/
/*                            FUNCTION PROTOTYPES                            */
/*---------------------------------------------------------------------------*/
void Boss_mail_queue_init(boss_mail_q_t *mail_q, boss_sigs_t mail_q_sig);

void *Boss_mbox_alloc(boss_uptr_t size);

void Boss_mbox_send(boss_mail_q_t *mail_q, void *mbox);
boss_uptr_t Boss_mbox_pend(boss_mail_q_t *mail_q, void *mbox,
                                                  boss_tmr_t wait_time_ms);

void *Boss_mbox_receive(boss_mail_q_t *mail_q);
void Boss_mbox_done(void *mbox, boss_uptr_t result);

boss_tcb_t *Boss_mbox_sender_tcb(void *mbox);

#endif  /* _BOSS_Q_MAIL_H_ */
