#define _POSIX_C_SOURCE 200809L
#include "cluster_client.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static unsigned short parse_port(const char *text)
{
    char *end = NULL;
    unsigned long value = strtoul(text, &end, 10);
    if (text[0] == '\0' || end == text || *end != '\0' || value == 0 || value > 65535) {
        return 0;
    }
    return (unsigned short)value;
}

static int pause_one_second(void)
{
    struct timespec remaining = {.tv_sec = 1, .tv_nsec = 0};
    while (nanosleep(&remaining, &remaining) < 0) {
        if (errno != EINTR) {
            perror("nanosleep");
            return -1;
        }
    }
    return 0;
}

int main(int argc, char **argv)
{
    int once = argc == 2 && strcmp(argv[1], "--once") == 0;
    if (argc > 2 || (argc == 2 && !once)) {
        fprintf(stderr, "Usage: %s [--once]\n", argv[0]);
        return 2;
    }

    const char *port_text = getenv("CLUSTER_PORT");
    unsigned short port = parse_port(port_text == NULL ? "3000" : port_text);
    if (port == 0) {
        fprintf(stderr, "CLUSTER_PORT must be between 1 and 65535\n");
        return 2;
    }

    unsigned int sample = 0;
    do {
        double speed = 48.0 + (double)((sample * 7U) % 121U);
        ClusterTelemetry telemetry = {
            .speed = speed,
            .power = sample % 4U == 3U ? -18.0 : 24.0 + speed * 0.35,
            .battery = 76.0 - (double)(sample % 50U) * 0.1,
            .range = 304.0 - (double)(sample % 50U) * 1.2,
            .gear = "D",
            .temperature = 25.0 + (double)(sample % 8U),
            .odometer = 12481.0 + (double)sample * 0.01,
            .left = sample % 12U == 5U,
            .right = sample % 12U == 9U,
            .lights = 1,
            .seatbelt = 1,
            .warning = sample % 20U == 19U
        };

        if (cluster_post_telemetry(&telemetry, port) != 0) {
            return 1;
        }
        printf("Sent sample %u to local cluster on port %u\n", sample + 1U, (unsigned int)port);
        fflush(stdout);
        sample++;
        if (!once && pause_one_second() != 0) {
            return 1;
        }
    } while (!once);

    return 0;
}
