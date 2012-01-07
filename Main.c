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
    [ A A _ T A S K ]
---------------------------------------------------------------------------*/
boss_tcb_t  aa_tcb;
boss_stk_t  aa_stk[ 512 / sizeof(boss_stk_t) ];   /* 512 bytes */

/*===============================================
    A A _ M A I N
-----------------------------------------------*/
void aa_main(void *p_arg)
{  
  int aa_count = 0;
  
  PRINTF("[%s TASK] Init \r\n", Boss_self()->name);
  
  for(;;)
  {    
    Boss_sleep(500);  /* 500ms */
    PRINTF(" AA_TASK count = %d \r\n", ++aa_count);
  }
}


/*===========================================================================
    [ B B _ T A S K ]
---------------------------------------------------------------------------*/
boss_tcb_t  bb_tcb;
boss_stk_t  bb_stk[ 512 / sizeof(boss_stk_t) ];   /* 512 bytes */

/*===============================================
    B B _ M A I N
-----------------------------------------------*/
void bb_main(void *p_arg)
{
  int bb_count = 0;
  
  PRINTF("[%s TASK] Init \r\n", Boss_self()->name);
  
  for(;;)
  {
    Boss_sleep(1000); /* 1000ms */
    PRINTF("BB_TASK count = %d \r\n", ++bb_count);
    
    
    #ifdef _BOSS_SPY_
    Boss_spy_stack_profile(&aa_tcb);
    Boss_spy_stack_profile(&bb_tcb);
    #endif
  }
}


/*===========================================================================
    RT-BOSS Init & Start exetrn
---------------------------------------------------------------------------*/
extern boss_tcb_t idle_tcb;
extern boss_stk_t idle_stack[];
void idle_main(void *args);

void device_init(void);

/*===========================================================================
    M A I N
---------------------------------------------------------------------------*/
int main(void)
{
  device_init();
  
  _Boss_mem_pool_init();    /* Memory Pool init */
  Boss_init(idle_main, &idle_tcb, idle_stack, _IDLE_STACK_BYTES);
  
  Boss_task_create( aa_main,              /* Task Entry Point       */
                    _BOSS_NULL,           /* Task Argument          */
                    &aa_tcb,              /* TCB(Task Control Block)*/
                    AA_PRIO_1,            /* Priority               */
                    aa_stk,               /* Stack Point (Base)     */
                    sizeof(aa_stk),       /* Stack Size (Bytes)     */
                    "AA"
                    );
  
  Boss_task_create( bb_main,
                    _BOSS_NULL,
                    &bb_tcb,
                    BB_PRIO_2,
                    bb_stk,
                    sizeof(bb_stk),
                    "BB"
                    );
  
  Boss_start();                /* Boss Scheduling Start */
  
  BOSS_ASSERT(_BOSS_FALSE);   /* Invalid */
  return 0;
}

