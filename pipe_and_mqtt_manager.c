// pipe_and_mqtt_manager.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/select.h>
#include <errno.h> // **FIX 1: Added for 'errno' and 'EINTR'**

// Paho MQTT Library Headers
#include "MQTTClient.h"

// --- MQTT Configuration ---
#define MQTT_ADDRESS   "tcp://localhost:1883"
#define MQTT_CLIENTID  "PipeAndMqttManager"
#define MQTT_TOPIC     "esp32/cds"
#define MQTT_QOS       1

// Global MQTT client handle
MQTTClient client;

// --- MQTT Callback Functions ---

// Callback for when the connection is lost
void connection_lost(void *context, char *cause) {
    printf("\nMQTT Connection lost\n");
    printf("     cause: %s\n", cause);
}

// Callback for when a message arrives
int message_arrived(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    printf("MQTT Message Arrived: ");
    printf("topic: %s, payload: %.*s\n", topicName, message->payloadlen, (char*)message->payload);
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

// --- Main Program ---

int main() {
    int pipe_fd[2];
    pid_t pid;

    // 1. Create the pipe for IPC
    if (pipe(pipe_fd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // 2. Fork to create a child process
    pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // --- Child Process Logic ---
        close(pipe_fd[0]);
        dup2(pipe_fd[1], STDOUT_FILENO);
        close(pipe_fd[1]);
        execlp("./random_simulator", "random_simulator", NULL);
        perror("execlp");
        exit(EXIT_FAILURE);

    } else {
        // --- Parent Process Logic ---
        close(pipe_fd[1]);

        // --- MQTT Client Setup ---
        MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
        int rc;

        MQTTClient_create(&client, MQTT_ADDRESS, MQTT_CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
        conn_opts.keepAliveInterval = 20;
        conn_opts.cleansession = 1;
        MQTTClient_setCallbacks(client, NULL, connection_lost, message_arrived, NULL);

        if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
            printf("Failed to connect to MQTT broker, return code %d\n", rc);
            exit(EXIT_FAILURE);
        }
        printf("Connected to MQTT broker: %s\n", MQTT_ADDRESS);

        MQTTClient_subscribe(client, MQTT_TOPIC, MQTT_QOS);
        printf("Subscribed to topic: %s\n\n", MQTT_TOPIC);

        // --- Main Event Loop using select() ---
        printf("Parent process is now listening for data from pipe and MQTT...\n");
        while (1) {
            fd_set read_fds;
            struct timeval tv;
            int max_fd;
            int activity;
            
            FD_ZERO(&read_fds);
            FD_SET(pipe_fd[0], &read_fds);
            max_fd = pipe_fd[0];

            tv.tv_sec = 1;
            tv.tv_usec = 0;

            activity = select(max_fd + 1, &read_fds, NULL, NULL, &tv);

            if ((activity < 0) && (errno != EINTR)) {
                perror("select error");
            }

            if (FD_ISSET(pipe_fd[0], &read_fds)) {
                char buffer[128];
                ssize_t bytes_read = read(pipe_fd[0], buffer, sizeof(buffer) - 1);
                if (bytes_read > 0) {
                    buffer[bytes_read] = '\0';
                    printf("Received from Pipe: %s", buffer);
                } else {
                    printf("\nChild process has finished. Exiting.\n");
                    break;
                }
            }
            
            // **FIX 2: Call MQTTClient_yield() with no arguments and no return value check.**
            MQTTClient_yield();
        }
        
        // --- Cleanup ---
        printf("Cleaning up and disconnecting.\n");
        MQTTClient_disconnect(client, 10000);
        MQTTClient_destroy(&client);
        close(pipe_fd[0]);
        wait(NULL);
    }

    return 0;
}

