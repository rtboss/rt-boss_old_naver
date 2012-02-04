/*
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*                                                                             *
*                              RT-BOSS (Timer)                                *
*                                                                             *
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*/

/*===========================================================================*/
/*                               INCLUDE FILE                                */
/*---------------------------------------------------------------------------*/
#include "Boss.h"
#include "Boss_Tmr.h"

/*===========================================================================*/
/*                      DEFINITIONS & TYPEDEFS & MACROS                      */
/*---------------------------------------------------------------------------*/


/*===========================================================================*/
/*                             GLOBAL VARIABLES                              */
/*---------------------------------------------------------------------------*/
/*static*/ boss_tmr_t   *_boss_timer_list = _BOSS_NULL; /* 카운트할 타이머 리스트 */

/*===========================================================================*/
/*                            FUNCTION PROTOTYPES                            */
/*---------------------------------------------------------------------------*/

/*===========================================================================
    _   B O S S _ T I M E R _ T I C K
---------------------------------------------------------------------------*/
void _Boss_timer_tick(boss_tmr_ms_t tick_ms)
{
  boss_tmr_t  *p_prev;
  boss_tmr_t  *p_tmr;
  boss_tmr_t  *p_done_list = _BOSS_NULL;
  
  BOSS_IRQ_DISABLE();
  p_prev  = _BOSS_NULL;
  p_tmr   = _boss_timer_list;
  
  while(p_tmr != _BOSS_NULL)
  {    
    if(tick_ms < p_tmr->tmr_ms)
    {
      p_tmr->tmr_ms = p_tmr->tmr_ms - tick_ms;
      
      p_prev  = p_tmr;
      p_tmr   = p_tmr->next;
    }
    else  /* 타이머 완료 */
    {
      boss_tmr_t  *p_done;
      
      if(p_prev == _BOSS_NULL) {
        _boss_timer_list = p_tmr->next;
      } else {
        p_prev->next = p_tmr->next;
      }
      
      p_done  = p_tmr;
      p_tmr   = p_tmr->next;
      
      p_done->next = p_done_list;
      p_done_list = p_done;
    }
  }
  BOSS_IRQ_RESTORE();

  while(p_done_list != _BOSS_NULL)        /* 완료 타이머 처리 */
  {
    boss_tmr_t  *p_done = p_done_list;
    p_done_list = p_done->next;

    if(p_done->sig != 0) {                /* 시그널 처리 */
      BOSS_ASSERT(p_done->p_tcb != _BOSS_NULL);
      Boss_send(p_done->p_tcb, p_done->sig);
    }

    if(p_done->tmr_cb != _BOSS_NULL) {    /* 콜백 함수 실행 */
      p_done->tmr_cb(p_done);
    }
  }
}



/*===========================================================================
    B O S S _ T M R _ S T A R T
---------------------------------------------------------------------------*/
void Boss_tmr_start( boss_tmr_t *p_tmr, boss_tmr_ms_t tmr_ms, 
          boss_tcb_t  *p_tcb, boss_sigs_t sig, void (*tmr_cb)(boss_tmr_t *) )
{
  BOSS_ASSERT(p_tmr != _BOSS_FALSE);
  BOSS_ASSERT(_BOSS_NULL == Boss_tmr_stop(p_tmr) ); /* 중복 방지 */
  
  p_tmr->tmr_ms = tmr_ms;
  
  p_tmr->p_tcb  = p_tcb;
  p_tmr->sig    = sig;

  p_tmr->tmr_cb = tmr_cb;

  BOSS_IRQ_DISABLE();
  p_tmr->next = _boss_timer_list;   /* 타이머 추가 */
  _boss_timer_list = p_tmr;
  
  Boss_sigs_clear(p_tcb, sig);
  BOSS_IRQ_RESTORE();
}


/*===========================================================================
    B O S S _ T M R _ S T O P
---------------------------------------------------------------------------*/
boss_tmr_t *Boss_tmr_stop(boss_tmr_t *p_tmr)
{
  boss_tmr_t *p_prev;
  boss_tmr_t *p_find;

  BOSS_IRQ_DISABLE();
  p_prev = _BOSS_NULL;
  p_find = _boss_timer_list;
  while( (p_find != _BOSS_NULL) && (p_find != p_tmr) )
  {
    p_prev = p_find;
    p_find = p_find->next;
  }

  if(p_find != _BOSS_NULL)        /* 타이머 제거 */
  {
    if(p_prev == _BOSS_NULL) {
      _boss_timer_list = p_find->next;
    } else {
      p_prev->next = p_find->next;
    }
  }
  BOSS_IRQ_RESTORE();

  return p_find;      /* "p_tmr" or "_BOSS_NULL" */
}



/*===========================================================================
    B O S S _ S L E E P
---------------------------------------------------------------------------*/
void Boss_sleep(boss_tmr_ms_t wait_ms)
{
  boss_tmr_t sleep_tmr;

  Boss_tmr_start(&sleep_tmr, wait_ms, Boss_self(), BOSS_SIG_SLEEP, _BOSS_NULL);

  (void)Boss_wait(BOSS_SIG_SLEEP);
}


/*===========================================================================
    B O S S _ W A I T _ S L E E P
---------------------------------------------------------------------------*/
boss_sigs_t Boss_wait_sleep(boss_sigs_t wait_sigs,  boss_tmr_ms_t wait_ms)
{
  boss_sigs_t   recv_sigs;
  boss_tmr_t    sleep_tmr;
  
  Boss_tmr_start(&sleep_tmr, wait_ms, Boss_self(), BOSS_SIG_SLEEP, _BOSS_NULL);

  recv_sigs = Boss_wait(wait_sigs | BOSS_SIG_SLEEP);

  if( (recv_sigs & BOSS_SIG_SLEEP) == 0 ) {
      Boss_tmr_stop(&sleep_tmr);
  }
  
  return recv_sigs;
}

