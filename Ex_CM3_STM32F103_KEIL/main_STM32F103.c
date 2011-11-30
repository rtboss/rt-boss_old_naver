/*
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*                                                                             *
*                                M  A  I  N                                   *
*                                                                             *
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*/

/*===========================================================================*/
/*                               INCLUDE FILE                                */
/*---------------------------------------------------------------------------*/
#include "Boss.h"
#include "stm32f10x.h"

/*===========================================================================*/
/*                      DEFINITIONS & TYPEDEFS & MACROS                      */
/*---------------------------------------------------------------------------*/


/*===========================================================================*/
/*                             GLOBAL VARIABLES                              */
/*---------------------------------------------------------------------------*/
boss_sem_t      sem_test[1];
boss_mail_q_t   bb_task_q[1];
boss_msg_q_t    aa_msg_q[1];


/*===========================================================================*/
/*                            FUNCTION PROTOTYPES                            */
/*---------------------------------------------------------------------------*/


/*===============================================
    L E D _ C O N F I G
-----------------------------------------------*/
void led_config(void)
{
  RCC->APB2ENR |=  1 <<  3;   /* Enable GPIOB clock          */
  GPIOB->CRH    = 0x33333333; /* Configure the GPIO for LEDs */
}


/*===============================================
    L E D _ O N
-----------------------------------------------*/
void led_on(boss_byte_t led)
{
  GPIOB->BSRR = (uint32_t)(led);
}


/*===============================================
    L E D _ O F F
-----------------------------------------------*/
void led_off(boss_byte_t led)
{
  GPIOB->BRR  = (uint32_t)(led);
}


/*===========================================================================
    [ A A _ T A S K ]
---------------------------------------------------------------------------*/
#define AA_SIG_MSG_Q          SIG_00_BIT

boss_tcb_t  aa_task[1];
boss_stk_t  aa_stk[ 256 / sizeof(boss_stk_t) ];

/*===============================================
    A A _ T A S K _ E X E
-----------------------------------------------*/
void aa_task_exe(void)
{
  Boss_sem_init(sem_test);
  Boss_msg_queue_init(aa_msg_q, AA_SIG_MSG_Q);
  
  for(;;)
  {
    boss_sigs_t sigs = Boss_wait(AA_SIG_MSG_Q, 510);
    
    if(sigs & BOSS_SIG_SLEEP)
    {
      static  boss_uptr_t  result = 0;
      boss_uptr_t         *p_mbox;

      boss_byte_t         temp_led;

      p_mbox  = Boss_mbox_alloc( sizeof(boss_uptr_t) );
      *p_mbox = result;
      result = Boss_mbox_pend(bb_task_q, p_mbox, _BOSS_FALSE);
      result = result % 7;

      temp_led = (boss_byte_t)result;
      temp_led = temp_led << 4;
      
      Boss_sem_obtain(sem_test, _BOSS_FALSE);   /* 세마포어를 잡고 있기 위해 */
      led_off(0x70);
      Boss_sleep(250);
      led_on(temp_led);
      Boss_sleep(250);
      led_off(0x70);
      Boss_sleep(250);
      led_on(temp_led);
      Boss_sleep(250);
      Boss_sem_release(sem_test);
    }
    
    if(sigs & AA_SIG_MSG_Q)
    {
      msg_cmd_t     m_cmd;
      boss_uptr_t   param;

      while(M_CMD_EMPTY != (m_cmd = Boss_msg_receive(aa_msg_q, &param)))
      {
        switch (m_cmd)
        {
          case M_CMD_PARAM_LED_ON :
            led_off(0x0F);
            led_on((boss_byte_t)param);
            break;

          default :
            break;
        }
      }
    }
  }
}



/*===========================================================================
    [ B B _ T A S K ]
---------------------------------------------------------------------------*/
#define BB_SIG_QUEUE            SIG_00_BIT
#define BB_SIG_TIMER            SIG_01_BIT
#define BB_SIG_LED1_ON          SIG_02_BIT
#define BB_SIG_LED1_OFF         SIG_03_BIT

#define BB_WAITING_SIGS    (  BB_SIG_QUEUE | BB_SIG_TIMER           \
                            | BB_SIG_LED1_ON | BB_SIG_LED1_OFF )

boss_tcb_t  bb_task[1];
boss_stk_t  bb_stk[ 256 / sizeof(boss_stk_t) ];

/*===============================================
    B B _ T A S K _ E X E
-----------------------------------------------*/
void bb_task_exe(void)
{
  Boss_tmr_create(Boss_self(), BB_SIG_TIMER, 1500, _BOSS_FALSE);
  Boss_mail_queue_init(bb_task_q, BB_SIG_QUEUE);
  
  for(;;)
  {
    boss_sigs_t sigs = Boss_wait(BB_WAITING_SIGS, _BOSS_FALSE);

    if(sigs & BB_SIG_QUEUE)
    {
      boss_uptr_t *p_mbox;
      while( _BOSS_NULL != (p_mbox = Boss_mbox_receive(bb_task_q)) )
      {
        boss_uptr_t result;
        result = *p_mbox + 1;

        Boss_mbox_done(p_mbox, result);
      }
    }

    if(sigs & BB_SIG_TIMER)
    {
      static boss_byte_t led_val = 0;
      
      Boss_sem_obtain(sem_test, _BOSS_FALSE);
      led_val = led_val + 1;
      led_val = led_val & 0x0F;

      if(led_val != 0)
      {
        Boss_msg_send(aa_msg_q, M_CMD_PARAM_LED_ON, led_val);
      }
      else
      {
        led_off(0x0F);
      }
      Boss_sem_release(sem_test);
      Boss_tmr_create(Boss_self(), BB_SIG_TIMER, 500, _BOSS_FALSE);
    }

    if(sigs & BB_SIG_LED1_ON)
    {
      led_on(0x80);
    }

    if(sigs & BB_SIG_LED1_OFF)
    {
      led_off(0x80);
    }
  }
}



/*===========================================================================
    [ C C _ T A S K ]
---------------------------------------------------------------------------*/
boss_tcb_t  cc_task[1];
boss_stk_t  cc_stk[ 256 / sizeof(boss_stk_t) ];

/*===============================================
    C C _ T A S K _ E X E
-----------------------------------------------*/
void cc_task_exe(void)
{
  for(;;)
  {
    Boss_sleep(500);
    Boss_sigs_send(bb_task, BB_SIG_LED1_ON);
    
    Boss_sleep(500);
    Boss_sigs_send(bb_task, BB_SIG_LED1_OFF);
  }
}



/*===========================================================================
    RT-BOSS Init & Start exetrn
---------------------------------------------------------------------------*/
extern boss_tcb_t idle_task[1];
extern boss_stk_t idle_stack[];

void idle_task_exe(void);
void conf_init(void);


/*===========================================================================
    M A I N
---------------------------------------------------------------------------*/
int main(void)
{
  led_config();
  
  Boss_init(idle_task, idle_task_exe, idle_stack, _IDLE_STACK_BYTES);
  
  Boss_task_create(aa_task, aa_task_exe, PRIO_AA_1, aa_stk, sizeof(aa_stk), _BOSS_NULL);
  Boss_task_create(bb_task, bb_task_exe, PRIO_BB_2, bb_stk, sizeof(bb_stk), _BOSS_NULL);
  Boss_task_create(cc_task, cc_task_exe, PRIO_CC_3, cc_stk, sizeof(cc_stk), _BOSS_NULL);
  
  Boss_start( conf_init );
  BOSS_ASSERT(_BOSS_FALSE);     /* Boss_start() 이후에는 실행되지 않는다. */
  return 0;
}

