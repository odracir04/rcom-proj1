#include "../include/alarm.h"
#include <stdio.h>

int alarmEnabled = FALSE;
int alarmCount = 0;

void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount++;

    printf("\nAlarm #%d\n", alarmCount);
}