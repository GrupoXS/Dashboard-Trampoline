#define _POSIX_C_SOURCE 200809L
#include "tpl_os.h"
#include "cluster_client.h"

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

static ClusterTelemetry vehicle = {
    .speed = 48.0, .power = 40.8, .battery = 76.0, .range = 304.0,
    .gear = "D", .temperature = 25.0, .odometer = 12481.0,
    .left = 0, .right = 0, .lights = 1, .seatbelt = 1, .warning = 0
};
static unsigned int sample;
static pthread_mutex_t telemetry_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t telemetry_ready = PTHREAD_COND_INITIALIZER;
static ClusterTelemetry pending_telemetry;
static int telemetry_pending;
static unsigned short cluster_port(void);

static void *telemetry_worker(void *unused)
{
    (void)unused;
    for (;;) {
        ClusterTelemetry snapshot;
        pthread_mutex_lock(&telemetry_mutex);
        while (!telemetry_pending) {
            pthread_cond_wait(&telemetry_ready, &telemetry_mutex);
        }
        snapshot = pending_telemetry;
        telemetry_pending = 0;
        pthread_mutex_unlock(&telemetry_mutex);

        if (cluster_post_telemetry(&snapshot, cluster_port()) != 0) {
            fprintf(stderr, "Telemetry POST failed; RTOS tasks continue\n");
        }
    }
    return NULL;
}

static unsigned short cluster_port(void)
{
    const char *text = getenv("CLUSTER_PORT");
    char *end = NULL;
    unsigned long port = text == NULL ? 3000 : strtoul(text, &end, 10);
    if ((text != NULL && (end == text || *end != '\0')) || port == 0 || port > 65535) {
        return 0;
    }
    return (unsigned short)port;
}

int main(void)
{
    pthread_t worker;
    if (cluster_port() == 0) {
        fprintf(stderr, "CLUSTER_PORT must be between 1 and 65535\n");
        return EXIT_FAILURE;
    }
    if (pthread_create(&worker, NULL, telemetry_worker, NULL) != 0) {
        perror("pthread_create telemetry worker");
        return EXIT_FAILURE;
    }
    (void)pthread_detach(worker);
    StartOS(OSDEFAULTAPPMODE);
    return EXIT_SUCCESS;
}

TASK(VehicleSimulationTask)
{
    GetResource(VehicleDataResource);
    sample++;
    vehicle.speed = 48.0 + (double)((sample * 7U) % 121U);
    vehicle.power = sample % 4U == 3U ? -18.0 : 24.0 + vehicle.speed * 0.35;
    vehicle.battery = 76.0 - (double)(sample % 50U) * 0.1;
    vehicle.range = 304.0 - (double)(sample % 50U) * 1.2;
    vehicle.temperature = 25.0 + (double)(sample % 8U);
    vehicle.odometer = 12481.0 + (double)sample * 0.01;
    vehicle.left = sample % 12U == 5U;
    vehicle.right = sample % 12U == 9U;
    vehicle.warning = sample % 20U == 19U;
    ReleaseResource(VehicleDataResource);
    TerminateTask();
}

TASK(DiagnosticsTask)
{
    int invalid;
    GetResource(VehicleDataResource);
    invalid = vehicle.speed < 0.0 || vehicle.speed > 260.0 ||
        vehicle.battery < 0.0 || vehicle.battery > 100.0 || vehicle.warning;
    if (invalid) {
        vehicle.warning = 1;
    }
    ReleaseResource(VehicleDataResource);
    if (invalid) {
        (void)SetEvent(WarningHandlerTask, WarningEvent);
    }
    TerminateTask();
}

TASK(WarningHandlerTask)
{
    (void)WaitEvent(WarningEvent);
    (void)ClearEvent(WarningEvent);
    puts("Vehicle diagnostic warning detected");
    TerminateTask();
}

TASK(TelemetryTask)
{
    ClusterTelemetry snapshot;
    GetResource(VehicleDataResource);
    snapshot = vehicle;
    ReleaseResource(VehicleDataResource);

    /* A POSIX worker performs blocking HTTP outside the Trampoline scheduler. */
    pthread_mutex_lock(&telemetry_mutex);
    pending_telemetry = snapshot;
    telemetry_pending = 1;
    pthread_cond_signal(&telemetry_ready);
    pthread_mutex_unlock(&telemetry_mutex);
    TerminateTask();
}
