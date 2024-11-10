#include "../include/alarm.h"
#include "../include/stats.h"
#include "../include/link_layer.h"
#include <stdio.h>

int alarmEnabled = FALSE;
int alarmCount = 0;
extern LinkLayerStats stats;
extern LinkLayer connection;

void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount++;
    stats.timeouts++;
    printf("Alarm #%d\n", alarmCount);
    if (alarmCount < connection.nRetransmissions) stats.retransmissions++;
}