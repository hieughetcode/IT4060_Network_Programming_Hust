#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 9000
#define MAX_SUBS 200

// Cấu trúc lưu trữ 1 lượt đăng ký
typedef struct
{
    int sockfd;
    char topic[50];
    // biến active lưu trạng thái 1 nếu đang dùng, 0 nếu ô trống
    int active;
} Subscription;

Subscription subs[MAX_SUBS];
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

// Đăng ký (SUB)
void add_sub(int sockfd, const char *topic)
{
    pthread_mutex_lock(&lock);
    for (int i = 0; i < MAX_SUBS; i++)
    {
        if (subs[i].active && subs[i].sockfd == sockfd && strcmp(subs[i].topic, topic) == 0)
        {
            pthread_mutex_unlock(&lock);
            return;
        }
    }

    for (int i = 0; i < MAX_SUBS; i++)
    {
        if (!subs[i].active)
        {
            subs[i].sockfd = sockfd;
            strncpy(subs[i].topic, topic, 49);
            subs[i].active = 1;
            break;
        }
    }
    pthread_mutex_unlock(&lock);
}

// Hủy đăng ký(UNSUB)
void remove_sub(int sockfd, const char *topic)
{
    pthread_mutex_lock(&lock);
    for (int i = 0; i < MAX_SUBS; i++)
    {
        if (subs[i].active && subs[i].sockfd == sockfd && strcmp(subs[i].topic, topic) == 0)
        {
            subs[i].active = 0;
        }
    }
    pthread_mutex_unlock(&lock);
}

// Xóa toàn bộ đăng ký khi client ngắt kết nối
void remove_all_client_subs(int sockfd)
{
    pthread_mutex_lock(&lock);
    for (int i = 0; i < MAX_SUBS; i++)
    {
        if (subs[i].active && subs[i].sockfd == sockfd)
        {
            subs[i].active = 0;
        }
    }
    pthread_mutex_unlock(&lock);
}

// Chuyển tiếp tin nhắn (PUB)
void publish_msg(const char *topic, const char *msg)
{
    char send_buf[2048];
    snprintf(send_buf, sizeof(send_buf), "[TIN NHAN - %s]: %s\n", topic, msg);

    pthread_mutex_lock(&lock);
    for (int i = 0; i < MAX_SUBS; i++)
    {
        if (subs[i].active && strcmp(subs[i].topic, topic) == 0)
        {
            send(subs[i].sockfd, send_buf, strlen(send_buf), 0);
        }
    }
    pthread_mutex_unlock(&lock);
}

// Luồng xử lý cho từng client
void *handle_client(void *arg)
{
    int sockfd = *(int *)arg;
    free(arg);
    char buffer[2048];

    printf("[NEW CONNECTION] Client socket %d da ket noi.\n", sockfd);

    while (1)
    {
        memset(buffer, 0, sizeof(buffer));
        int bytes_read = recv(sockfd, buffer, sizeof(buffer) - 1, 0);

        if (bytes_read <= 0)
        {
            break;
        }

        buffer[strcspn(buffer, "\r\n")] = 0;
        if (strlen(buffer) == 0)
            continue;

        char cmd[20] = {0}, topic[50] = {0}, msg[1024] = {0};
        int parsed = sscanf(buffer, "%19s %49s %1023[^\n]", cmd, topic, msg);

        if (strcmp(cmd, "SUB") == 0 && parsed >= 2)
        {
            add_sub(sockfd, topic);
            char ack[100];
            snprintf(ack, sizeof(ack), "OK: Da dang ky chu de '%s'\n", topic);
            send(sockfd, ack, strlen(ack), 0);
            printf("[Socket %d] SUB -> %s\n", sockfd, topic);
        }
        else if (strcmp(cmd, "UNSUB") == 0 && parsed >= 2)
        {
            remove_sub(sockfd, topic);
            char ack[100];
            snprintf(ack, sizeof(ack), "OK: Da huy dang ky chu de '%s'\n", topic);
            send(sockfd, ack, strlen(ack), 0);
            printf("[Socket %d] UNSUB -> %s\n", sockfd, topic);
        }
        else if (strcmp(cmd, "PUB") == 0 && parsed == 3)
        {
            publish_msg(topic, msg);
            char ack[100];
            snprintf(ack, sizeof(ack), "OK: Da gui tin nhan den '%s'\n", topic);
            send(sockfd, ack, strlen(ack), 0);
            printf("[Socket %d] PUB -> %s: %s\n", sockfd, topic, msg);
        }
        else
        {
            const char *err = "ERROR: Sai cu phap. Dung: SUB <topic> | UNSUB <topic> | PUB <topic> <msg>\n";
            send(sockfd, err, strlen(err), 0);
        }
    }

    printf("[DISCONNECT] Client socket %d da ngat ket noi.\n", sockfd);
    remove_all_client_subs(sockfd);
    close(sockfd);
    return NULL;
}

int main()
{
    int server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Khởi tạo socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // Cấu hình địa chỉ và port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0)
    {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("[STARTING] Server dang lang nghe tren cong %d...\n", PORT);

    while (1)
    {
        client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (client_socket < 0)
        {
            perror("Accept failed");
            continue;
        }

        int *new_sock = malloc(sizeof(int));
        *new_sock = client_socket;

        pthread_t thread_id;
        pthread_create(&thread_id, NULL, handle_client, (void *)new_sock);
        // Tự động dọn dẹp tài nguyên thread khi luồng kết thúc
        pthread_detach(thread_id);
    }

    return 0;
}