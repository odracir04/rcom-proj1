#ifndef STATS_H
#define STATS_H

typedef struct {
    int sentFrames;
    int receivedFrames;
    int rejectedFrames;
    int retransmissions;
    int timeouts;
    double elapsedTime;
} LinkLayerStats;

#endif
