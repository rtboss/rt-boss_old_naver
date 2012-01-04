#ifndef _BOSS_TMR_H_
#define _BOSS_TMR_H_
/*
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*                                                                             *
*                             RT-BOSS (Timer)                                 *
*                                                                             *
*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*
*/

/*===========================================================================*/
/*                               INCLUDE FILE                                */
/*---------------------------------------------------------------------------*/

/*===========================================================================*/
/*                      DEFINITIONS & TYPEDEFS & MACROS                      */
/*---------------------------------------------------------------------------*/
#define BOSS_TMR_INFINITE               (~(boss_tmr_ms_t)0)

typedef struct boss_tmr_struct {
  boss_tmr_ms_t     tmr_ms;           /* Timer Countdown Millisecond  */
  
  boss_tcb_t        *p_tcb;           /* TCB    */
  boss_sigs_t       sig;              /* Signal */
  
  void (*tmr_cb)(struct boss_tmr_struct *);   /* Callback Function  */
  
  struct boss_tmr_struct  *next;
} boss_tmr_t;


/*===========================================================================*/
/*                            FUNCTION PROTOTYPES                            */
/*---------------------------------------------------------------------------*/
void _Boss_timer_tick(boss_tmr_ms_t tick_ms);

void Boss_tmr_start( boss_tmr_t *p_tmr, boss_tmr_ms_t tmr_ms, 
          boss_tcb_t  *p_tcb, boss_sigs_t sig, void (*tmr_cb)(boss_tmr_t *) );

boss_tmr_t *Boss_tmr_stop(boss_tmr_t *p_tmr);

void Boss_sleep(boss_tmr_ms_t wait_ms);
boss_sigs_t Boss_wait_sleep(boss_sigs_t wait_sigs,  boss_tmr_ms_t wait_ms);

#endif  /* _BOSS_TMR_H_ */
