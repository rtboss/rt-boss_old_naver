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
#include "Boss_Kernel.h"
#include "Boss_Tmr.h"

/*===========================================================================*/
/*                      DEFINITIONS & TYPEDEFS & MACROS                      */
/*---------------------------------------------------------------------------*/


/*===========================================================================*/
/*                             GLOBAL VARIABLES                              */
/*---------------------------------------------------------------------------*/
/*static*/ _timer_t *_timer_list = _BOSS_NULL;  /* 카운트할 타이머 리스트 */

/*===========================================================================*/
/*                            FUNCTION PROTOTYPES                            */
/*---------------------------------------------------------------------------*/


/*===========================================================================
    B O S S _ T M R _ C R E A T E
---------------------------------------------------------------------------*/
void Boss_tmr_create(boss_tcb_t *p_tcb, boss_sigs_t sig, 
                                    boss_tmr_t alarm_ms, boss_tmr_t repeat_ms)
{
  _timer_t *p_tmr;
  
  BOSS_IRQ_LOCK();
  p_tmr = _timer_list;              
  while( (p_tmr != _BOSS_NULL)      /* 동일한 타이머가 있는지 찾는다 */
                && ( (p_tmr->p_tcb != p_tcb) || (p_tmr->sig != sig) ) )
  {
    p_tmr = p_tmr->next;
  }

  if(p_tmr != _BOSS_NULL)
  {
    BOSS_ASSERT(p_tmr->p_tcb == p_tcb);
    BOSS_ASSERT(p_tmr->sig == sig);
    
    p_tmr->alarm_ms   = alarm_ms;
    p_tmr->repeat_ms  = repeat_ms;
  }
  else
  {    
    p_tmr = _BOSS_TMR_ALLOC(sizeof(_timer_t));

    p_tmr->p_tcb  = p_tcb;
    p_tmr->sig    = sig;
    
    p_tmr->alarm_ms   = alarm_ms;
    p_tmr->repeat_ms  = repeat_ms;

    p_tmr->next = _timer_list;     /* 리스트 추가 */
    _timer_list = p_tmr;
  }
  Boss_sigs_clear(p_tcb, sig);
  BOSS_IRQ_FREE();
}


/*===========================================================================
    B O S S _ T M R _ R E M O V E
---------------------------------------------------------------------------*/
void Boss_tmr_remove(boss_tcb_t *p_tcb, boss_sigs_t sig)
{
  _timer_t *p_prev;
  _timer_t *p_tmr;
  
  BOSS_IRQ_LOCK();
  p_prev  = _BOSS_NULL;
  p_tmr   = _timer_list;  
  while( (p_tmr != _BOSS_NULL)      /* 동일한 타이머가 있는지 찾는다 */
                && ( (p_tmr->p_tcb != p_tcb) || (p_tmr->sig != sig) ) )
  {
    p_prev  = p_tmr;
    p_tmr   = p_tmr->next;
  }

  if(p_tmr != _BOSS_NULL)
  {
    if(p_prev == _BOSS_NULL)
    {
      _timer_list = p_tmr->next;
    }
    else
    {
      p_prev->next = p_tmr->next;
    }
  }
  BOSS_IRQ_FREE();

  if(p_tmr != _BOSS_NULL)
  {
    _BOSS_TMR_FREE(p_tmr);
  }
}


/*===========================================================================
    _   B O S S _ T I C K _ T I M E R
---------------------------------------------------------------------------*/
void _Boss_tick_timer(boss_tmr_t tick_ms)
{
  _timer_t *p_prev;
  _timer_t *p_tmr;
  _timer_t *p_free;
  
  BOSS_ASSERT( Boss_sched_locking() != _BOSS_FALSE );
  
  BOSS_IRQ_LOCK();
  p_prev  = _BOSS_NULL;
  p_tmr   = _timer_list;
  
  while(p_tmr != _BOSS_NULL)
  {    
    if(tick_ms < p_tmr->alarm_ms)
    {
      p_tmr->alarm_ms = p_tmr->alarm_ms - tick_ms;
      
      p_prev  = p_tmr;
      p_tmr   = p_tmr->next;
    }
    else
    {
      BOSS_ASSERT(p_tmr != _BOSS_NULL);
      Boss_sigs_send(p_tmr->p_tcb, p_tmr->sig);     /* 타이머 완료 시그널 */
      
      if(p_tmr->repeat_ms != 0)
      {
        p_tmr->alarm_ms = p_tmr->repeat_ms;         /* 타이머 반복 */

        p_prev  = p_tmr;
        p_tmr   = p_tmr->next;
      }
      else
      {
        if(p_prev == _BOSS_NULL)
        {
          _timer_list = p_tmr->next;
        }
        else
        {
          p_prev->next = p_tmr->next;
        }
        
        p_free  = p_tmr;
        p_tmr   = p_tmr->next;
        
        _BOSS_TMR_FREE(p_free);
      }
    }
  }
  BOSS_IRQ_FREE();
}

