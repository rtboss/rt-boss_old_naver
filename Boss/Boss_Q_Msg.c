/*
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*                                                                             *
*                          RT-BOSS (Message Queue)                            *
*                                                                             *
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*/

/*===========================================================================*/
/*                               INCLUDE FILE                                */
/*---------------------------------------------------------------------------*/
#include "Boss_Kernel.h"
#include "Boss_Q_Msg.h"

/*===========================================================================*/
/*                      DEFINITIONS & TYPEDEFS & MACROS                      */
/*---------------------------------------------------------------------------*/


/*===========================================================================*/
/*                             GLOBAL VARIABLES                              */
/*---------------------------------------------------------------------------*/

/*===========================================================================*/
/*                            FUNCTION PROTOTYPES                            */
/*---------------------------------------------------------------------------*/


/*===========================================================================
    B O S S _ M S G _ Q U E U E _ I N I T
---------------------------------------------------------------------------*/
void Boss_msg_queue_init(boss_msg_q_t *msg_q, boss_sigs_t msg_q_sig)
{
  BOSS_ASSERT(msg_q->msg_q_tcb == _BOSS_NULL);
  BOSS_ASSERT(msg_q->msg_fifo_list == _BOSS_NULL);
  BOSS_ASSERT(msg_q_sig != BOSS_SIGS_NULL);

  msg_q->msg_q_tcb  = Boss_self();
  msg_q->msg_q_sig  = msg_q_sig;
  msg_q->msg_fifo_list = _BOSS_NULL;
}


/*===========================================================================
    B O S S _ M S G _ S E N D
---------------------------------------------------------------------------*/
void Boss_msg_send(boss_msg_q_t *msg_q, msg_cmd_t m_cmd, boss_uptr_t param)
{
  _msg_fifo_t *msg_fifo;

  BOSS_ASSERT( (M_CMD_EMPTY < m_cmd) && (m_cmd < M_CMD_MAX) );
    
  msg_fifo = _BOSS_MSG_FIFO_ALLOC(sizeof(_msg_fifo_t));
  
  msg_fifo->m_cmd = m_cmd;
  msg_fifo->param = param;
  msg_fifo->next  = _BOSS_NULL;

  BOSS_IRQ_LOCK();
  if(msg_q->msg_fifo_list == _BOSS_NULL)
  {
    msg_q->msg_fifo_list = msg_fifo;
  }
  else
  {
    _msg_fifo_t *p_find = msg_q->msg_fifo_list;

    while(p_find->next != _BOSS_NULL)
    {
      p_find = p_find->next;
    }
    p_find->next = msg_fifo;        /* 리스트의 마지막에 추가 한다. */
  }
  BOSS_IRQ_FREE();
  
  Boss_sigs_send(msg_q->msg_q_tcb, msg_q->msg_q_sig);
}


/*===========================================================================
    B O S S _ M S G _ R E C E I V E
---------------------------------------------------------------------------*/
msg_cmd_t Boss_msg_receive(boss_msg_q_t *msg_q, boss_uptr_t *p_param)
{
  msg_cmd_t m_cmd = M_CMD_EMPTY;

  BOSS_ASSERT(msg_q->msg_q_tcb == Boss_self());

  if(msg_q->msg_fifo_list != _BOSS_NULL)
  {
    _msg_fifo_t *msg_fifo;
    
    BOSS_IRQ_LOCK();
    msg_fifo = msg_q->msg_fifo_list;
    msg_q->msg_fifo_list = msg_fifo->next;
    BOSS_IRQ_FREE();

    m_cmd = msg_fifo->m_cmd;
    *p_param = msg_fifo->param;
    
    _BOSS_MSG_FIFO_FREE(msg_fifo);
  }

  return m_cmd;
}

