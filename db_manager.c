// db_manager.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <errno.h>

// Include MySQL and MQTT headers
#include <mysql/mysql.h>
#include "MQTTClient.h"

// --- Configuration ---
// MQTT
#define MQTT_ADDRESS   "tcp://localhost:1883"
#define MQTT_CLIENTID  "DbManagerClient"
#define MQTT_TOPIC     "esp32/cds"
#define MQTT_QOS       1

// MySQL
#define DB_HOST        "localhost"
#define DB_USER        "sensor_user"
#define DB_PASS        "password" // <-- IMPORTANT: Change this to your password
#define DB_NAME        "sensor_db"

// --- Global Handles ---
MYSQL *conn;
MQTTClient client;

// --- Helper Function to Insert Data into MySQL ---
void insert_reading(const char* sensor_name, const char* value) {
    char query[512];

    // --- CHECK AND RECONNECT LOGIC ---
    // Before executing a query, ping the server to check if the connection is alive.
    if (mysql_ping(conn) != 0) {
        fprintf(stderr, "MySQL connection lost. Attempting to reconnect...\n");
        if (mysql_real_connect(conn, DB_HOST, DB_USER, DB_PASS, DB_NAME, 0, NULL, 0) == NULL) {
            fprintf(stderr, "Reconnect failed: %s\n", mysql_error(conn));
            // If reconnection fails, we cannot proceed.
            return;
        }
        printf("Reconnect successful.\n");
    }
    // --- END OF RECONNECT LOGIC ---
    
    // Create the SQL query string. This uses a subquery to find the sensor_id from the sensor_name.
    sprintf(query, "INSERT INTO readings (sensor_id, value) VALUES ((SELECT sensor_id FROM sensors WHERE sensor_name = '%s'), '%s')",
            sensor_name, value);

    // Execute the query
    if (mysql_query(conn, query)) {
        fprintf(stderr, "MySQL Insert Error: %s\n", mysql_error(conn));
    } else {
        printf("MySQL Insert Success: sensor='%s', value='%s'\n", sensor_name, value);
    }
}


// --- MQTT Callbacks ---
void connection_lost(void *context, char *cause) {
    printf("\nMQTT Connection lost. Cause: %s\n", cause);
}

int message_arrived(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    char value_buffer[256];
    
    // Ensure payload is null-terminated
    int payload_len = message->payloadlen;
    if (payload_len >= sizeof(value_buffer)) {
        payload_len = sizeof(value_buffer) - 1;
    }
    strncpy(value_buffer, message->payload, payload_len);
    value_buffer[payload_len] = '\0';
    
    printf("MQTT Message Arrived: topic='%s', payload='%s'\n", topicName, value_buffer);
    
    // Insert the received data into the database
    insert_reading(topicName, value_buffer);

    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}


// --- Main Program ---
int main() {
    int pipe_fd[2];
    pid_t pid;

    // 1. Setup MySQL Connection
    conn = mysql_init(NULL);
    if (conn == NULL) {
        fprintf(stderr, "mysql_init() failed\n");
        exit(EXIT_FAILURE);
    }
    if (mysql_real_connect(conn, DB_HOST, DB_USER, DB_PASS, DB_NAME, 0, NULL, 0) == NULL) {
        fprintf(stderr, "mysql_real_connect() failed: %s\n", mysql_error(conn));
        mysql_close(conn);
        exit(EXIT_FAILURE);
    }
    printf("MySQL connection successful.\n");

    // 2. Create Pipe and Fork
    if (pipe(pipe_fd) == -1) { perror("pipe"); exit(EXIT_FAILURE); }
    pid = fork();
    if (pid == -1) { perror("fork"); exit(EXIT_FAILURE); }

    if (pid == 0) {
        // --- Child Process ---
        close(pipe_fd[0]);
        dup2(pipe_fd[1], STDOUT_FILENO);
        close(pipe_fd[1]);
        execlp("./random_simulator", "random_simulator", NULL);
        perror("execlp");
        exit(EXIT_FAILURE);
    } else {
        // --- Parent Process ---
        close(pipe_fd[1]);

        // 3. Setup MQTT Client
        MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
        MQTTClient_create(&client, MQTT_ADDRESS, MQTT_CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
        conn_opts.keepAliveInterval = 20;
        conn_opts.cleansession = 1;
        MQTTClient_setCallbacks(client, NULL, connection_lost, message_arrived, NULL);

        int rc;
        if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
            printf("Failed to connect to MQTT, return code %d\n", rc);
            exit(EXIT_FAILURE);
        }
        printf("MQTT connection successful.\n");
        MQTTClient_subscribe(client, MQTT_TOPIC, MQTT_QOS);
        printf("Subscribed to MQTT topic: %s\n\n", MQTT_TOPIC);

        // 4. Main Event Loop
        printf("Listening for data from pipe and MQTT...\n");
        while (1) {
            fd_set read_fds;
            struct timeval tv;
            FD_ZERO(&read_fds);
            FD_SET(pipe_fd[0], &read_fds);
            tv.tv_sec = 1; tv.tv_usec = 0;

            int activity = select(pipe_fd[0] + 1, &read_fds, NULL, NULL, &tv);

            if (activity < 0 && errno != EINTR) { perror("select error"); }

            if (FD_ISSET(pipe_fd[0], &read_fds)) {
                char buffer[128];
                ssize_t bytes_read = read(pipe_fd[0], buffer, sizeof(buffer) - 1);
                if (bytes_read > 0) {
                    buffer[bytes_read] = '\0';
                    // Trim newline character from the end
                    buffer[strcspn(buffer, "\n")] = 0;
                    printf("Received from Pipe: value='%s'\n", buffer);
                    // Insert pipe data into database
                    insert_reading("random_simulator", buffer);
                } else {
                    printf("\nChild process finished. Exiting.\n");
                    break;
                }
            }
            // Yield to MQTT client to process messages
            MQTTClient_yield();
        }
        
        // 5. Cleanup
        printf("Cleaning up...\n");
        MQTTClient_disconnect(client, 10000);
        MQTTClient_destroy(&client);
        mysql_close(conn);
        close(pipe_fd[0]);
        wait(NULL);
    }
    return 0;
}

