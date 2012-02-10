/*
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*                                                                             *
*                 RT-BOSS (Config File)         [ ATmega128 ]                 *
*                                                                             *
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*/

/*===========================================================================*/
/*                               INCLUDE FILE                                */
/*---------------------------------------------------------------------------*/
#include <avr/io.h>
#include <avr/interrupt.h>
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

boss_u32_t  _spy_restart_us = 0;              /* 측정 시작 시간 (us)  */
boss_u32_t  _spy_elapse_us  = 0;              /* 경과 시간 (us)       */
boss_tcb_t  *_spy_tcb_tbl[_BOSS_SPY_TCB_MAX]; /* SPY TCB list         */


/*===========================================================================
    _   B O S S _ S P Y _ E L A P S E _ U S
---------------------------------------------------------------------------*/
boss_u32_t _Boss_spy_elapse_us(void)
{
  boss_u32_t us;
  boss_u32_t reload = OCR1A + 1;
  boss_u32_t tmr_val = TCNT1;     /* count value */
  
                                /* SysTick->VAL => micro second */
  us = _spy_elapse_us + ((tmr_val * (_BOSS_TICK_MS_ * 1000)) / reload);
  
  if(TIFR & (1 << OCF1A) ) {
    us = us + (_BOSS_TICK_MS_ * 1000);
  }

  return us;
}


/*===========================================================================
    _   B O S S _ S P Y _ T I C K
---------------------------------------------------------------------------*/
void _Boss_spy_tick(boss_tmr_ms_t tick_ms)
{
  _spy_elapse_us += (boss_u32_t)(tick_ms * 1000);   /* tick_ms -> us */
  
  if(_spy_elapse_us > 4200000000u)  /* 70분 = 4,200,000,000 us */
  { /* 70분 동안 Boss_spy_report() 호출 되지 않으면 재시작 */
    boss_reg_t idx;
    
    BOSS_IRQ_DISABLE();
    _spy_elapse_us = 0;
    
    for(idx = 0; idx < _BOSS_SPY_TCB_MAX; idx++)
    {
      if(_spy_tcb_tbl[idx] != _BOSS_NULL)
      {
        boss_tcb_t *p_tcb = _spy_tcb_tbl[idx];
        p_tcb->cpu_ent_us = 0;
        p_tcb->cpu_sum_us = 0;
      }
    }
    BOSS_IRQ_RESTORE();
  }
}


/*===========================================================================
    _   B O S S _ S P Y _ C O N T E X T
---------------------------------------------------------------------------*/
void _Boss_spy_context(boss_tcb_t *curr_tcb, boss_tcb_t *best_tcb)
{
  { /* [ Stack ] */
    BOSS_ASSERT(curr_tcb->sp_begin[-1] == (boss_stk_t)0xEEEEEEEE);  // Stack invasion (Begin)
    BOSS_ASSERT(curr_tcb->sp_finis[0] == (boss_stk_t)0xEEEEEEEE);   // Stack invasion (finis)

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
  }

  { /* [ C P U ] */
    boss_u32_t now_us = _Boss_spy_elapse_us();

    curr_tcb->cpu_sum_us += now_us - curr_tcb->cpu_ent_us;
    best_tcb->cpu_ent_us = now_us;
  }
  
  /* [ Context Switch Number ] */
  best_tcb->context++;
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
    
    p_tcb->sp_finis = &sp_base[0];
    p_tcb->sp_peak  = &sp_base[size-1];
    p_tcb->sp_begin = &sp_base[size];
  }

  /* [ C P U ] */
  p_tcb->cpu_ent_us = 0;
  p_tcb->cpu_sum_us = 0;

  /* [ Context Switch Number ] */
  p_tcb->context = 0;
}


/*===========================================================================
    B O S S _ S P Y _ R E P O R T
---------------------------------------------------------------------------*/
void Boss_spy_report(void)
{
  boss_tcb_t *curr_tcb;
  boss_u32_t total_us;
  boss_u32_t restart_us;
  
  boss_reg_t idx;

  _Boss_sched_lock();
  BOSS_IRQ_DISABLE();
  total_us = _Boss_spy_elapse_us();
  _spy_elapse_us = 0;
  restart_us = _Boss_spy_elapse_us();
  
  curr_tcb = Boss_self();
  curr_tcb->cpu_sum_us += total_us - curr_tcb->cpu_ent_us;
  curr_tcb->cpu_ent_us = restart_us;

  total_us = total_us - _spy_restart_us;
  _spy_restart_us = restart_us;
  BOSS_IRQ_RESTORE();

  PRINTF("\r\n[NAME]\t STACK %%(u/t)\t C P U\t   Context\r\n");
  PRINTF("------------------------------------------\r\n");
  for(idx = 0; idx < _BOSS_SPY_TCB_MAX; idx++)
  {
    if(_spy_tcb_tbl[idx] != _BOSS_NULL)
    {
      boss_tcb_t *p_tcb = _spy_tcb_tbl[idx];

      PRINTF("[%s]", p_tcb->name);

      { /* [ Stack ] */
        boss_uptr_t stk_total = (boss_uptr_t)p_tcb->sp_begin - (boss_uptr_t)p_tcb->sp_finis;
        boss_uptr_t stk_used  = (boss_uptr_t)p_tcb->sp_begin - (boss_uptr_t)p_tcb->sp_peak;
        boss_reg_t  stk_percent = (boss_reg_t)(((boss_u32_t)stk_used * 100) / (boss_u32_t)stk_total);

        PRINTF("\t% 2d%%(%3d/%3d)",stk_percent, stk_used, stk_total);
      }

      { /* [ C P U ] */
        boss_u32_t cpu_percent = 0;    /* percent XX.xxx % */
        
        if(p_tcb->cpu_sum_us != 0)
        {
          cpu_percent = (boss_u32_t)(((boss_u64_t)(p_tcb->cpu_sum_us) * 100 * 1000)
                                                      / (boss_u64_t)total_us);
          p_tcb->cpu_sum_us = 0;
        }
        
        PRINTF("\t%2d.%03d%%", (int)(cpu_percent/1000), (int)(cpu_percent%1000));
      }
      
      PRINTF("\t   %7d\r\n", p_tcb->context);
      p_tcb->context = 0;
    }
  }
  
  PRINTF("\r\n");
  _Boss_sched_free();
}
#endif /* _BOSS_SPY_ */



/*
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*                                                                             *
*                       [ ATmega128 UART0 (printf) ]                          *
*                                                                             *
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*/
int _put_char(char c, FILE *fp);


/*===========================================================================
    _ P R I N T F _ I N I T I A L
---------------------------------------------------------------------------*/
void _printf_initial(void)
{
  /* Set baud rate (16Mhz 115200bps) */
  UBRR0H = 0;
  UBRR0L = 8;
  UCSR0A &= ~(1 << U2X0);     /* Disable Double USART Speed */

  /* Enable receiver and transmitter  */
  UCSR0B = (1<<RXEN0) | (1<<TXEN0);
 
  /* Set frame format: 8data, 1stop bit */
  UCSR0C = (3 << UCSZ00);

  (void)fdevopen(_put_char, NULL);
}



/*===========================================================================
    _ P U T _ C H A R
---------------------------------------------------------------------------*/
int _put_char(char c, FILE *fp)
{  
  while ( !( UCSR0A & (1<<UDRE0)) );
  UDR0 = c;
  
  return c;
}


/*===========================================================================
    _ A S S E R T
---------------------------------------------------------------------------*/
void _assert(const char *file, unsigned int line)
{
  cli();
  PRINTF("\r\n ASSERT :");
  printf_P(file);
  PRINTF(" %d", line);
  for(;;)
  {
  }
}


/*===========================================================================
    _   A   T M E G A 1 2 8 _ T I M E R 1 _ I N I T
---------------------------------------------------------------------------*/
void _ATmega128_timer1_init(void)
{
  TCCR1A = 0x00;            /* WGM1[3:0] = 4 (CTC)              */
  TCCR1B = 0x0A;            /* prescaler = 8                    */
  TCCR1C = 0x00;

  OCR1A  = 20000 - 1;       /* (8 * (1 + 19999) ) / 16MHZ = 10ms */

  TCNT1  = 0;               /* Clear counter1                   */ 
  TIMSK = 1 << OCIE1A;      /* Enable OC1A interrupt            */
}


/*===========================================================================
    B O S S _ D E V I C E _ I N I T
---------------------------------------------------------------------------*/
void Boss_device_init(void)
{
  _ATmega128_timer1_init();
  _printf_initial();
}


/*===========================================================================
   Timer Compare Match A  (ATmega128 Timer1 ISR)
---------------------------------------------------------------------------*/
ISR(TIMER1_COMPA_vect)
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

