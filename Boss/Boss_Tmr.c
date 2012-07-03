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
  boss_tmr_t  *p_tmr;
  
  BOSS_IRQ_DISABLE();
  p_tmr = _boss_timer_list;
  
  while(p_tmr != _BOSS_NULL)
  {
    if(tick_ms < p_tmr->tmr_ms)
    {
      p_tmr->tmr_ms = p_tmr->tmr_ms - tick_ms;
      p_tmr = p_tmr->next;
    }
    else  /* 타이머 완료 */
    {
      Boss_send(p_tmr->p_tcb, p_tmr->sig);      /* 시그널 처리 */
      
      if(p_tmr->rpt_ms != 0)                    /* 반복 타이머 */
      {
        p_tmr->tmr_ms = p_tmr->rpt_ms;
        p_tmr = p_tmr->next;
      }
      else                                      /* 타이머 제거 */
      {
        boss_tmr_t  *p_free = p_tmr;
        p_tmr = p_tmr->next;
        
        if(p_free->prev == _BOSS_NULL) {
          _boss_timer_list = p_free->next;
        } else {
          p_free->prev->next = p_free->next;
        }
        
        if(p_free->next != _BOSS_NULL) {
          p_free->next->prev = p_free->prev;
        }
        
        Boss_mfree(p_free);                   /* 메모리 해제 */
      }
    }
  }
  BOSS_IRQ_RESTORE();
}



/*===========================================================================
    B O S S _ T M R _ S T A R T
---------------------------------------------------------------------------*/
void Boss_tmr_start( boss_tcb_t *p_tcb, boss_sigs_t sig,
                                  boss_tmr_ms_t tmr_ms, boss_tmr_ms_t rpt_ms )
{
  boss_tmr_t *p_tmr;

  BOSS_ASSERT(p_tcb != _BOSS_NULL);

  p_tmr = Boss_malloc(sizeof(boss_tmr_t));

  p_tmr->p_tcb  = p_tcb;
  p_tmr->sig    = sig;

  p_tmr->tmr_ms = tmr_ms;
  p_tmr->rpt_ms = rpt_ms;

  p_tmr->prev   = _BOSS_NULL;
  p_tmr->next   = _BOSS_NULL;
  
  BOSS_IRQ_DISABLE();         /* 타이머 추가 */
  if(_boss_timer_list != _BOSS_NULL)
  {
    BOSS_ASSERT(_boss_timer_list->prev == _BOSS_NULL);
    
    _boss_timer_list->prev = p_tmr;
    p_tmr->next = _boss_timer_list;
  }
  _boss_timer_list = p_tmr;
  
  Boss_sigs_clear(p_tcb, sig);
  BOSS_IRQ_RESTORE();
}


/*===========================================================================
    B O S S _ T M R _ C A N C E L
---------------------------------------------------------------------------*/
void Boss_tmr_cancel(boss_tcb_t *p_tcb, boss_sigs_t sig )
{
  boss_tmr_t *p_find;

  BOSS_IRQ_DISABLE();
  p_find = _boss_timer_list;
  
  while( p_find != _BOSS_NULL )
  {
    if( (p_find->p_tcb == p_tcb) && (p_find->sig == sig) )  /* 타이머 제거 */
    {
      boss_tmr_t  *p_free = p_find;
      p_find = p_find->next;
      
      if(p_free->prev == _BOSS_NULL) {
        _boss_timer_list = p_free->next;
      } else {
        p_free->prev->next = p_free->next;
      }
      
      if(p_free->next != _BOSS_NULL) {
        p_free->next->prev = p_free->prev;
      }
      
      Boss_mfree(p_free);                                     /* 메모리 해제 */
    }
    else
    {
      p_find = p_find->next;
    }
  }  
  BOSS_IRQ_RESTORE();
}


/*===========================================================================
    B O S S _ S L E E P
---------------------------------------------------------------------------*/
void Boss_sleep(boss_tmr_ms_t wait_ms)
{
  Boss_tmr_start(Boss_self(), BOSS_SIG_SLEEP, wait_ms, _BOSS_FALSE);

  Boss_wait(BOSS_SIG_SLEEP);
}


/*===========================================================================
    B O S S _ W A I T _ S L E E P
---------------------------------------------------------------------------*/
boss_sigs_t Boss_wait_sleep(boss_sigs_t wait_sigs,  boss_tmr_ms_t wait_ms)
{
  boss_sigs_t recv_sigs;
  
  Boss_tmr_start(Boss_self(), BOSS_SIG_SLEEP, wait_ms, _BOSS_FALSE);

  recv_sigs = Boss_wait(wait_sigs | BOSS_SIG_SLEEP);

  if( (recv_sigs & BOSS_SIG_SLEEP) == 0 ) {
    Boss_tmr_cancel(Boss_self(), BOSS_SIG_SLEEP);
  }
  
  return recv_sigs;
}

