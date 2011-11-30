#ifndef _BOSS_CONF_H_
#define _BOSS_CONF_H_
/*
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*                                                                             *
*                 RT-BOSS (Config File)         [ ATmega128 ]                 *
*                                                                             *
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*/

/*===========================================================================*/
/*                            RT-BOSS ��������                               */
/*---------------------------------------------------------------------------*/
typedef unsigned char       boss_u08_t;     /* unsigned  8bit �������� */
typedef unsigned int        boss_u16_t;     /* unsigned 16bit �������� */
typedef unsigned long int   boss_u32_t;     /* unsigned 32bit �������� */

typedef boss_u08_t          boss_reg_t;     /* MCU �������� ũ��    */
typedef boss_u16_t          boss_uptr_t;    /* unsigned ������ ũ�� */
typedef boss_u08_t          boss_stk_t;     /* ����                 */
typedef boss_u08_t          boss_byte_t;    /* Byte                 */

typedef boss_u16_t          boss_sigs_t;    /* �ñ׳�             */
typedef boss_u16_t          boss_tmr_t;     /* Ÿ�̸� ī��Ʈ(ms)  */


/*---------------------------------------------------------------------------*/
/*  [ ���ͷ�Ʈ / ISR ]                                                       */
/*                                                                           */
/*  BOSS_IRQ_LOCK() : ���ͷ�Ʈ ���� �� ��Ȱ��ȭ                              */
/*  BOSS_IRQ_FREE() : ���ͷ�Ʈ ���� ���·� ����                              */
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
/*                            CONFIG �Լ� ����                               */
/*---------------------------------------------------------------------------*/
void *_BOSS_TMR_ALLOC(boss_uptr_t size);
void _BOSS_TMR_FREE(void *p_tmr);

void *_BOSS_MSG_FIFO_ALLOC(boss_uptr_t size);
void _BOSS_MSG_FIFO_FREE(void *msg_fifo);

void *_BOSS_MAIL_Q_BOX_ALLOC(boss_uptr_t size);
void _BOSS_MAIL_Q_BOX_FREE(void *h_mbox);


/*===========================================================================*/
/*                          CONFIG ��ũ�� ����                               */
/*---------------------------------------------------------------------------*/
#define BOSS_ASSERT(expr)  do { if(!(expr)) assert_failed(__LINE__); } while(0)
void assert_failed(unsigned int line);

#define _MCU_ISR_EXECUTE(isr_func)    do {                    \
                                        _Boss_sched_lock();   \
                                        _mcu_isr_begin();     \
                                        isr_func();           \
                                        _mcu_isr_finis();     \
                                        _Boss_sched_free();   \
                                      } while(0)

void _mcu_isr_begin(void);
void _mcu_isr_finis(void);


/*===========================================================================*/
/*                           RT-BOSS ����� ����                             */
/*---------------------------------------------------------------------------*/
#define _BOSS_TICK_MS_          10      /* Tick (ms)  */
#define _BOSS_MEM_POOL_SIZE     1024    /* Bytes      */
#define _IDLE_STACK_BYTES       100     /* Bytes      */


/*===========================================================================*/
/*                            �ñ׳� ��Ʈ ����                               */
/*---------------------------------------------------------------------------*/
/* ���� 4��Ʈ�� RT-BOSS���� ����� (SIG_15_BIT ~ SIG_12_BIT)  */

#define SIG_11_BIT          (boss_sigs_t)(1 << 11)      /* 0x0800 */
#define SIG_10_BIT          (boss_sigs_t)(1 << 10)      /* 0x0400 */
#define SIG_09_BIT          (boss_sigs_t)(1 << 9)       /* 0x0200 */
#define SIG_08_BIT          (boss_sigs_t)(1 << 8)       /* 0x0100 */

#define SIG_07_BIT          (boss_sigs_t)(1 << 7)       /* 0x0080 */
#define SIG_06_BIT          (boss_sigs_t)(1 << 6)       /* 0x0040 */
#define SIG_05_BIT          (boss_sigs_t)(1 << 5)       /* 0x0020 */
#define SIG_04_BIT          (boss_sigs_t)(1 << 4)       /* 0x0010 */
#define SIG_03_BIT          (boss_sigs_t)(1 << 3)       /* 0x0008 */
#define SIG_02_BIT          (boss_sigs_t)(1 << 2)       /* 0x0004 */
#define SIG_01_BIT          (boss_sigs_t)(1 << 1)       /* 0x0002 */
#define SIG_00_BIT          (boss_sigs_t)(1 << 0)       /* 0x0001 */


/*===========================================================================*/
/*                             �½�ũ �켱����                               */
/*---------------------------------------------------------------------------*/
typedef enum {
  PRIO_BOSS_KERNEL = 0,   /* ���� �켱���� */
  
  PRIO_AA_1,              /* TOP 1 */
  PRIO_BB_2,              /* MID 2 */
  PRIO_CC_3,              /* LOW 3 */
  
  PRIO_BOSS_IDLE = 255,   /* ���� �켱���� */
} boss_prio_t;


/*===========================================================================*/
/*                           �޽��� ť ��ɾ�                                */
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
