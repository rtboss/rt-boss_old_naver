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

/*===========================================================================*/
/*                      DEFINITIONS & TYPEDEFS & MACROS                      */
/*---------------------------------------------------------------------------*/

/*===========================================================================*/
/*                             GLOBAL VARIABLES                              */
/*---------------------------------------------------------------------------*/

/*===========================================================================*/
/*                            FUNCTION PROTOTYPES                            */
/*---------------------------------------------------------------------------*/

/*
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*                                                                             *
*                            RT-BOSS ( SPY  )                                 *
*                                                                             *
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*/
#ifdef _BOSS_SPY_
#define _BOSS_SPY_TCB_MAX       10            /* SPY TCB Table Size   */

boss_u32_t  _spy_elapse_us  = 0;              /* 경과 시간 (us)       */
boss_tcb_t  *_spy_tcb_tbl[_BOSS_SPY_TCB_MAX]; /* SPY TCB list         */


struct {                /* [ ARM Cortex-Mx MSP (Main Stack Pointer) ] */
  boss_stk_t    *sp_base;
  boss_stk_t    *sp_peak;
  boss_stk_t    *sp_limit;
} _spy_msp;


/*===========================================================================
    _   B O S S _ S P Y _ E L A P S E _ U S
---------------------------------------------------------------------------*/
boss_u32_t _Boss_spy_elapse_us(void)
{
  boss_u32_t us;
  boss_u32_t reload = SysTick->LOAD;
  boss_u32_t value  = reload - SysTick->VAL;   /* count-down value */
  
                                /* SysTick->VAL => micro second */
  us = (value * ((boss_u32_t)_BOSS_TICK_MS_ * (boss_u32_t)1000)) / (reload + 1);
  us = _spy_elapse_us + us;
  
  return us;
}


/*===========================================================================
    _   B O S S _ S P Y _ T I C K
---------------------------------------------------------------------------*/
void _Boss_spy_tick(void)
{
  _spy_elapse_us += (boss_u32_t)_BOSS_TICK_MS_ * (boss_u32_t)1000;  /* tick_ms -> us */
  
  if(_spy_elapse_us > 4200000000u)  /* 70분 = 4,200,000,000 us */
  {
    Boss_spy_restart();             /* 70분 경과시 */
  }
}


/*===========================================================================
    _   B O S S _ S P Y _ C O N T E X T
---------------------------------------------------------------------------*/
void _Boss_spy_context(boss_tcb_t *curr_tcb, boss_tcb_t *best_tcb)
{
  { /* [ Stack ] */
    BOSS_ASSERT(curr_tcb->sp_base[0] == (boss_stk_t)0xEEEEEEEE);  // Stack invasion
    while( (curr_tcb->sp_peak[-1] != (boss_stk_t)0xEEEEEEEE) 
        || (curr_tcb->sp_peak[-2] != (boss_stk_t)0xEEEEEEEE)
        || (curr_tcb->sp_peak[-3] != (boss_stk_t)0xEEEEEEEE)
        || (curr_tcb->sp_peak[-4] != (boss_stk_t)0xEEEEEEEE) )
    {
      curr_tcb->sp_peak--;
      BOSS_ASSERT(curr_tcb->sp_peak > curr_tcb->sp_base);   // Stack overflow
    }
  }

  { /* [ C P U ] */
    boss_u32_t now_us = _Boss_spy_elapse_us();

    if( now_us < curr_tcb->cpu_ent_us ) {       /* Tick Timer Pend */
      now_us = now_us + ((boss_u32_t)_BOSS_TICK_MS_ * (boss_u32_t)1000);
      BOSS_ASSERT(now_us >= curr_tcb->cpu_ent_us);
    }
    
    curr_tcb->cpu_sum_us += now_us - curr_tcb->cpu_ent_us;
    best_tcb->cpu_ent_us = now_us;
  }
  
  /* [ Context Switch Number ] */
  best_tcb->context++;
  
  
  /* [ ARM Cortex-Mx MSP (Main Stack Pointer) ] */
  BOSS_ASSERT(_spy_msp.sp_base[0] == (boss_stk_t)0xEEEEEEEE);   // Stack invasion
  while( (_spy_msp.sp_peak[-1] != (boss_stk_t)0xEEEEEEEE) 
      || (_spy_msp.sp_peak[-2] != (boss_stk_t)0xEEEEEEEE)
      || (_spy_msp.sp_peak[-3] != (boss_stk_t)0xEEEEEEEE)
      || (_spy_msp.sp_peak[-4] != (boss_stk_t)0xEEEEEEEE) )
  {
    _spy_msp.sp_peak--;
    BOSS_ASSERT(_spy_msp.sp_peak > _spy_msp.sp_base);  // MSP Stack overflow
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

  { /* [ Stack ] */
    boss_uptr_t size  = bytes / sizeof(boss_stk_t);
    boss_uptr_t i;
    
    for(i = 0; i < size; i++) {
      sp_base[i] = (boss_stk_t)0xEEEEEEEE;  // 스택 [E] empty
    }
    
    p_tcb->sp_base  = &sp_base[0];
    p_tcb->sp_peak  = &sp_base[size-1];
    p_tcb->sp_limit = &sp_base[size];
  }

  /* [ C P U ] */
  p_tcb->cpu_ent_us = 0;
  p_tcb->cpu_sum_us = 0;

  /* [ Context Switch Number ] */
  p_tcb->context = 0;
}


/*===========================================================================
    B O S S _ S P Y _ R E S T A R T
---------------------------------------------------------------------------*/
void Boss_spy_restart(void)
{
  boss_reg_t idx;
  
  BOSS_IRQ_DISABLE();
  _spy_elapse_us  = 0;
  
  for(idx = 0; idx < _BOSS_SPY_TCB_MAX; idx++)
  {
    if(_spy_tcb_tbl[idx] != _BOSS_NULL)
    {
      _spy_tcb_tbl[idx]->cpu_ent_us = 0;
      _spy_tcb_tbl[idx]->cpu_sum_us = 0;
      _spy_tcb_tbl[idx]->context    = 0;
    }
  }
  BOSS_IRQ_RESTORE();
}


/*===========================================================================
    B O S S _ S P Y _ R E P O R T
---------------------------------------------------------------------------*/
void Boss_spy_report(void)
{
  boss_tcb_t *curr_tcb;
  boss_u32_t total_us;
  boss_reg_t idx;
  
  boss_u32_t cpu_pct_sum = 0;
  boss_u32_t context_sum = 0;
  
  _Boss_sched_lock();
  PRINTF("\n[TASK]\t  STACK %%(u/t)\t  C P U    Context\n");
  PRINTF("------------------------------------------\n");
  
  BOSS_IRQ_DISABLE();
  total_us = _Boss_spy_elapse_us();
  _spy_elapse_us = 0;
  
  curr_tcb = Boss_self();
  curr_tcb->cpu_sum_us += total_us - curr_tcb->cpu_ent_us;
  curr_tcb->cpu_ent_us = 0;
  BOSS_IRQ_RESTORE();
  
  for(idx = 0; idx < _BOSS_SPY_TCB_MAX; idx++)
  {
    if(_spy_tcb_tbl[idx] != _BOSS_NULL)
    {
      boss_tcb_t *p_tcb = _spy_tcb_tbl[idx];

      PRINTF(" %s", p_tcb->name);

      { /* [ Stack ] */
        boss_uptr_t stk_total;
        boss_uptr_t stk_used;
        boss_reg_t  stk_pct;
        
        stk_total = (boss_uptr_t)p_tcb->sp_limit - (boss_uptr_t)p_tcb->sp_base;
        stk_used  = (boss_uptr_t)p_tcb->sp_limit - (boss_uptr_t)p_tcb->sp_peak;
        stk_pct = (boss_reg_t)(((boss_u32_t)stk_used * 100) / (boss_u32_t)stk_total);
        
        PRINTF("\t  %2d%%(%3d/%3d)", stk_pct, stk_used, stk_total);
      }

      { /* [ C P U ] */
        boss_u32_t cpu_pct = 0;     /* percent XX.xxx % */
        
        if(p_tcb->cpu_sum_us != 0)
        {
          cpu_pct = (boss_u32_t)(((boss_u64_t)(p_tcb->cpu_sum_us) * (boss_u64_t)100000)
                                                      / (boss_u64_t)total_us);
          p_tcb->cpu_sum_us = 0;
        }
        
        PRINTF("\t %2d.%03d%%", (int)(cpu_pct/1000), (int)(cpu_pct%1000));
        cpu_pct_sum = cpu_pct_sum  + cpu_pct;
      }
      
      PRINTF("   %7d\n", p_tcb->context);
      context_sum = context_sum + p_tcb->context;
      p_tcb->context = 0;
    }
  }

  PRINTF("[TOTAL] :\t\t %2d.%03d%%   %7d\n\n",
          (int)(cpu_pct_sum/1000), (int)(cpu_pct_sum%1000), context_sum);
  _Boss_sched_free();
  
  { /* [ ARM Cortex-Mx MSP (Main Stack Pointer) ] */
    boss_uptr_t msp_total;
    boss_uptr_t msp_used;
    boss_reg_t  msp_pct;
    
    msp_total = (boss_uptr_t)_spy_msp.sp_limit - (boss_uptr_t)_spy_msp.sp_base;
    msp_used  = (boss_uptr_t)_spy_msp.sp_limit - (boss_uptr_t)_spy_msp.sp_peak; 
    msp_pct = (boss_reg_t)(((boss_u32_t)msp_used * 100) / (boss_u32_t)msp_total);
    
    PRINTF("[ M S P ] %%(u/t) :  %2d%% (%3d/%3d)\n", msp_pct, msp_used, msp_total);
  }
}


/*===========================================================================
    _   B O S S _ S P Y _ M S P _ S E T U P
---------------------------------------------------------------------------*/
void _Boss_spy_msp_setup(void)
{
  extern const boss_u32_t CSTACK$$Base;
  extern const boss_u32_t CSTACK$$Limit;
  
  boss_stk_t *msp = (boss_stk_t *)__get_MSP(); /* Get Current Main Stack Pointer (MSP) */
  
  _spy_msp.sp_base  = (boss_stk_t *)&CSTACK$$Base;
  _spy_msp.sp_limit = (boss_stk_t *)&CSTACK$$Limit;
  _spy_msp.sp_peak  = (boss_stk_t *)msp;
  
  msp = msp - 1;    /* FD(Full Descending) Stack */
  
  for(; (boss_stk_t *)&CSTACK$$Base <= msp; --msp)
  {
    *msp = (boss_stk_t)0xEEEEEEEE;  // 스택 [E] empty
  }
}
#endif /* _BOSS_SPY_ */


#ifdef _BOSS_MEM_INFO_
/*===========================================================================
    B O S S _ M E M _ I N F O _ R E P O R T
---------------------------------------------------------------------------*/
void Boss_mem_info_report(void)
{
  boss_uptr_t total;
  boss_uptr_t used;
  boss_uptr_t peak;

  _Boss_sched_lock();
  PRINTF("\n[Mmory]  Peak byte  Used byte  Total  Block  first\n");

  total = _Boss_mem_info_total();
  used  = _Boss_mem_info_used();
  peak  = _Boss_mem_info_peak();
  
  PRINTF("[Info]  %4d (%2d%%) %4d (%2d%%)  %4d    %2d    %4d\n\n", 
                      peak, (boss_uptr_t)((peak * 100) / total),
                      used, (boss_uptr_t)((used * 100) / total),
                      total,_Boss_mem_info_block(),
                      _Boss_mem_info_first_free() );
  _Boss_sched_free();
}
#endif


/*===========================================================================
    _ A S S E R T
---------------------------------------------------------------------------*/
void _assert(const char *file, unsigned int line)
{
  __disable_irq();
  PRINTF("\n ASSERT : %s %d", file, line);
  for(;;)
  {
  }
}


/*
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*                                                                             *
*                      [ STM32L152VB USART3 (printf) ]                        *
*                                                                             *
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*/

/*===========================================================================
    U S A R T 3 _ I N I T
---------------------------------------------------------------------------*/
void usart3_init(void)
{
  GPIO_InitTypeDef  GPIO_InitStructure;
  USART_InitTypeDef USART_InitStruct;

  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOD, ENABLE);     /* Enable GPIO clock */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);  /* Enable UART clock */

  GPIO_PinAFConfig(GPIOD, GPIO_PinSource8, GPIO_AF_USART3); /*!< PD.8 to TX(USART3) */
  GPIO_PinAFConfig(GPIOD, GPIO_PinSource9, GPIO_AF_USART3); /*!< PD.9 to RX(USART3) */

  /* Configure USART Tx/RX as alternate function push-pull */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_40MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(GPIOD, &GPIO_InitStructure);

  /* USART configuration */
  USART_InitStruct.USART_BaudRate = 112500;
  USART_InitStruct.USART_WordLength = USART_WordLength_8b;
  USART_InitStruct.USART_StopBits = USART_StopBits_1;
  USART_InitStruct.USART_Parity = USART_Parity_No;
  USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStruct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  USART_Init(USART3, &USART_InitStruct);
  
  USART_Cmd(USART3, ENABLE);      /* Enable USART */
}


/*===========================================================================
    F P U T C
---------------------------------------------------------------------------*/
int fputc(int ch, FILE *f)
{
  static boss_reg_t fputc_init = _BOSS_FALSE;

  if(fputc_init == _BOSS_FALSE)
  {
    fputc_init = _BOSS_TRUE;
    usart3_init();
  }

  if(ch == '\n') {
    fputc('\r', f);
  }
  
  /* Loop until transmit data register is empty */
  while((USART3->SR & USART_FLAG_TXE) == (uint16_t)RESET)
  {}
  
  /* Place your implementation of fputc here */
  /* e.g. write a character to the USART */
  USART3->DR = (uint16_t)ch;

  return ch;
}


/*===========================================================================
    B O S S _ D E V I C E _ I N I T
---------------------------------------------------------------------------*/
void Boss_device_init(void)
{
  #ifdef _BOSS_SPY_
  _Boss_spy_msp_setup();
  #endif

  if (SysTick_Config(SystemCoreClock / 1000)) { /* Setup SysTick Timer for 1 msec interrupts  */
    while (1);                                  /* Capture error */
  }
  
  NVIC_SetPriority(PendSV_IRQn, (1<<__NVIC_PRIO_BITS) - 1);  /* PendSV IRQ 우선순위 */
}



/*
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*                                                                             *
*                       [ RT-BOSS Tick Timer (ISR) ]                          *
*                                                                             *
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*/
/*===========================================================================
    S Y S   T I C K _   H A N D L E R                 [Cortex-M3 SysTick ISR]
---------------------------------------------------------------------------*/
void SysTick_Handler(void)    /* Boss Tick Timer */
{
  _BOSS_ISR_BEGIN();
  {
    _Boss_timer_tick(_BOSS_TICK_MS_);
    
    #ifdef _BOSS_SPY_
    _Boss_spy_tick();
    #endif
  }
  _BOSS_ISR_FINIS();
}

