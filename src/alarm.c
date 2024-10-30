#include "../include/alarm.h"
#include "../include/stats.h"

int alarmEnabled = FALSE;
int alarmCount = 0;
extern LinkLayerStats stats;

void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount++;
    stats.timeouts++;
    if (alarmCount < 3) stats.retransmissions++;
}