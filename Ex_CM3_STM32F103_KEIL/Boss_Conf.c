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
void _Boss_tick_sleep(boss_tmr_t tick_ms);
void _Boss_tick_timer(boss_tmr_t tick_ms);


/*===========================================================================
    B O S S _ T I C K
---------------------------------------------------------------------------*/
void _Boss_tick(void)
{
  boss_tmr_t tick_ms = _BOSS_TICK_MS_;
  
  _Boss_tick_sleep(tick_ms);
  _Boss_tick_timer(tick_ms);
}


/*===========================================================================
    B O S S _ S L E E P
---------------------------------------------------------------------------*/
void Boss_sleep(boss_tmr_t sleep_ms)
{
  (void)Boss_wait(BOSS_SIGS_NULL, sleep_ms);
}


/*===========================================================================
    B O S S _ S L E E P _ H M S
---------------------------------------------------------------------------*/
void Boss_sleep_hms(boss_byte_t h, boss_byte_t m, boss_byte_t s)
{
  boss_u32_t hms;
  
  hms = ((boss_u32_t)h*60*60) + ((boss_u32_t)m*60) + (boss_u32_t)s;
  hms = hms * 1000; /* ms */

  while(hms != 0)
  {
    boss_tmr_t sleep_ms = ~((boss_tmr_t)0);   /* 타이머 카운트 최대값 */
                                              /* 32bit : 0xFFFFFFFF   */
                                              /* 16bit : 0xFFFF       */
                                              /*  8bit : 0xFF         */
    if( (boss_u32_t)sleep_ms < hms )
    {
      hms = hms - (boss_u32_t)sleep_ms;
    }
    else
    {
      sleep_ms = (boss_tmr_t)hms;
      hms = 0;
    }
    
    (void)Boss_wait(BOSS_SIGS_NULL, sleep_ms);
  }
}



/*===========================================================================
    _   B   O   S   S _   T   M   R _   A   L   L   O   C
---------------------------------------------------------------------------*/
void *_BOSS_TMR_ALLOC(boss_uptr_t size)
{
  return Boss_mem_alloc(size);
}


/*===========================================================================
    _   B   O   S   S _   T   M   R _   F   R   E   E
---------------------------------------------------------------------------*/
void _BOSS_TMR_FREE(void *p_tmr)
{
  Boss_mem_free(p_tmr);
}



/*===========================================================================
    _   B   O   S   S _   M   S   G _   F   I   F   O _   A   L   L   O   C
---------------------------------------------------------------------------*/
void *_BOSS_MSG_FIFO_ALLOC(boss_uptr_t size)
{
  return Boss_mem_alloc(size);
}


/*===========================================================================
    _   B   O   S   S _   M   S   G _   F   I   F   O _   F   R   E   E
---------------------------------------------------------------------------*/
void _BOSS_MSG_FIFO_FREE(void *msg_fifo)
{
  Boss_mem_free(msg_fifo);
}



/*===========================================================================
_   B   O   S   S _   M   A   I   L _   Q _   B   O   X _   A   L   L   O   C
---------------------------------------------------------------------------*/
void *_BOSS_MAIL_Q_BOX_ALLOC(boss_uptr_t size)
{
  return Boss_mem_alloc(size);
}


/*===========================================================================
_   B   O   S   S _   M   A   I   L _   Q _   B   O   X _   F   R   E   E
---------------------------------------------------------------------------*/
void _BOSS_MAIL_Q_BOX_FREE(void *h_mbox)
{
  Boss_mem_free(h_mbox);
}



/*
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*                                                                             *
*                         RT-BOSS ( IDLE TASK )                               *
*                                                                             *
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*/

boss_tcb_t idle_task[1];
boss_stk_t idle_stack[ _IDLE_STACK_BYTES / sizeof(boss_stk_t) ];

/*===========================================================================
    I D L E _ T A S K
---------------------------------------------------------------------------*/
void idle_task_exe(void)
{
  for(;;)
  {
  }
}




/*===========================================================================
    C O N F _ I N I T
---------------------------------------------------------------------------*/
void conf_init(void)
{
  /* RT-BOSS 스케줄러 시작 전 실행함. (인트럽트가 중지 상태에서 실행함) */
  if (SysTick_Config(SystemCoreClock / 1000)) { /* Setup SysTick Timer for 1 msec interrupts  */
    while (1);                                  /* Capture error */
  }
  
  NVIC_SetPriority(PendSV_IRQn, (1<<__NVIC_PRIO_BITS) - 1);  /* PendSV IRQ 우선순위 */
  
  Boss_mem_free( Boss_mem_alloc(1) );         /* RT-BOSS 메모리 초기화 */
}


/*===========================================================================
    S Y S   T I C K _   H A N D L E R (Cortex-M3 SysTick ISR)
---------------------------------------------------------------------------*/
void SysTick_Handler(void)
{
  _MCU_ISR_EXECUTE( _Boss_tick );
}


/*
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*                                                                             *
*                 [ Cortex-M3 SVC_Handler & SVC Vector Table ]                *
*                                                                             *
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*/


/*===========================================================================*/
/*                        SVC FUNCTION PROTOTYPES                            */
/*---------------------------------------------------------------------------*/
void _SVC_Boss_start_schedule(boss_stk_t *cur_task_sp);


/*===========================================================================*/
/*                          SVC FUNCTION TABLE                               */
/*---------------------------------------------------------------------------*/
typedef uint64_t (*svc_func_t)(uint32_t, uint32_t, uint32_t, uint32_t);

const svc_func_t _svc_vector[] = {
  (svc_func_t)_SVC_Boss_start_schedule,     /* SVC 0x00 */
};


/*===========================================================================
    S V C _ H A N D L E R _ E X E C U T E
---------------------------------------------------------------------------*/
void svc_handler_execute(unsigned int *args)
{
  /*
  **  R0 = svc_args[0]; R1 = svc_args[1]; R2 = svc_args[2];  R3 = svc_args[3];
  ** R12 = svc_args[4]; LR = svc_args[5]; PC = svc_args[6]; PSR = svc_args[7];
  */

  uint32_t  svc_num = ((char *)args[6])[-2];  // [(PC) -2]

  if( svc_num >= (sizeof(_svc_vector) / sizeof(svc_func_t)) )
  {
    for(;;);  // svc number error
  }

  *((uint64_t *)args) = _svc_vector[svc_num](args[0], args[1], args[2], args[3]);
}


/*===========================================================================
    S   V   C _   H A N D L E R
---------------------------------------------------------------------------*/
__ASM void SVC_Handler(void)
{
  IMPORT  svc_handler_execute

  TST     LR, #4            // LR(EXC_RETURN) 값을 테스트 하여
  ITE     EQ                
  MRSEQ   R0, MSP           // "0" 이면 "R0"에 MSP 스텍을 저장
  MRSNE   R0, PSP           // "1" 이면 "R0"에 PSP 스텍을 저장

  B      svc_handler_execute

  ALIGN
}



/*===========================================================================
    A S S E R T _ F A I L E D
---------------------------------------------------------------------------*/
unsigned int assert_line;

void assert_failed(const unsigned char *file, unsigned int line)
{
  (void)_mcu_irq_lock();
  assert_line = line;

  while(line != 0)
  {
  }
}

