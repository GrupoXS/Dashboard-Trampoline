#ifndef CLUSTER_CLIENT_H
#define CLUSTER_CLIENT_H

typedef struct {
    double speed;
    double power;
    double battery;
    double range;
    const char *gear;
    double temperature;
    double odometer;
    int left;
    int right;
    int lights;
    int seatbelt;
    int warning;
} ClusterTelemetry;

/* POST one telemetry snapshot to the local Node cluster API. */
int cluster_post_telemetry(const ClusterTelemetry *telemetry, unsigned short port);

#endif
