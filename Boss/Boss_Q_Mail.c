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
#include "Boss_Q_Mail.h"

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
    B O S S _ M A I L _ Q U E U E _ I N I T
---------------------------------------------------------------------------*/
void Boss_mail_queue_init(boss_mail_q_t *mail_q, boss_sigs_t mail_q_sig)
{
  BOSS_ASSERT(mail_q->mail_q_tcb == _BOSS_NULL);
  BOSS_ASSERT(mail_q->mbox_list == _BOSS_NULL);
  BOSS_ASSERT(mail_q_sig != BOSS_SIGS_NULL);

  mail_q->mail_q_tcb = Boss_self();
  mail_q->mail_q_sig = mail_q_sig;
  mail_q->mbox_list  = _BOSS_NULL;
}


/*===========================================================================
    B O S S _ M B O X _ A L L O C
---------------------------------------------------------------------------*/
void *Boss_mbox_alloc(boss_uptr_t size)
{
  _mbox_head_t  *h_mbox;
  
  h_mbox = _BOSS_MAIL_Q_BOX_ALLOC(sizeof(_mbox_head_t) + size);

  h_mbox->mail_cond = _MAIL_NONE;
  h_mbox->prio      = PRIO_BOSS_KERNEL;
  h_mbox->sender    = _BOSS_NULL;
  h_mbox->p_result  = _BOSS_NULL;
  h_mbox->next      = _BOSS_NULL;

  return  (h_mbox + 1);  // mbox : ( (void *)h_mbox + sizeof(_mbox_head_t) )
}


/*===========================================================================
    B O S S _ M B O X _ S E N D
---------------------------------------------------------------------------*/
void Boss_mbox_send(boss_mail_q_t *mail_q, void *mbox)
{
  _mbox_head_t  *h_mbox;

  _mbox_head_t  *p_prev;
  _mbox_head_t  *p_next;

  h_mbox = ((_mbox_head_t *)mbox) - 1; // h_mbox : ( (void *)mbox - sizeof(_mbox_head_t) )

  BOSS_ASSERT(h_mbox->mail_cond  == _MAIL_NONE);
  BOSS_ASSERT(h_mbox->prio    == PRIO_BOSS_KERNEL);
  BOSS_ASSERT(h_mbox->sender  == _BOSS_NULL);
  BOSS_ASSERT(h_mbox->next    == _BOSS_NULL);
  BOSS_ASSERT((h_mbox->p_result == _BOSS_NULL) || (mail_q->mail_q_tcb != Boss_self()) );

  h_mbox->mail_cond = _MAIL_EXE_READY;
  
  if(_mcu_isr_running() == _BOSS_FALSE)
  {
    boss_tcb_t  *cur_tcb = Boss_self();
    
    h_mbox->prio    = cur_tcb->prio;
    h_mbox->sender  = cur_tcb;
  }
  
  BOSS_IRQ_LOCK();
  p_prev = _BOSS_NULL;
  p_next = mail_q->mbox_list;

  while( (p_next != _BOSS_NULL) && (p_next->prio <= h_mbox->prio) ) /* 우선순위로 정렬 */
  {
    p_prev = p_next;
    p_next = p_next->next;
  }
  
  /* 큐 링크 연결 */
  if(p_prev == _BOSS_NULL)
  {
    h_mbox->next = mail_q->mbox_list;
    mail_q->mbox_list = h_mbox;
  }
  else
  {
    p_prev->next = h_mbox;
    h_mbox->next = p_next;
  }
  BOSS_IRQ_FREE();

  Boss_sigs_send(mail_q->mail_q_tcb, mail_q->mail_q_sig);
}


/*===========================================================================
    B O S S _ M B O X _ P E N D
---------------------------------------------------------------------------*/
boss_uptr_t Boss_mbox_pend(boss_mail_q_t *mail_q, void *mbox,
                                                    boss_tmr_t wait_time_ms)
{
  _mbox_head_t    *h_mbox;
  boss_sigs_t     sigs;  
  boss_uptr_t     result = _BOSS_SUCCESS;
  
  BOSS_ASSERT(_mcu_irq_hold() == _BOSS_FALSE);
  BOSS_ASSERT(_mcu_isr_running() == _BOSS_FALSE);
  BOSS_ASSERT(Boss_sched_locking() == _BOSS_FALSE);
  BOSS_ASSERT((Boss_self()->sigs & BOSS_SIG_MBOX_PEND_DONE) != BOSS_SIG_MBOX_PEND_DONE);

  h_mbox = ((_mbox_head_t *)mbox) - 1;
  h_mbox->p_result = &result;

  _Boss_priority_rise(Boss_self()->prio, mail_q->mail_q_tcb, 1);  /* 우선순위 고정 및 상승 */
  
  Boss_sigs_clear( Boss_self(), BOSS_SIG_MBOX_PEND_DONE );
  Boss_mbox_send( mail_q, mbox );
  
  sigs = Boss_wait(BOSS_SIG_MBOX_PEND_DONE, wait_time_ms);  /* 큐 실행 대기 */

  BOSS_IRQ_LOCK();
  sigs = sigs | Boss_sigs_receive();
  if((sigs & BOSS_SIG_MBOX_PEND_DONE) != BOSS_SIG_MBOX_PEND_DONE) /* 타임 아웃, 실행 실패 */
  {
    _Boss_priority_fall_original(mail_q->mail_q_tcb, 1);      /* 우선순위 고정 해제 */
    
    BOSS_ASSERT((sigs & BOSS_SIG_SLEEP) == BOSS_SIG_SLEEP);
    if(h_mbox->mail_cond == _MAIL_EXE_READY)
    {
      result = _MBOX_FAIL_TIMEOUT;      /* 실행 되지 않았을때 */
    }
    else
    {
      BOSS_ASSERT(h_mbox->mail_cond == _MAIL_EXECUTING);
      result = _MBOX_FAIL_EXECUTE;      /* 실행 중 타임 아웃 일때 */
    }
    
    h_mbox->mail_cond = _MAIL_EXE_STOP;
    h_mbox->p_result  = _BOSS_NULL;
  }
  BOSS_IRQ_FREE();

  return result;
}


/*===========================================================================
    B O S S _ M B O X _ R E C E I V E
---------------------------------------------------------------------------*/
void *Boss_mbox_receive(boss_mail_q_t *mail_q)
{
  void *mbox = _BOSS_NULL;
  
  BOSS_ASSERT(mail_q->mail_q_tcb == Boss_self());

  while( (mail_q->mbox_list != _BOSS_NULL) && (mbox == _BOSS_NULL) )
  {
    _mbox_head_t   *h_mbox;
    
    BOSS_IRQ_LOCK();
    h_mbox = mail_q->mbox_list;
    mail_q->mbox_list = h_mbox->next;

    if(h_mbox->mail_cond == _MAIL_EXE_READY)
    {
      h_mbox->mail_cond = _MAIL_EXECUTING;
      mbox = (h_mbox + 1);
    }
    BOSS_IRQ_FREE();

    if(mbox == _BOSS_NULL)
    {
      BOSS_ASSERT(h_mbox->mail_cond == _MAIL_EXE_STOP);
      _BOSS_MAIL_Q_BOX_FREE(h_mbox);
    }
  }

  return mbox;
}


/*===========================================================================
    B O S S _ M B O X _ D O N E
---------------------------------------------------------------------------*/
void Boss_mbox_done(void *mbox, boss_uptr_t result)
{
  _mbox_head_t *h_mbox;

  BOSS_ASSERT((_mcu_irq_hold() ==_BOSS_FALSE) || (Boss_sched_locking() != _BOSS_FALSE));
  BOSS_ASSERT(_mcu_isr_running() == _BOSS_FALSE);
  
  h_mbox = ((_mbox_head_t *)mbox) - 1;

  BOSS_ASSERT( (h_mbox->mail_cond == _MAIL_EXECUTING) 
              || (h_mbox->mail_cond == _MAIL_EXE_STOP));

  _Boss_sched_lock();
  BOSS_IRQ_LOCK();
  if(h_mbox->p_result != _BOSS_NULL)  /* mbox pend wait */
  {
    BOSS_ASSERT( h_mbox->mail_cond == _MAIL_EXECUTING );
    BOSS_ASSERT( *(h_mbox->p_result) == _BOSS_SUCCESS );
    
    *(h_mbox->p_result) = result;
    _Boss_priority_fall_original( Boss_self(), 1);           /* 우선순위 고정 해제 */
    
    Boss_sigs_send(h_mbox->sender, BOSS_SIG_MBOX_PEND_DONE);
  }
  BOSS_IRQ_FREE();
  _Boss_sched_free();
  
  _BOSS_MAIL_Q_BOX_FREE(h_mbox);
}



/*===========================================================================
    B O S S _ M B O X _ S E N D E R _ T C B
---------------------------------------------------------------------------*/
boss_tcb_t *Boss_mbox_sender_tcb(void *mbox)
{
  _mbox_head_t *h_mbox = ((_mbox_head_t *)mbox) - 1;

  BOSS_ASSERT((h_mbox->mail_cond == _MAIL_EXECUTING) 
              || (h_mbox->mail_cond == _MAIL_EXE_STOP));

  return (h_mbox->sender);
}

