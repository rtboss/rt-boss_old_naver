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
#include "Boss_Kernel.h"

/*===========================================================================*/
/*                      DEFINITIONS & TYPEDEFS & MACROS                      */
/*---------------------------------------------------------------------------*/
#define _SCHED_LOCK_MAX         5
#define _PRIORITY_RISE_MAX      10

/*===========================================================================*/
/*                             GLOBAL VARIABLES                              */
/*---------------------------------------------------------------------------*/
/*static*/ boss_tcb_t *_current_tcb     = _BOSS_NULL;
/*static*/ boss_tcb_t *_sched_tcb_list  = _BOSS_NULL;
/*static*/ boss_reg_t _sched_locking    = 0;

/*static*/ boss_tcb_t *_sleep_tcb_list  = _BOSS_NULL;

/*===========================================================================*/
/*                            FUNCTION PROTOTYPES                            */
/*---------------------------------------------------------------------------*/
void _Boss_start_schedule(boss_stk_t *cur_task_sp);
void _Boss_context_switch(void);
boss_stk_t *_Boss_stk_init(void (*executer)(void), boss_stk_t *sp_base, 
                                                        boss_uptr_t stk_bytes);

static void _Boss_sleep_list_remove(boss_tcb_t *p_tcb);


/*===========================================================================
    B O S S _ S E L F
---------------------------------------------------------------------------*/
boss_tcb_t *Boss_self(void)
{
  return _current_tcb;
}


/*===========================================================================
    B O S S _ I N I T
---------------------------------------------------------------------------*/
void Boss_init(boss_tcb_t *idle_tcb, void (*idle_exe)(void),
                                  boss_stk_t *sp_base, boss_uptr_t stk_bytes)
{
  BOSS_ASSERT(_sched_tcb_list == _BOSS_NULL);
  BOSS_ASSERT(_sched_locking == _BOSS_FALSE);
  BOSS_ASSERT(_sleep_tcb_list == _BOSS_NULL);
  
  BOSS_IRQ_LOCK();
  _sched_locking = _SCHED_LOCK_MAX;                 /* 스케줄러 시작 전 */
  
  Boss_task_create(idle_tcb, idle_exe, PRIO_BOSS_IDLE, sp_base, stk_bytes, _BOSS_NULL);
  BOSS_IRQ_FREE();
}


/*===========================================================================
    B O S S _ S T A R T
---------------------------------------------------------------------------*/
void Boss_start(void (*init_conf)(void))
{
  boss_stk_t *cur_task_sp;
  
  BOSS_ASSERT(_mcu_isr_running() == _BOSS_FALSE);
  BOSS_ASSERT(_sched_locking == _SCHED_LOCK_MAX);
  BOSS_ASSERT(_current_tcb == _BOSS_NULL);
  
  BOSS_IRQ_LOCK();
  _current_tcb = _sched_tcb_list;                 /* Current TCB 최초 시행 */
  _current_tcb->status = TASK_RUNNING;
  cur_task_sp = _current_tcb->sp;                 /* Current 태스크 스택 포인터 */
  
  if(init_conf != _BOSS_NULL) {
    init_conf();
  }

  _sched_locking = 0;                             /* 스케줄러 실행 가능 */  
  _Boss_start_schedule(cur_task_sp);              /* 스케줄러 시작 */
  
  BOSS_ASSERT(_BOSS_FALSE); /* _Boss_start_schedule() 이후에는 실행되지 않는다. */  
  BOSS_IRQ_FREE();
}


/*===========================================================================
    _ B O S S _ S W I T C H _ C U R R E N T _ T C B
---------------------------------------------------------------------------*/
boss_stk_t *_Boss_switch_current_tcb(boss_stk_t *cur_task_sp)
{
  BOSS_ASSERT(_mcu_irq_hold() != _BOSS_FALSE);
  BOSS_ASSERT(_sched_locking == _BOSS_FALSE);
  BOSS_ASSERT(_sched_tcb_list->status > TASK_WAITING);
  BOSS_ASSERT(_current_tcb != _BOSS_NULL);

  _current_tcb->sp = cur_task_sp;

  if(_current_tcb->status == TASK_RUNNING) {
    _current_tcb->status = TASK_SUSPEND;
  }

  _current_tcb = _sched_tcb_list;       // Current TCB 교체
  _current_tcb->status = TASK_RUNNING;  // Current 태스크 실행중
  cur_task_sp = _current_tcb->sp;       // Current 태스크 스택 포인터
  
  return cur_task_sp;
}


/*===========================================================================
    B O S S _ S C H E D U L E
---------------------------------------------------------------------------*/
static void _Boss_schedule(void)
{
  if(_sched_locking == _BOSS_FALSE)
  {
    if(_current_tcb != _sched_tcb_list)
    {
      _Boss_context_switch();   /* 문맥 전환 */
    }
  }
}


/*===========================================================================
    _ B O S S _ S C H E D _ L O C K
---------------------------------------------------------------------------*/
void _Boss_sched_lock(void)
{
  BOSS_ASSERT(_sched_locking < _SCHED_LOCK_MAX);
  
  BOSS_IRQ_LOCK();
  _sched_locking++;
  BOSS_IRQ_FREE();
}


/*===========================================================================
    _   B O S S _ S C H E D _ F R E E
---------------------------------------------------------------------------*/
void _Boss_sched_free(void)
{
  BOSS_ASSERT(_sched_locking != 0);

  BOSS_IRQ_LOCK();
  _sched_locking--;
  BOSS_IRQ_FREE();

  _Boss_schedule();
}


/*===========================================================================
    B O S S _ S C H E D _ L O C K I N G
---------------------------------------------------------------------------*/
boss_reg_t Boss_sched_locking(void)
{
  return _sched_locking;
}


/*===========================================================================
    _   B O S S _ S C H E D _ L I S T _ I N S E R T
---------------------------------------------------------------------------*/
static void _Boss_sched_list_insert(boss_tcb_t *p_tcb)
{
  BOSS_ASSERT(_mcu_irq_hold() != _BOSS_FALSE);
  BOSS_ASSERT(_sched_tcb_list != _BOSS_NULL);
  BOSS_ASSERT(p_tcb->status == TASK_READY);
  BOSS_ASSERT(p_tcb->prio < PRIO_BOSS_IDLE);
  BOSS_ASSERT(p_tcb->prio <= p_tcb->prio_orig);

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
}


/*===========================================================================
    _   B O S S _ S C H E D _ L I S T _ R E M O V E
---------------------------------------------------------------------------*/
static void _Boss_sched_list_remove(boss_tcb_t *p_tcb)
{
  BOSS_ASSERT(_mcu_irq_hold() != _BOSS_FALSE);
  BOSS_ASSERT(p_tcb->status > TASK_WAITING);
  BOSS_ASSERT(_sched_tcb_list != _BOSS_NULL);

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
  
  p_tcb->next = _BOSS_NULL;
}


/*===========================================================================
    _   B O S S _ S C H E D _ L I S T _ U P D A T E
---------------------------------------------------------------------------*/
static void _Boss_sched_list_update(boss_tcb_t *p_tcb)
{
  _task_status_t status_backup; /* _Boss_sched_list_insert() 함수에는 
                                  "TASK_READY" 만 추가 할수 있어서
                                  p_tcb->status 를 임시 변경 및 복원한다. */
  
  BOSS_ASSERT(_mcu_irq_hold() != _BOSS_FALSE);
  BOSS_ASSERT(p_tcb->next != _BOSS_NULL);
  BOSS_ASSERT(p_tcb->status > TASK_WAITING);

  _Boss_sched_list_remove(p_tcb);

  status_backup = p_tcb->status;
  
  p_tcb->status = TASK_READY;
  _Boss_sched_list_insert(p_tcb);
  
  p_tcb->status = status_backup;
}



/*===========================================================================
    _ B O S S _ P R I O R I T Y _ R I S E
---------------------------------------------------------------------------*/
void _Boss_priority_rise(boss_prio_t up_prio, boss_tcb_t *p_tcb,
                                                        boss_byte_t inc_rise)
{
  BOSS_ASSERT((p_tcb->prio_rise + inc_rise) < _PRIORITY_RISE_MAX);

  BOSS_IRQ_LOCK();
  p_tcb->prio_rise = p_tcb->prio_rise + inc_rise;
  
  if(up_prio < p_tcb->prio)
  {
    p_tcb->prio = up_prio;
    if(p_tcb->status > TASK_WAITING)
    {
      _Boss_sched_list_update(p_tcb);
    }
  }
  BOSS_IRQ_FREE();
}


/*===========================================================================
    _ B O S S _ P R I O R I T Y _ F A L L _ O R I G I N A L
---------------------------------------------------------------------------*/
void _Boss_priority_fall_original(boss_tcb_t *p_tcb, boss_byte_t dec_rise)
{
  BOSS_ASSERT(dec_rise <= p_tcb->prio_rise);

  BOSS_IRQ_LOCK();
  p_tcb->prio_rise = p_tcb->prio_rise - dec_rise;

  if( (p_tcb->prio_rise == 0) && (p_tcb->prio != p_tcb->prio_orig) )
  {
    p_tcb->prio = p_tcb->prio_orig;
    
    if(p_tcb->status > TASK_WAITING)
    {
      _Boss_sched_list_update(p_tcb);
    }
  }
  BOSS_IRQ_FREE();
}



/*===========================================================================
    B O S S _ T A S K _ P R I O R I T Y
---------------------------------------------------------------------------*/
void Boss_task_priority(boss_tcb_t *p_tcb, boss_prio_t new_prio)
{
  BOSS_ASSERT(new_prio < PRIO_BOSS_IDLE);
  
  BOSS_IRQ_LOCK();
  if(p_tcb->prio_rise != 0)
  {
    p_tcb->prio_orig = new_prio;
    if(new_prio < p_tcb->prio)
    {
      p_tcb->prio = new_prio;
    }
  }
  else /* p_tcb->prio_rise == 0 */
  {
    p_tcb->prio_orig  = new_prio;
    p_tcb->prio       = new_prio;
  }
  
  if(p_tcb->status > TASK_WAITING)
  {
    _Boss_sched_list_update(p_tcb);
  }
  BOSS_IRQ_FREE();

  _Boss_schedule();
}



/*===========================================================================
    B O S S _ T A S K _ C R E A T E
---------------------------------------------------------------------------*/
void Boss_task_create( boss_tcb_t *p_tcb, void (*executer)(void), 
      boss_prio_t prio, boss_stk_t *sp_base, boss_uptr_t stk_bytes, void *args)
{
  BOSS_IRQ_LOCK();
  BOSS_ASSERT(p_tcb->status <=  TASK_WAITING);
  p_tcb->status = TASK_DORMANT;
  BOSS_IRQ_FREE();
  
  p_tcb->prio       = prio;
  p_tcb->prio_orig  = prio;
  p_tcb->prio_rise  = 0;
  
  p_tcb->sp         = _Boss_stk_init(executer, sp_base, stk_bytes);
  p_tcb->sp_base    = sp_base;
  p_tcb->stk_size   = stk_bytes;

  p_tcb->sigs       = 0;
  p_tcb->sigs_wait  = 0;

  p_tcb->sleep_ms   = 0;
  
  p_tcb->args       = args;
  
  p_tcb->next       = _BOSS_NULL;
  
  if(_sched_tcb_list == _BOSS_NULL)
  {
    /* IDLE Task */
    BOSS_ASSERT(p_tcb->prio == PRIO_BOSS_IDLE);
    BOSS_ASSERT(Boss_sched_locking() != _BOSS_FALSE);
    
    BOSS_IRQ_LOCK();
    p_tcb->status   = TASK_READY;
    _sched_tcb_list = p_tcb;
    BOSS_IRQ_FREE();
  }
  else
  {
    BOSS_ASSERT(p_tcb->prio < PRIO_BOSS_IDLE);
    
    BOSS_IRQ_LOCK();
    p_tcb->status     = TASK_READY;
    _Boss_sched_list_insert(p_tcb);
    BOSS_IRQ_FREE();
  }
  
  _Boss_schedule();
}


/*===========================================================================
    B O S S _ T A S K _ D E L E T E
---------------------------------------------------------------------------*/
void Boss_task_delete(boss_tcb_t *p_tcb)
{
  BOSS_ASSERT(_mcu_isr_running() == _BOSS_FALSE);
  BOSS_ASSERT(p_tcb->prio_rise == 0);
  BOSS_ASSERT(p_tcb->prio < PRIO_BOSS_IDLE);
  
  BOSS_IRQ_LOCK();
  if(p_tcb->status > TASK_WAITING) {
    _Boss_sched_list_remove(p_tcb);
  }

  if(p_tcb->sleep_ms != 0) {
    _Boss_sleep_list_remove(p_tcb);
  }
  
  p_tcb->status     = TASK_DORMANT;
  p_tcb->sigs_wait  = 0;
  BOSS_IRQ_FREE();

  _Boss_schedule();
}





/*===========================================================================
    _   B O S S _ T I C K _ S L E E P
---------------------------------------------------------------------------*/
void _Boss_tick_sleep(boss_tmr_t tick_ms)
{  
  BOSS_ASSERT( Boss_sched_locking() != _BOSS_FALSE );

  BOSS_IRQ_LOCK();
  if(_sleep_tcb_list != _BOSS_NULL)
  {
    boss_tcb_t *p_prev;
    boss_tcb_t *p_tcb;

    p_prev = _BOSS_NULL;
    p_tcb  = _sleep_tcb_list;

    while( p_tcb != _BOSS_NULL )
    {
      if(tick_ms < p_tcb->sleep_ms)
      {
        p_tcb->sleep_ms = p_tcb->sleep_ms - tick_ms;
        
        p_prev = p_tcb;
        p_tcb  = p_tcb->next;
      }
      else
      {
        boss_tcb_t *p_done = p_tcb;
        
        BOSS_ASSERT(p_done->status == TASK_WAITING);
        BOSS_ASSERT(p_done->sigs_wait & BOSS_SIG_SLEEP);
        BOSS_ASSERT(p_done->sleep_ms != 0);

        p_tcb = p_tcb->next;
        if(p_prev == _BOSS_NULL) {
          _sleep_tcb_list = p_tcb;
        } else {
          p_prev->next = p_tcb;
        }
        
        p_done->sleep_ms = 0;
        Boss_sigs_send(p_done, BOSS_SIG_SLEEP);
      }
    }
  }
  BOSS_IRQ_FREE();
}


/*===========================================================================
    _   B O S S _ S L E E P _ L I S T _ R E M O V E
---------------------------------------------------------------------------*/
static void _Boss_sleep_list_remove(boss_tcb_t *p_tcb)
{ 
  BOSS_ASSERT(_mcu_irq_hold() != _BOSS_FALSE);
  BOSS_ASSERT(p_tcb->status == TASK_WAITING);
  BOSS_ASSERT(p_tcb->sigs_wait & BOSS_SIG_SLEEP);
  BOSS_ASSERT(p_tcb->sleep_ms != 0);
  BOSS_ASSERT(_sleep_tcb_list != _BOSS_NULL);

  if(_sleep_tcb_list == p_tcb)
  {
    _sleep_tcb_list = p_tcb->next;
  }
  else
  {
    boss_tcb_t *p_find = _sleep_tcb_list;
    
    while(p_find->next != p_tcb)
    {
      p_find = p_find->next;
      BOSS_ASSERT(p_find != _BOSS_NULL);
    }

    p_find->next = p_tcb->next;
  }

  p_tcb->sleep_ms = 0;
  p_tcb->next = _BOSS_NULL;
}


/*
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*                              RT-BOSS (Signal)                               *
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*/

/*===========================================================================
    B O S S _ W A I T
---------------------------------------------------------------------------*/
boss_sigs_t Boss_wait(boss_sigs_t wait_sigs, boss_tmr_t sleep_ms)
{
  boss_tcb_t  *cur_tcb;

  BOSS_ASSERT(_mcu_irq_hold() == _BOSS_FALSE);
  BOSS_ASSERT(_mcu_isr_running() == _BOSS_FALSE);
  BOSS_ASSERT( Boss_sched_locking() == _BOSS_FALSE);
  BOSS_ASSERT( (sleep_ms != 0) || (wait_sigs != BOSS_SIGS_NULL) );
  
  BOSS_ASSERT(Boss_self()->status == TASK_RUNNING);
  BOSS_ASSERT(Boss_self()->prio < PRIO_BOSS_IDLE);
  BOSS_ASSERT(Boss_self()->sleep_ms == 0);
  BOSS_ASSERT((Boss_self()->sigs & BOSS_SIG_SLEEP) != BOSS_SIG_SLEEP);
  
  cur_tcb = Boss_self();
  cur_tcb->sigs_wait = wait_sigs;

  BOSS_IRQ_LOCK();
  if( (cur_tcb->sigs & wait_sigs) == BOSS_SIGS_NULL ) /* 실행할 시그널이 없을면 */
  {
    _Boss_sched_list_remove(cur_tcb);     /* 스케줄러 리스트에서 제거 */
    cur_tcb->status = TASK_WAITING;       /* 현재 태스크 대기상태 */
    
    if(sleep_ms != 0)
    {
      cur_tcb->sigs_wait = cur_tcb->sigs_wait | BOSS_SIG_SLEEP;
      cur_tcb->sleep_ms = sleep_ms;
      
      cur_tcb->next   = _sleep_tcb_list;    /* Sleep 리스트에 추가 */
      _sleep_tcb_list = cur_tcb;
    }
  }
  BOSS_IRQ_FREE();                      /* 인터럽트 복원(활성화) */
  
  _Boss_schedule();                     /* 문맥전환 실행 */
  
  return Boss_sigs_receive();
}



/*===========================================================================
    B O S S _ S I G S _ S E N D
---------------------------------------------------------------------------*/
void Boss_sigs_send(boss_tcb_t *p_tcb, boss_sigs_t sigs)
{
  BOSS_ASSERT(Boss_self()->status == TASK_RUNNING);
  BOSS_ASSERT(p_tcb->status >= TASK_WAITING);
  
  BOSS_IRQ_LOCK();
  p_tcb->sigs = p_tcb->sigs | sigs;

  if( (p_tcb->status == TASK_WAITING) && (p_tcb->sigs_wait & sigs) )
  {
    if(p_tcb->sleep_ms != 0) {
      _Boss_sleep_list_remove(p_tcb);
    }

    p_tcb->status = TASK_READY;
    _Boss_sched_list_insert(p_tcb);
  }  
  BOSS_IRQ_FREE();

  _Boss_schedule();       /* 문맥전환 실행  */
}


/*===========================================================================
    B O S S _ S I G S _ R E C E I V E
---------------------------------------------------------------------------*/
boss_sigs_t Boss_sigs_receive(void)
{
  boss_sigs_t recv_sigs;
  boss_tcb_t  *cur_tcb = Boss_self();
  
  BOSS_IRQ_LOCK();
  recv_sigs     = cur_tcb->sigs & cur_tcb->sigs_wait;
  cur_tcb->sigs = cur_tcb->sigs & ~recv_sigs;   /* 수신한 시그널 클리어 */
  BOSS_IRQ_FREE();
  
  return recv_sigs;
}


/*===========================================================================
    B O S S _ S I G S _ C L E A R
---------------------------------------------------------------------------*/
void Boss_sigs_clear(boss_tcb_t *p_tcb, boss_sigs_t sigs)
{
  BOSS_IRQ_LOCK();
  p_tcb->sigs = p_tcb->sigs & ~sigs;
  BOSS_IRQ_FREE();
}

