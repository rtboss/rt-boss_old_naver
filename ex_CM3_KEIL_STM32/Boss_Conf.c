/*
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*                                                                             *
*                 RT-BOSS (Config File)         [ Cortex-M3 ]                 *
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

/*===========================================================================*/
/*                            FUNCTION PROTOTYPES                            */
/*---------------------------------------------------------------------------*/


/*===========================================================================
    S Y S   T I C K _   H A N D L E R (Cortex-M3 SysTick ISR)
---------------------------------------------------------------------------*/
void SysTick_Handler(void)    /* Boss Tick Timer */
{
  _BOSS_ISR_BEGIN();
  {
    _Boss_timer_tick(_BOSS_TICK_MS_);
  }
  _BOSS_ISR_FINIS();
}



/*===========================================================================
    D E V I C E _ I N I T
---------------------------------------------------------------------------*/
void device_init(void)
{
  if (SysTick_Config(SystemCoreClock / 1000)) { /* Setup SysTick Timer for 1 msec interrupts  */
    while (1);                                  /* Capture error */
  }
  
  NVIC_SetPriority(PendSV_IRQn, (1<<__NVIC_PRIO_BITS) - 1);  /* PendSV IRQ 우선순위 */
}




/*
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*                                                                             *
*                   [ Cortex-M3 Debug (printf) Viewer ]                       *
*                                                                             *
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*/

#define ITM_Port8(n)    (*((volatile unsigned char *)(0xE0000000+4*n)))
#define ITM_Port16(n)   (*((volatile unsigned short*)(0xE0000000+4*n)))
#define ITM_Port32(n)   (*((volatile unsigned long *)(0xE0000000+4*n)))

#define DEMCR           (*((volatile unsigned long *)(0xE000EDFC)))
#define TRCENA          0x01000000


struct __FILE { int handle; /* Add whatever needed */ };
FILE __stdout;
FILE __stdin;


/*===========================================================================
    F P U T C
---------------------------------------------------------------------------*/
int fputc(int ch, FILE *f) {
  if (DEMCR & TRCENA) {
    while (ITM_Port32(0) == 0);
    ITM_Port8(0) = ch;
  }
  return(ch);
}


/*===========================================================================
    _ A S S E R T
---------------------------------------------------------------------------*/
void _assert(const unsigned char *file, unsigned int line)
{
  __disable_irq();
  PRINTF("\r\n ASSERT : %s %d", file, line);
  for(;;)
  {
  }
}



/*
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*                                                                             *
*                            RT-BOSS ( SPY  )                                 *
*                                                                             *
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*/
#ifdef _BOSS_SPY_
/*===========================================================================
    _   B O S S _ S P Y _ C O N T E X T
---------------------------------------------------------------------------*/
void _Boss_spy_context(boss_tcb_t *curr_tcb, boss_tcb_t *best_tcb)
{
  BOSS_ASSERT(curr_tcb->sp_begin[-1] == (boss_stk_t)0xBBBBBBBB);  // Stack invasion (Begin)
  BOSS_ASSERT(curr_tcb->sp_finis[0] == (boss_stk_t)0xFFFFFFFF);   // Stack invasion (finis)


  if( (curr_tcb->sp_peak[0] != (boss_stk_t)0xEEEEEEEE) 
      || (curr_tcb->sp_peak[-1] != (boss_stk_t)0xEEEEEEEE)
      || (curr_tcb->sp_peak[-2] != (boss_stk_t)0xEEEEEEEE)
      || (curr_tcb->sp_peak[-3] != (boss_stk_t)0xEEEEEEEE) )
  {
    if( (curr_tcb->sp_peak - 4) > curr_tcb->sp_finis)
    {
      curr_tcb->sp_peak -= 4;
    }
    
    while( (curr_tcb->sp_peak > curr_tcb->sp_finis) 
                  && (*curr_tcb->sp_peak != (boss_stk_t)0xEEEEEEEE) )
    {
      curr_tcb->sp_peak--;
    }

    BOSS_ASSERT(curr_tcb->sp_peak > curr_tcb->sp_finis);    // Stack overflow
  }
}


/*===========================================================================
    _   B O S S _ S P Y _ S E T U P
---------------------------------------------------------------------------*/
void _Boss_spy_setup(boss_tcb_t *p_tcb, boss_stk_t *sp_base, boss_uptr_t bytes)
{
  { /* [ Stack ] */
    boss_uptr_t size  = bytes / sizeof(boss_stk_t);
    
    p_tcb->sp_finis = &sp_base[0];
    p_tcb->sp_peak  = &sp_base[size-1];
    p_tcb->sp_begin = &sp_base[size];
  }
}


/*===========================================================================
    B O S S _ S P Y _ S T A C K _ P R O F I L E
---------------------------------------------------------------------------*/
void Boss_spy_stack_profile(boss_tcb_t *p_tcb)
{
  boss_uptr_t total;
  boss_uptr_t usage;
  boss_uptr_t percent;

  total = (boss_uptr_t)p_tcb->sp_begin - (boss_uptr_t)p_tcb->sp_finis;
  usage = (boss_uptr_t)p_tcb->sp_begin - (boss_uptr_t)p_tcb->sp_peak;
  percent = (usage * 100) / total;

  PRINTF("[%s TASK] Usage/Total : %d/%d (%d%%) \r\n",p_tcb->name, usage, total, percent);
}
#endif /* _BOSS_SPY_ */



/*
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*                                                                             *
*                         RT-BOSS ( IDLE TASK )                               *
*                                                                             *
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*/

boss_tcb_t idle_tcb;
boss_stk_t idle_stack[ _IDLE_STACK_BYTES / sizeof(boss_stk_t) ];

/*===========================================================================
    I D L E _ M A I N
---------------------------------------------------------------------------*/
void idle_main(void *p_arg)
{
  for(;;)
  {
  }
}

