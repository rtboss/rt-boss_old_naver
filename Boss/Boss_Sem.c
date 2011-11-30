/*
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*                                                                             *
*                            RT-BOSS (Semaphore)                              *
*                                                                             *
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*/

/*===========================================================================*/
/*                               INCLUDE FILE                                */
/*---------------------------------------------------------------------------*/
#include "Boss_Kernel.h"
#include "Boss_Sem.h"

/*===========================================================================*/
/*                      DEFINITIONS & TYPEDEFS & MACROS                      */
/*---------------------------------------------------------------------------*/


/*===========================================================================*/
/*                             GLOBAL VARIABLES                              */
/*---------------------------------------------------------------------------*/


/*===========================================================================*/
/*                            FUNCTION PROTOTYPES                            */
/*---------------------------------------------------------------------------*/
void _Boss_priority_fall_original(boss_tcb_t *p_tcb, boss_byte_t dec_rise);
void _Boss_priority_rise(boss_prio_t up_prio, boss_tcb_t *p_tcb,
                                                        boss_byte_t inc_rise);

/*===========================================================================
    B O S S _ S E M _ I N I T
---------------------------------------------------------------------------*/
void Boss_sem_init(boss_sem_t *p_sem)
{
  p_sem->init       = _BOSS_TRUE;
  p_sem->busy       = _BOSS_FALSE;
  p_sem->wait_count = 0;
  p_sem->wait_list  = _BOSS_NULL;
  p_sem->user_tcb   = _BOSS_NULL;
}


/*===========================================================================
    B O S S _ S E M _ O B T A I N
---------------------------------------------------------------------------*/
boss_reg_t Boss_sem_obtain(boss_sem_t *p_sem, boss_tmr_t wait_time_ms)
{
  boss_reg_t  _irq_store_;
  boss_reg_t  obtain = (boss_reg_t)_BOSS_SUCCESS;
  
  BOSS_ASSERT(_mcu_irq_hold() == _BOSS_FALSE);
  BOSS_ASSERT(_mcu_isr_running() == _BOSS_FALSE);
  BOSS_ASSERT(Boss_sched_locking() == _BOSS_FALSE);
  BOSS_ASSERT(p_sem->init == _BOSS_TRUE);
  BOSS_ASSERT(p_sem->user_tcb != Boss_self());   /* 이중 획득요청 */
  BOSS_ASSERT((Boss_self()->sigs & BOSS_SIG_SEM_OBTAIN) != BOSS_SIG_SEM_OBTAIN);

  _irq_store_ = _mcu_irq_lock();
  if(p_sem->busy == _BOSS_FALSE)
  {
    BOSS_ASSERT(p_sem->user_tcb == _BOSS_NULL);
    BOSS_ASSERT(p_sem->wait_count == 0);
    BOSS_ASSERT(p_sem->wait_list == _BOSS_NULL);
    
    p_sem->user_tcb = Boss_self();
    p_sem->busy     = _BOSS_TRUE;
  }
  else
  {
    boss_sigs_t       sigs;
    boss_sem_link_t   sem_link;
    
    sem_link.p_tcb = Boss_self();
    sem_link.prev = _BOSS_NULL;
    sem_link.next = _BOSS_NULL;
    
    if(p_sem->wait_list != _BOSS_NULL)
    {
      sem_link.next = p_sem->wait_list;
      p_sem->wait_list->prev = &sem_link;
    }
    p_sem->wait_list = &sem_link;                     /* 대기 링크에 추가 */
    
    p_sem->wait_count++;                                /* 우선순위 고정 */
    _Boss_priority_rise(Boss_self()->prio, p_sem->user_tcb, 1);
    Boss_sigs_clear( Boss_self(), BOSS_SIG_SEM_OBTAIN );
    _mcu_irq_free(_irq_store_);
    
    sigs = Boss_wait(BOSS_SIG_SEM_OBTAIN, wait_time_ms);  /* 세마포어 대기 */

    _irq_store_ = _mcu_irq_lock();
    sigs = sigs | Boss_sigs_receive();
    if((sigs & BOSS_SIG_SEM_OBTAIN) != BOSS_SIG_SEM_OBTAIN)  /* 세마포어 획득 실패 */
    {
      BOSS_ASSERT((sigs & BOSS_SIG_SLEEP) == BOSS_SIG_SLEEP);

      _Boss_priority_fall_original(p_sem->user_tcb, 1); /* 우선순위 고정 해제 */
      p_sem->wait_count--;

      /* 세마포어 리스트에서 제거 */
      if(p_sem->wait_list == &sem_link)
      {
        p_sem->wait_list = sem_link.next;
        if(sem_link.next != _BOSS_NULL)
        {
          sem_link.next->prev = _BOSS_NULL;
        }
      }
      else
      {
        sem_link.prev->next = sem_link.next;
        sem_link.next->prev = sem_link.prev;
      }
      
      obtain = (boss_reg_t)_BOSS_FAILURE;   /* 세마포어 획득 실패 (타임 아웃) */
    }
  }
  _mcu_irq_free(_irq_store_);
  
  return obtain;
}


/*===========================================================================
    B O S S _ S E M _ R E L E A S E
---------------------------------------------------------------------------*/
void Boss_sem_release(boss_sem_t *p_sem)
{
  BOSS_ASSERT(p_sem->user_tcb == Boss_self());
  BOSS_ASSERT(p_sem->busy == _BOSS_TRUE);

  _Boss_sched_lock();
  BOSS_IRQ_LOCK();                  /* 인터럽트 비활성화 */
  if(p_sem->wait_count == 0)
  {
    BOSS_ASSERT(p_sem->wait_list == _BOSS_NULL);
    
    p_sem->user_tcb = _BOSS_NULL;
    p_sem->busy     = _BOSS_FALSE;
  }
  else
  {
    //boss_byte_t wait_task = 1;    /* 디버깅용 */
    boss_sem_link_t   *p_best;
    boss_sem_link_t   *p_find;
    
    BOSS_ASSERT(p_sem->wait_list != _BOSS_NULL);

    p_best = p_sem->wait_list;
    p_find = p_best->next;
    while(p_find != _BOSS_NULL)   /* 세마포어 대기 리스트에서 우선순위가 높은 테스크 선택 */
    {
      //wait_task++;  /* 디버깅용 */
      if(p_find->p_tcb->prio <= p_best->p_tcb->prio)
      {
        p_best = p_find;
      }
      p_find = p_find->next;
    }
    //BOSS_ASSERT(p_sem->wait_count == wait_task);  /* 디버깅용 */

    if(p_best->prev == _BOSS_NULL)
    {
      BOSS_ASSERT(p_sem->wait_list == p_best);
      
      p_sem->wait_list = p_best->next;
      if(p_best->next != _BOSS_NULL)
      {
        p_best->next->prev = _BOSS_NULL;
      }      
    }
    else
    {
      p_best->prev->next = p_best->next;
      p_best->next->prev = p_best->prev;
    }
    
    BOSS_ASSERT(p_sem->user_tcb->prio_rise >= p_sem->wait_count);
    
                                   /* 세마포어를 사용한 태스크 우선순위 복원 */
    _Boss_priority_fall_original(p_sem->user_tcb, p_sem->wait_count);
    p_sem->wait_count--;
                                  /* 세마포어를 사용할 태스크 우선순위 고정 */
    _Boss_priority_rise(p_best->p_tcb->prio, p_best->p_tcb, p_sem->wait_count);
    
    p_sem->user_tcb = p_best->p_tcb;
    
    Boss_sigs_send(p_best->p_tcb, BOSS_SIG_SEM_OBTAIN);
  }
  BOSS_IRQ_FREE();
  _Boss_sched_free();
}

