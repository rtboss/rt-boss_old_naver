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
void _Boss_spy_tick(boss_tmr_ms_t tick_ms);


/*===========================================================================
    S Y S   T I C K _   H A N D L E R (Cortex-M3 SysTick ISR)
---------------------------------------------------------------------------*/
void SysTick_Handler(void)    /* Boss Tick Timer */
{
  _BOSS_ISR_BEGIN();
  {
    _Boss_timer_tick(_BOSS_TICK_MS_);
    #ifdef _BOSS_SPY_
    _Boss_spy_tick(_BOSS_TICK_MS_);
    #endif
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
boss_u32_t  _spy_enter_us = 0;     /* microsecond */
boss_u32_t  _spy_elapse_us = 0;   /* microsecond */
boss_tcb_t  *_spy_tcb_tbl[_BOSS_SPY_TCB_MAX];


/*===========================================================================
    _   B O S S _ S P Y _ T I C K
---------------------------------------------------------------------------*/
void _Boss_spy_tick(boss_tmr_ms_t tick_ms)
{
  _spy_elapse_us += (boss_u32_t)(tick_ms * 1000);
}


/*===========================================================================
    _   B O S S _ S P Y _ N O W _ T I C K _ U S
---------------------------------------------------------------------------*/
boss_u32_t _Boss_spy_now_tick_us(void)
{
  boss_u32_t now_us;
  boss_u32_t reload = SysTick->LOAD + 1;
  boss_u32_t tmr_cnt = reload - SysTick->VAL;

  now_us = ( (_BOSS_TICK_MS_ * 1000) * tmr_cnt ) / reload;
  now_us = _spy_elapse_us + now_us;
  
  if(SCB->ICSR & SCB_ICSR_PENDSTSET_Msk) {
    now_us = now_us + (_BOSS_TICK_MS_ * 1000);
  }

  return now_us;
}


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
    --curr_tcb->sp_peak;
    
    while( (curr_tcb->sp_peak > curr_tcb->sp_finis) 
                  && (*curr_tcb->sp_peak != (boss_stk_t)0xEEEEEEEE) )
    {
      curr_tcb->sp_peak--;
    }

    BOSS_ASSERT(curr_tcb->sp_peak > curr_tcb->sp_finis);    // Stack overflow
  }

  best_tcb->context_num++;  /* Context Switch */

  { /* CPU */
    boss_u32_t now_us = _Boss_spy_now_tick_us();

    curr_tcb->cpu_total += now_us - curr_tcb->cpu_enter;
    best_tcb->cpu_enter = now_us;
  }
}


/*===========================================================================
    _   B O S S _ S P Y _ S E T U P
---------------------------------------------------------------------------*/
void _Boss_spy_setup(boss_tcb_t *p_tcb, boss_stk_t *sp_base, boss_uptr_t bytes)
{
  { /* SPY TCB 등록 */
    boss_reg_t idx;
  
    for(idx = 0; idx < _BOSS_SPY_TCB_MAX; idx++)
    {
      if(_spy_tcb_tbl[idx] == _BOSS_NULL) {
        _spy_tcb_tbl[idx] = p_tcb;
        break;
      }
    }
  
    BOSS_ASSERT(idx < _BOSS_SPY_TCB_MAX);
  }

  { /* [ Context Switch Number ] */
    p_tcb->context_num = 0;
  }

  { /* [ Stack ] */
    boss_uptr_t size  = bytes / sizeof(boss_stk_t);
    
    p_tcb->sp_finis = &sp_base[0];
    p_tcb->sp_peak  = &sp_base[size-1];
    p_tcb->sp_begin = &sp_base[size];
  }

  { /* CPU */
    p_tcb->cpu_enter = 0;
    p_tcb->cpu_total = 0;
  }
}


/*===========================================================================
    B O S S _ S P Y _ R E P O R T
---------------------------------------------------------------------------*/
void Boss_spy_report(void)
{
  boss_tcb_t *curr_tcb;
  boss_u32_t total_us;
  boss_u32_t now_us;
  
  boss_reg_t idx;

  BOSS_IRQ_DISABLE();
  total_us = _Boss_spy_now_tick_us();
  _spy_elapse_us = 0;
  now_us = _Boss_spy_now_tick_us();
  
  curr_tcb = Boss_self();
  curr_tcb->cpu_total += total_us - curr_tcb->cpu_enter;
  curr_tcb->cpu_enter = now_us;

  total_us -= _spy_enter_us;
  _spy_enter_us = now_us;
  BOSS_IRQ_RESTORE();

  PRINTF("\r\nNAME\t  CPU  \t\t STACK \t\t Context\r\n");
  PRINTF("------------------------------------------------\r\n");
  for(idx = 0; idx < _BOSS_SPY_TCB_MAX; idx++)
  {
    if(_spy_tcb_tbl[idx] != _BOSS_NULL)
    {
      boss_tcb_t *p_tcb = _spy_tcb_tbl[idx];
      boss_u32_t cpu_per;
      boss_u32_t stk_per;
      boss_u32_t stk_total;
      boss_u32_t stk_usage;
      cpu_per = (boss_u32_t)(((boss_u64_t)(p_tcb->cpu_total) * 100 * 1000000)
                                                      / (boss_u64_t)total_us);
      
      stk_total = (boss_u32_t)p_tcb->sp_begin - (boss_u32_t)p_tcb->sp_finis;
      stk_usage = (boss_u32_t)p_tcb->sp_begin - (boss_u32_t)p_tcb->sp_peak;
      stk_per = (stk_usage * 100) / stk_total;
      
      PRINTF("[%s]\t %2d.%06d%% \t %d%% (%d/%d) \t %6d \r\n"
                ,p_tcb->name, cpu_per/1000000, cpu_per%1000000
                ,stk_per, stk_usage, stk_total, p_tcb->context_num);
      
      p_tcb->cpu_total = 0;
    }
  }
  
  PRINTF("\r\n");
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

