#define _POSIX_C_SOURCE 200112L
#include "cluster_client.h"

#include <arpa/inet.h>
#include <errno.h>
#include <math.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#define REQUEST_CAPACITY 2048
#define RESPONSE_LINE_CAPACITY 128
#define API_KEY_MAX 256

static int send_all(int fd, const char *data, size_t length)
{
    size_t sent = 0;
    while (sent < length) {
        ssize_t result = send(fd, data + sent, length - sent, 0);
        if (result < 0 && errno == EINTR) {
            continue;
        }
        if (result <= 0) {
            return -1;
        }
        sent += (size_t)result;
    }
    return 0;
}

static int valid_api_key(const char *key)
{
    size_t length;
    if (key == NULL || (length = strlen(key)) == 0 || length > API_KEY_MAX) {
        return 0;
    }
    for (size_t i = 0; i < length; i++) {
        unsigned char c = (unsigned char)key[i];
        if (c < 0x21 || c > 0x7e) {
            return 0;
        }
    }
    return 1;
}

static int valid_gear(const char *gear)
{
    return gear != NULL &&
        (strcmp(gear, "P") == 0 || strcmp(gear, "R") == 0 ||
         strcmp(gear, "N") == 0 || strcmp(gear, "D") == 0 ||
         strcmp(gear, "B") == 0);
}

static int in_range(double value, double minimum, double maximum)
{
    return isfinite(value) && value >= minimum && value <= maximum;
}

int cluster_post_telemetry(const ClusterTelemetry *telemetry, unsigned short port)
{
    if (telemetry == NULL || port == 0 || !valid_gear(telemetry->gear) ||
        !in_range(telemetry->speed, 0.0, 260.0) ||
        !in_range(telemetry->power, -100.0, 300.0) ||
        !in_range(telemetry->battery, 0.0, 100.0) ||
        !in_range(telemetry->range, 0.0, 1000.0) ||
        !in_range(telemetry->temperature, -40.0, 100.0) ||
        !in_range(telemetry->odometer, 0.0, 9999999.0) ||
        (telemetry->left != 0 && telemetry->left != 1) ||
        (telemetry->right != 0 && telemetry->right != 1) ||
        (telemetry->lights != 0 && telemetry->lights != 1) ||
        (telemetry->seatbelt != 0 && telemetry->seatbelt != 1) ||
        (telemetry->warning != 0 && telemetry->warning != 1)) {
        fprintf(stderr, "Invalid telemetry snapshot\n");
        return -1;
    }

    const char *api_key = getenv("API_KEY");
    if (!valid_api_key(api_key)) {
        fprintf(stderr, "Set API_KEY to a non-empty printable API key (max 256 chars)\n");
        return -1;
    }

    char body[768];
    int body_length = snprintf(
        body, sizeof(body),
        "{\"speed\":%.1f,\"power\":%.1f,\"battery\":%.1f,\"range\":%.1f,"
        "\"gear\":\"%s\",\"temperature\":%.1f,\"odometer\":%.1f,"
        "\"left\":%s,\"right\":%s,\"lights\":%s,\"seatbelt\":%s,\"warning\":%s}",
        telemetry->speed, telemetry->power, telemetry->battery, telemetry->range,
        telemetry->gear, telemetry->temperature, telemetry->odometer,
        telemetry->left ? "true" : "false", telemetry->right ? "true" : "false",
        telemetry->lights ? "true" : "false", telemetry->seatbelt ? "true" : "false",
        telemetry->warning ? "true" : "false");
    if (body_length < 0 || (size_t)body_length >= sizeof(body)) {
        fprintf(stderr, "Telemetry JSON is too large\n");
        return -1;
    }

    char request[REQUEST_CAPACITY];
    int request_length = snprintf(
        request, sizeof(request),
        "POST /api/telemetry HTTP/1.1\r\n"
        "Host: 127.0.0.1:%u\r\n"
        "Authorization: Bearer %s\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n\r\n"
        "%s",
        (unsigned int)port, api_key, body_length, body);
    if (request_length < 0 || (size_t)request_length >= sizeof(request)) {
        fprintf(stderr, "HTTP request is too large\n");
        return -1;
    }

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        return -1;
    }

    const struct timeval timeout = {.tv_sec = 2, .tv_usec = 0};
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0 ||
        setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("setsockopt");
        close(fd);
        return -1;
    }

    struct sockaddr_in address = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr.s_addr = htonl(INADDR_LOOPBACK)
    };
    if (connect(fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("connect to local cluster");
        close(fd);
        return -1;
    }
    if (send_all(fd, request, (size_t)request_length) < 0) {
        perror("send telemetry");
        close(fd);
        return -1;
    }

    char response_line[RESPONSE_LINE_CAPACITY];
    size_t used = 0;
    char chunk[256];
    while (used + 1 < sizeof(response_line)) {
        ssize_t count = recv(fd, chunk, sizeof(chunk), 0);
        if (count < 0 && errno == EINTR) {
            continue;
        }
        if (count <= 0) {
            break;
        }
        for (ssize_t i = 0; i < count && used + 1 < sizeof(response_line); i++) {
            response_line[used++] = chunk[i];
            if (chunk[i] == '\n') {
                response_line[used] = '\0';
                int status = 0;
                if (sscanf(response_line, "HTTP/%*u.%*u %d", &status) != 1) {
                    fprintf(stderr, "Malformed response from cluster\n");
                    close(fd);
                    return -1;
                }
                close(fd);
                if (status < 200 || status >= 300) {
                    fprintf(stderr, "Cluster rejected telemetry (HTTP %d)\n", status);
                    return -1;
                }
                return 0;
            }
        }
    }

    fprintf(stderr, "No complete HTTP response from cluster\n");
    close(fd);
    return -1;
}
