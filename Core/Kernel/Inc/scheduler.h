#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

__attribute__((naked)) void scheduler_launch(void);
void scheduler_trigger(void);
void scheduler_rr(void);

#endif
