#include "../include/alarm.h"
#include "../include/stats.h"
#include "../include/link_layer.h"

int alarmEnabled = FALSE;
int alarmCount = 0;
extern LinkLayerStats stats;
extern LinkLayer connection;

void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount++;
    stats.timeouts++;
    if (alarmCount < connection.nRetransmissions) stats.retransmissions++;
}