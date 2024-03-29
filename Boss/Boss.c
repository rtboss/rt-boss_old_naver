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
#include "Boss.h"

/*===========================================================================*/
/*                      DEFINITIONS & TYPEDEFS & MACROS                      */
/*---------------------------------------------------------------------------*/
#define _SCHED_LOCKING_MAX         10

/*===========================================================================*/
/*                             GLOBAL VARIABLES                              */
/*---------------------------------------------------------------------------*/
/*static*/ boss_tcb_t *_current_tcb     = _BOSS_NULL;
/*static*/ boss_tcb_t *_sched_tcb_list  = _BOSS_NULL;

/*static*/ boss_reg_t _sched_locking    = 0;

/*===========================================================================*/
/*                            FUNCTION PROTOTYPES                            */
/*---------------------------------------------------------------------------*/
void _Boss_start_schedule(void);
void _Boss_context_switch(void);
boss_stk_t *_Boss_stk_init( void (*task)(void *p_arg), void *p_arg,
                                boss_stk_t *sp_base,  boss_uptr_t stk_bytes);

#ifdef _BOSS_SPY_
void _Boss_spy_context(boss_tcb_t *curr_tcb, boss_tcb_t *best_tcb);
void _Boss_spy_setup(boss_tcb_t *p_tcb, boss_stk_t *sp_base, boss_uptr_t bytes);
#endif

/*===========================================================================
    B O S S _ S E L F
---------------------------------------------------------------------------*/
boss_tcb_t *Boss_self(void)
{
  return _current_tcb;
}


/*===========================================================================
    _   B O S S _ T C B _ I N I T
---------------------------------------------------------------------------*/
static void _Boss_tcb_init( boss_tcb_t *p_tcb, boss_prio_t prio,
                            void (*task)(void *p_arg), void *p_arg, 
                            boss_stk_t *sp_base, boss_uptr_t stk_bytes,
                            const char *name )
{
  #ifdef _BOSS_SPY_
  _Boss_spy_setup(p_tcb, sp_base, stk_bytes);
  #endif

  p_tcb->state  = _TCB_WAITING;
  p_tcb->prio   = prio;
  
  p_tcb->sigs   = 0;
  p_tcb->wait   = 0;
  
  p_tcb->sp     = _Boss_stk_init(task, p_arg, sp_base, stk_bytes);  
  
  p_tcb->next   = _BOSS_NULL;
  
  #ifdef _BOSS_TCB_NAME_SIZE
  {
    int  i;
    for(i = 0; i < _BOSS_TCB_NAME_SIZE; i++) {
      p_tcb->name[i] = name[i];
    }
    p_tcb->name[_BOSS_TCB_NAME_SIZE - 1] = 0;
  }
  #endif
}


/*===========================================================================
    B O S S _ I N I T
---------------------------------------------------------------------------*/
void Boss_init(void (*idle_task)(void *), boss_tcb_t *idle_tcb,
                                  boss_stk_t *sp_base, boss_uptr_t stk_bytes)
{
  BOSS_ASSERT( (_sched_tcb_list == _BOSS_NULL) && (_sched_locking == 0) );
  
  BOSS_IRQ_DISABLE();
  _sched_locking  = 1;              /* 스케줄링 금지 */

  _Boss_tcb_init(idle_tcb, PRIO_BOSS_IDLE, idle_task, _BOSS_NULL,
                                                sp_base, stk_bytes, "Idle");
  
  idle_tcb->state = _TCB_LISTING;
  _sched_tcb_list = idle_tcb;
  BOSS_IRQ_RESTORE();
}


/*===========================================================================
    B O S S _ S T A R T
---------------------------------------------------------------------------*/
void Boss_start(void)
{
  BOSS_ASSERT( (_sched_locking == 1) && (_current_tcb == _BOSS_NULL) );
  
  BOSS_IRQ_DISABLE();
  _current_tcb = _sched_tcb_list;               /* Best TCB       */
  _sched_locking = 0;                           /* 스케줄링 허용  */  
  BOSS_IRQ_RESTORE();
  
  _Boss_start_schedule();                       /* 스케줄러 시작  */  
}


/*===========================================================================
    _   B O S S _ S T A R T _ T C B _ S P
---------------------------------------------------------------------------*/
boss_stk_t *_Boss_start_tcb_sp(void)
{
  boss_stk_t *start_tcb_sp;

  start_tcb_sp = _current_tcb->sp;
  return start_tcb_sp;
}


/*===========================================================================
    _ B O S S _ S W I T C H _ C U R R E N T _ T C B
---------------------------------------------------------------------------*/
boss_stk_t *_Boss_switch_current_tcb(boss_stk_t *cur_task_sp)
{
  BOSS_ASSERT(_sched_locking == 0);
  
  _current_tcb->sp = cur_task_sp;       /* 실행중인 Task SP */

  #ifdef _BOSS_SPY_
  _Boss_spy_context(_current_tcb, _sched_tcb_list);
  #endif
  
  _current_tcb = _sched_tcb_list;       /* Current TCB 변경 */
  
  cur_task_sp = _current_tcb->sp;       /* 실행할 Task SP   */
  
  return cur_task_sp;
}


/*===========================================================================
    B O S S _ S C H E D U L E
---------------------------------------------------------------------------*/
static void _Boss_schedule(void)
{
  if( (_sched_locking == 0) && (_current_tcb != _sched_tcb_list) )
  {
    _Boss_context_switch();   /* 문맥 전환 */
  }
}


/*===========================================================================
    _   B O S S _ S C H E D _ L I S T _ I N S E R T
---------------------------------------------------------------------------*/
static void _Boss_sched_list_insert(boss_tcb_t *p_tcb)
{
  BOSS_ASSERT( (_BOSS_IRQ_() != 0) && (_sched_tcb_list != _BOSS_NULL)
            && (p_tcb->next == _BOSS_NULL) && (p_tcb->state == _TCB_WAITING) );
  
  if(p_tcb->prio < _sched_tcb_list->prio)
  {
    p_tcb->next = _sched_tcb_list;
    _sched_tcb_list = p_tcb;
  }
  else
  {
    boss_tcb_t *p_prev = _sched_tcb_list;
    boss_tcb_t *p_next = p_prev->next;
    
    BOSS_ASSERT(p_next != _BOSS_NULL);

    while(p_next->prio <= p_tcb->prio)
    {
      p_prev = p_next;
      p_next = p_next->next;
      BOSS_ASSERT(p_next != _BOSS_NULL);
    }
    p_prev->next = p_tcb;
    p_tcb->next = p_next;
  }
  
  p_tcb->state = _TCB_LISTING;        /* 스케줄러 리스트에 추가됨 */
}


/*===========================================================================
    _   B O S S _ S C H E D _ L I S T _ R E M O V E
---------------------------------------------------------------------------*/
static void _Boss_sched_list_remove(boss_tcb_t *p_tcb)
{
  BOSS_ASSERT( (_BOSS_IRQ_() != 0) && (p_tcb->next != _BOSS_NULL)
                && (p_tcb->state == _TCB_LISTING) );
  
  if(_sched_tcb_list == p_tcb)
  {
    _sched_tcb_list = p_tcb->next;
    BOSS_ASSERT(_sched_tcb_list != _BOSS_NULL);
  }
  else
  {
    boss_tcb_t *p_find = _sched_tcb_list;
    
    while(p_find->next != p_tcb)
    {
      p_find = p_find->next;
      BOSS_ASSERT(p_find != _BOSS_NULL);
    }
    
    p_find->next = p_tcb->next;
  }

  p_tcb->state  = _TCB_WAITING;   /* 스케줄러 리스트에서 제거됨 */
  p_tcb->next   = _BOSS_NULL;
}



/*
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*                               [ Signal ]                                    *
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*/

/*===========================================================================
    B O S S _ W A I T
---------------------------------------------------------------------------*/
boss_sigs_t Boss_wait(boss_sigs_t wait_sigs)
{
  boss_tcb_t  *cur_tcb;

  BOSS_ASSERT( (_BOSS_IRQ_() == 0) && (_BOSS_ISR_() == 0)
                && (Boss_sched_locking() == 0) && (wait_sigs != 0) );
  
  cur_tcb = Boss_self();
  cur_tcb->wait = wait_sigs;

  BOSS_IRQ_DISABLE();
  if( (cur_tcb->sigs & wait_sigs) == 0 )  /* 실행할 시그널이 없을면 */
  {
    _Boss_sched_list_remove(cur_tcb);     /* 스케줄러 리스트에서 제거 */
    _Boss_context_switch();               /* 문맥 전환 실행           */
  }
  BOSS_IRQ_RESTORE();
  
  return Boss_sigs_receive();
}



/*===========================================================================
    B O S S _ S E N D
---------------------------------------------------------------------------*/
void Boss_send(boss_tcb_t *p_tcb, boss_sigs_t sigs)
{  
  BOSS_IRQ_DISABLE();
  p_tcb->sigs = p_tcb->sigs | sigs;

  if( (p_tcb->state == _TCB_WAITING) && (p_tcb->wait & sigs) )
  {
    _Boss_sched_list_insert(p_tcb);
  }
  BOSS_IRQ_RESTORE();

  _Boss_schedule();       /* 문맥전환 실행  */
}


/*===========================================================================
    B O S S _ S I G S _ R E C E I V E
---------------------------------------------------------------------------*/
boss_sigs_t Boss_sigs_receive(void)
{
  boss_sigs_t recv_sigs;
  boss_tcb_t  *cur_tcb = Boss_self();
  
  BOSS_IRQ_DISABLE();
  recv_sigs     = cur_tcb->sigs & cur_tcb->wait;
  cur_tcb->sigs = cur_tcb->sigs & ~recv_sigs;   /* 수신한 시그널 클리어 */
  BOSS_IRQ_RESTORE();
  
  return recv_sigs;
}


/*===========================================================================
    B O S S _ S I G S _ C L E A R
---------------------------------------------------------------------------*/
void Boss_sigs_clear(boss_tcb_t *p_tcb, boss_sigs_t sigs)
{
  BOSS_IRQ_DISABLE();
  p_tcb->sigs = p_tcb->sigs & ~sigs;
  BOSS_IRQ_RESTORE();
}



/*
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*                                [ TASK ]                                     *
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*/

/*===========================================================================
    B O S S _ T A S K _ C R E A T E
---------------------------------------------------------------------------*/
void Boss_task_create(  void (*task)(void *p_arg), void *p_arg, 
                        boss_tcb_t *p_tcb, boss_prio_t prio, 
                        boss_stk_t *sp_base, boss_uptr_t stk_bytes,
                        const char *name )
{
  BOSS_ASSERT(_sched_tcb_list != _BOSS_NULL);

  _Boss_tcb_init(p_tcb, prio, task, p_arg, sp_base, stk_bytes, name);
  
  BOSS_IRQ_DISABLE();
  _Boss_sched_list_insert(p_tcb);
  BOSS_IRQ_RESTORE();
  
  _Boss_schedule();
}


/*===========================================================================
    B O S S _ T A S K _ D E L E T E
---------------------------------------------------------------------------*/
void Boss_task_delete(void)
{  
  boss_tcb_t  *cur_tcb;
  
  BOSS_ASSERT( (_BOSS_IRQ_() == 0) && (_BOSS_ISR_() == 0)
                && (Boss_sched_locking() == 0) );
  
  cur_tcb = Boss_self();
  
  BOSS_IRQ_DISABLE();
  _Boss_sched_list_remove(cur_tcb);
  cur_tcb->wait   = 0;
  BOSS_IRQ_RESTORE();

  _Boss_schedule();
}


/*===========================================================================
    B O S S _ T A S K _ P R I O R I T Y
---------------------------------------------------------------------------*/
void Boss_task_priority(boss_tcb_t *p_tcb, boss_prio_t new_prio)
{
  BOSS_ASSERT(new_prio < PRIO_BOSS_IDLE);
  
  BOSS_IRQ_DISABLE();
  p_tcb->prio = new_prio;
  
  if(p_tcb->state == _TCB_LISTING)    /* schedule list update */
  {
    _Boss_sched_list_remove(p_tcb);
    _Boss_sched_list_insert(p_tcb);
  }
  BOSS_IRQ_RESTORE();

  _Boss_schedule();
}


/*
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*                          [ Schedule Lock ]                                  *
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*/

/*===========================================================================
    _ B O S S _ S C H E D _ L O C K
---------------------------------------------------------------------------*/
void _Boss_sched_lock(void)
{
  BOSS_ASSERT(_sched_locking < _SCHED_LOCKING_MAX);
  
  BOSS_IRQ_DISABLE();
  _sched_locking++;
  BOSS_IRQ_RESTORE();
}


/*===========================================================================
    _   B O S S _ S C H E D _ F R E E
---------------------------------------------------------------------------*/
void _Boss_sched_free(void)
{
  BOSS_ASSERT(_sched_locking > 0);

  BOSS_IRQ_DISABLE();
  _sched_locking--;
  BOSS_IRQ_RESTORE();

  _Boss_schedule();
}


/*===========================================================================
    B O S S _ S C H E D _ L O C K I N G
---------------------------------------------------------------------------*/
boss_reg_t Boss_sched_locking(void)
{
  return _sched_locking;
}

