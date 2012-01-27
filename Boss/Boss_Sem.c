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
#include "Boss.h"
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
static void _Boss_sem_list_remove(boss_sem_t *p_sem, _sem_link_t *p_link);

/*===========================================================================
    B O S S _ S E M _ I N I T
---------------------------------------------------------------------------*/
void Boss_sem_init(boss_sem_t *p_sem)
{
  p_sem->busy       = 0;
  p_sem->wait_list  = _BOSS_NULL;
  p_sem->owner_tcb  = _BOSS_NULL;
}


/*===========================================================================
    B O S S _ S E M _ O B T A I N
---------------------------------------------------------------------------*/
boss_reg_t Boss_sem_obtain(boss_sem_t *p_sem, boss_tmr_ms_t timeout)
{
  boss_reg_t  irq_storage;
  boss_sigs_t sigs;
  _sem_link_t sem_link;
  boss_tcb_t  *cur_tcb = Boss_self();
  
  BOSS_ASSERT(_BOSS_IRQ_() == 0);         /* IRQ Enable */
  BOSS_ASSERT(_BOSS_ISR_() == 0);
  BOSS_ASSERT(Boss_sched_locking() == 0);
  
  BOSS_ASSERT(p_sem->owner_tcb != cur_tcb);     /* ���� ȹ�� ���� */
  BOSS_ASSERT((cur_tcb->sigs & BOSS_SIG_SEM_OBTAIN) != BOSS_SIG_SEM_OBTAIN);

  BOSS_IRQ_DISABLE_SR(irq_storage);
  if(p_sem->busy == 0)                                /* �������� �̻�� */
  {
    BOSS_ASSERT(p_sem->owner_tcb == _BOSS_NULL);
    BOSS_ASSERT(p_sem->wait_list == _BOSS_NULL);
    
    p_sem->busy++;
    p_sem->owner_tcb  = cur_tcb;
    BOSS_IRQ_RESTORE_SR(irq_storage);
    
    return _BOSS_SUCCESS;
  }
                                                    /* �������� ����� */
  sem_link.p_tcb = cur_tcb;
  sem_link.next = _BOSS_NULL;

  sem_link.next = p_sem->wait_list;                         /* ����Ʈ �߰�  */
  p_sem->wait_list = p_sem->wait_list = &sem_link;
  p_sem->busy++;
  
  Boss_sigs_clear(cur_tcb, BOSS_SIG_SEM_OBTAIN);
  BOSS_IRQ_RESTORE_SR(irq_storage);

  sigs = Boss_wait_sleep(BOSS_SIG_SEM_OBTAIN, timeout);  /* �������� ���  */

  BOSS_IRQ_DISABLE_SR(irq_storage);
  sigs = sigs | Boss_sigs_receive();
  if( sigs & BOSS_SIG_SEM_OBTAIN )                        /* �������� ȹ��  */
  {
    BOSS_ASSERT(p_sem->busy != _BOSS_FALSE);
    BOSS_ASSERT(p_sem->owner_tcb == cur_tcb);      
    BOSS_IRQ_RESTORE_SR(irq_storage);
    
    return _BOSS_SUCCESS;
  }
  
  BOSS_ASSERT((sigs & BOSS_SIG_SLEEP) == BOSS_SIG_SLEEP);    /* Ÿ�Ӿƿ� */
  _Boss_sem_list_remove(p_sem, &sem_link);                /* ����Ʈ ����  */
  p_sem->busy--;
  BOSS_IRQ_RESTORE_SR(irq_storage);

  return (boss_reg_t)_BOSS_FAILURE;               /* �������� ȹ�� ���� */
}


/*===========================================================================
    B O S S _ S E M _ R E L E A S E
---------------------------------------------------------------------------*/
void Boss_sem_release(boss_sem_t *p_sem)
{
  BOSS_ASSERT(_BOSS_ISR_() == 0);
  
  BOSS_ASSERT(p_sem->owner_tcb == Boss_self());
  BOSS_ASSERT(p_sem->busy != 0);

  _Boss_sched_lock();
  BOSS_IRQ_DISABLE();
  if(p_sem->wait_list == _BOSS_NULL)  
  {
    p_sem->busy--;
    p_sem->owner_tcb  = _BOSS_NULL;
    BOSS_ASSERT(p_sem->busy == 0);
  }
  else
  {
    _sem_link_t   *p_best = p_sem->wait_list;
    _sem_link_t   *p_find = p_best->next;

    while(p_find != _BOSS_NULL)                 /* Baest tcb find */
    {
      if(p_find->p_tcb->prio <= p_best->p_tcb->prio)
      {
        p_best = p_find;
      }
      p_find = p_find->next;
    }
    _Boss_sem_list_remove(p_sem, p_best);
    p_sem->busy--;

    p_sem->owner_tcb = p_best->p_tcb;
    Boss_sigs_send(p_best->p_tcb, BOSS_SIG_SEM_OBTAIN);
  }
  BOSS_IRQ_RESTORE();
  _Boss_sched_free();
}


/*===========================================================================
    _   B O S S _ S E M _ L I S T _ R E M O V E
---------------------------------------------------------------------------*/
static void _Boss_sem_list_remove(boss_sem_t *p_sem, _sem_link_t *p_link)
{
  BOSS_ASSERT(_BOSS_IRQ_() != 0);      /* IRQ Disable */
  
  if(p_sem->wait_list == p_link)
  {
    p_sem->wait_list = p_link->next;
  }
  else
  {
    _sem_link_t *p_find = p_sem->wait_list;
    
    while(p_find->next != p_link)
    {
      p_find = p_find->next;
      BOSS_ASSERT(p_find != _BOSS_NULL);
    }
    p_find->next = p_link->next;
  }
}

