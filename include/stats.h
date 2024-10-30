#ifndef STATS_H
#define STATS_H

typedef struct {
    int totalFrames;
    int rejectedFrames;
    int retransmissions;
    int timeouts;
    double elapsedTime;
} LinkLayerStats;

#endif