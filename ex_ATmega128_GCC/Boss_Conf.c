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
#include "Boss.h"
#include <avr/io.h>
#include <avr/interrupt.h>

/*===========================================================================*/
/*                      DEFINITIONS & TYPEDEFS & MACROS                      */
/*---------------------------------------------------------------------------*/

/*===========================================================================*/
/*                             GLOBAL VARIABLES                              */
/*---------------------------------------------------------------------------*/

/*===========================================================================*/
/*                            FUNCTION PROTOTYPES                            */
/*---------------------------------------------------------------------------*/
void _ATmega128_timer1_init(void);
void _printf_initial(void);

/*===========================================================================
   Timer Compare Match A  (ATmega128 Timer1 ISR)
---------------------------------------------------------------------------*/
ISR(TIMER1_COMPA_vect)
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
  _ATmega128_timer1_init();
  _printf_initial();
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
    _ P U T _ C H A R
---------------------------------------------------------------------------*/
int _put_char(char c, FILE *fp)
{  
  while ( !( UCSR0A & (1<<UDRE0)) );
  UDR0 = c;
  
  return c;
}


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
    _ A S S E R T
---------------------------------------------------------------------------*/
void _assert(unsigned int line)
{
  cli();
  PRINTF("\r\n ASSERT : %d line", line);
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
  BOSS_ASSERT(curr_tcb->sp_begin[-1] == (boss_stk_t)0xBB);    // Stack invasion (Begin)
  BOSS_ASSERT(curr_tcb->sp_finis[0] == (boss_stk_t)0xFF);     // Stack invasion (finis)


  if( (curr_tcb->sp_peak[0] != (boss_stk_t)0xEE) 
      || (curr_tcb->sp_peak[-1] != (boss_stk_t)0xEE)
      || (curr_tcb->sp_peak[-2] != (boss_stk_t)0xEE)
      || (curr_tcb->sp_peak[-3] != (boss_stk_t)0xEE) )
  {
    if( (curr_tcb->sp_peak - 4) > curr_tcb->sp_finis)
    {
      curr_tcb->sp_peak -= 4;
    }
    
    while( (curr_tcb->sp_peak > curr_tcb->sp_finis) 
                  && (*curr_tcb->sp_peak != (boss_stk_t)0xEE) )
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

