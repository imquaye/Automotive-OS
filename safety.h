#ifndef SAFETY_H
#define SAFETY_H

void safety_check(int fault);
int is_in_safe_mode();
int get_fault_count();
void activate_safe_mode();
void deactivate_safe_mode();

#endif
