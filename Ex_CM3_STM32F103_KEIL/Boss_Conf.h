#ifndef _BOSS_CONF_H_
#define _BOSS_CONF_H_
/*
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*                                                                             *
*                 RT-BOSS (Config File)         [ Cortex-M3 ]                 *
*                                                                             *
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*/

/*===========================================================================*/
/*                            RT-BOSS 데이터형                               */
/*---------------------------------------------------------------------------*/
typedef unsigned char       boss_u08_t;     /* unsigned  8bit 데이터형 */
typedef unsigned short int  boss_u16_t;     /* unsigned 16bit 데이터형 */
typedef unsigned int        boss_u32_t;     /* unsigned 32bit 데이터형 */

typedef boss_u32_t          boss_reg_t;     /* MCU 레지스터 크기    */
typedef boss_u32_t          boss_uptr_t;    /* unsigned 포인터 크기 */
typedef boss_u32_t          boss_stk_t;     /* 스택                 */
typedef boss_u08_t          boss_byte_t;    /* Byte                 */

typedef boss_u32_t          boss_sigs_t;    /* 시그널             */
typedef boss_u16_t          boss_tmr_t;     /* 타이머 카운트(ms)  */


/*---------------------------------------------------------------------------*/
/*  [ 인터럽트 / ISR ]                                                       */
/*                                                                           */
/*  BOSS_IRQ_LOCK() : 인터럽트 보관 및 비활성화                              */
/*  BOSS_IRQ_FREE() : 인터럽트 이전 상태로 복원                              */
/*---------------------------------------------------------------------------*/
#define BOSS_IRQ_LOCK()         do { boss_reg_t _irq_store_ = _mcu_irq_lock()
#define BOSS_IRQ_FREE()         _mcu_irq_free(_irq_store_); } while(0)

boss_reg_t  _mcu_isr_running(void);
boss_reg_t  _mcu_irq_hold(void);
boss_reg_t  _mcu_irq_lock(void);
void        _mcu_irq_free(boss_reg_t _irq_store_);


/*---------------------------------------------------------------------------*/
/*                             RT-BOSS (Sleep)                               */
/*---------------------------------------------------------------------------*/
void Boss_sleep(boss_tmr_t sleep_ms);
void Boss_sleep_hms(boss_byte_t h, boss_byte_t m, boss_byte_t s);


/*===========================================================================*/
/*                            CONFIG 함수 정의                               */
/*---------------------------------------------------------------------------*/
void *_BOSS_TMR_ALLOC(boss_uptr_t size);
void _BOSS_TMR_FREE(void *p_tmr);

void *_BOSS_MSG_FIFO_ALLOC(boss_uptr_t size);
void _BOSS_MSG_FIFO_FREE(void *msg_fifo);

void *_BOSS_MAIL_Q_BOX_ALLOC(boss_uptr_t size);
void _BOSS_MAIL_Q_BOX_FREE(void *h_mbox);


/*===========================================================================*/
/*                          CONFIG 메크로 정의                               */
/*---------------------------------------------------------------------------*/
#define BOSS_ASSERT(expr)  do { if(!(expr)) assert_failed(__FILE__,__LINE__); } while(0)
void assert_failed(const unsigned char *file, unsigned int line);

#define _MCU_ISR_EXECUTE(isr_func)    do {                    \
                                        _Boss_sched_lock();   \
                                        isr_func();           \
                                        _Boss_sched_free();   \
                                      } while(0)

/*===========================================================================*/
/*                           RT-BOSS 사용자 설정                             */
/*---------------------------------------------------------------------------*/
#define _BOSS_TICK_MS_          1       /* Tick (ms)  */
#define _BOSS_MEM_POOL_SIZE     1024    /* Bytes      */
#define _IDLE_STACK_BYTES       100     /* Bytes      */


/*===========================================================================*/
/*                            시그널 비트 정의                               */
/*---------------------------------------------------------------------------*/
/* 상위 4비트는 RT-BOSS에서 사용함 (SIG_31_BIT ~ SIG_28_BIT)  */

#define SIG_27_BIT          (boss_sigs_t)(1 << 27)      /* 0x 0800 0000 */
#define SIG_26_BIT          (boss_sigs_t)(1 << 26)      /* 0x 0400 0000 */
#define SIG_25_BIT          (boss_sigs_t)(1 << 25)      /* 0x 0200 0000 */
#define SIG_24_BIT          (boss_sigs_t)(1 << 24)      /* 0x 0100 0000 */

#define SIG_23_BIT          (boss_sigs_t)(1 << 23)      /* 0x 0080 0000 */
#define SIG_22_BIT          (boss_sigs_t)(1 << 22)      /* 0x 0040 0000 */
#define SIG_21_BIT          (boss_sigs_t)(1 << 21)      /* 0x 0020 0000 */
#define SIG_20_BIT          (boss_sigs_t)(1 << 20)      /* 0x 0010 0000 */
#define SIG_19_BIT          (boss_sigs_t)(1 << 19)      /* 0x 0008 0000 */
#define SIG_18_BIT          (boss_sigs_t)(1 << 18)      /* 0x 0004 0000 */
#define SIG_17_BIT          (boss_sigs_t)(1 << 17)      /* 0x 0002 0000 */
#define SIG_16_BIT          (boss_sigs_t)(1 << 16)      /* 0x 0001 0000 */

#define SIG_15_BIT          (boss_sigs_t)(1 << 15)      /* 0x 0000 8000 */
#define SIG_14_BIT          (boss_sigs_t)(1 << 14)      /* 0x 0000 4000 */
#define SIG_13_BIT          (boss_sigs_t)(1 << 13)      /* 0x 0000 2000 */
#define SIG_12_BIT          (boss_sigs_t)(1 << 12)      /* 0x 0000 1000 */
#define SIG_11_BIT          (boss_sigs_t)(1 << 11)      /* 0x 0000 0800 */
#define SIG_10_BIT          (boss_sigs_t)(1 << 10)      /* 0x 0000 0400 */
#define SIG_09_BIT          (boss_sigs_t)(1 << 9)       /* 0x 0000 0200 */
#define SIG_08_BIT          (boss_sigs_t)(1 << 8)       /* 0x 0000 0100 */

#define SIG_07_BIT          (boss_sigs_t)(1 << 7)       /* 0x 0000 0080 */
#define SIG_06_BIT          (boss_sigs_t)(1 << 6)       /* 0x 0000 0040 */
#define SIG_05_BIT          (boss_sigs_t)(1 << 5)       /* 0x 0000 0020 */
#define SIG_04_BIT          (boss_sigs_t)(1 << 4)       /* 0x 0000 0010 */
#define SIG_03_BIT          (boss_sigs_t)(1 << 3)       /* 0x 0000 0008 */
#define SIG_02_BIT          (boss_sigs_t)(1 << 2)       /* 0x 0000 0004 */
#define SIG_01_BIT          (boss_sigs_t)(1 << 1)       /* 0x 0000 0002 */
#define SIG_00_BIT          (boss_sigs_t)(1 << 0)       /* 0x 0000 0001 */


/*===========================================================================*/
/*                             태스크 우선순위                               */
/*---------------------------------------------------------------------------*/
typedef enum {
  PRIO_BOSS_KERNEL = 0,   /* 상위 우선순위 */
  
  PRIO_AA_1,              /* TOP 1 */
  PRIO_BB_2,              /* MID 2 */
  PRIO_CC_3,              /* LOW 3 */
  
  PRIO_BOSS_IDLE = 255,   /* 하위 우선순위 */
} boss_prio_t;


/*===========================================================================*/
/*                           메시지 큐 명령어                                */
/*---------------------------------------------------------------------------*/
typedef enum {
  M_CMD_EMPTY = 0,

  M_CMD_1,
  M_CMD_2,
  /* ... */

  M_CMD_PARAM_LED_ON,
  
  M_CMD_MAX = 255,
} msg_cmd_t;

#endif  /* _BOSS_CONF_H_ */
