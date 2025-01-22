#ifndef CONTROL_H
#define CONTROL_H

void controlInit();

void controlLoop();

void controlTask(int cpuid);

extern int id;

#endif //CONTROL_H
