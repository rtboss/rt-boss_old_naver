#ifndef _BOSS_TMR_H_
#define _BOSS_TMR_H_
/*
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*                                                                             *
*                             RT-BOSS (Timer)                                 *
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
typedef struct _timer_struct {
  boss_tcb_t    *p_tcb;     /* 전송할 태스크 */
  boss_sigs_t   sig;        /* 전송할 시그널 */
  
  boss_tmr_t    alarm_ms;   /* 타이머 카운트 */
  boss_tmr_t    repeat_ms;  /* 타이머 반복 카운트 (0 : 반복하지 않음) */
  
  struct _timer_struct  *next;
} _timer_t;


/*===========================================================================*/
/*                            FUNCTION PROTOTYPES                            */
/*---------------------------------------------------------------------------*/
void Boss_tmr_create(boss_tcb_t *p_tcb, boss_sigs_t sig, 
                                  boss_tmr_t alarm_ms, boss_tmr_t repeat_ms);

void Boss_tmr_remove(boss_tcb_t *p_tcb, boss_sigs_t sig);

#endif  /* _BOSS_TMR_H_ */
