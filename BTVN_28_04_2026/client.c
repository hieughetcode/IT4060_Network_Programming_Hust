#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>

// Luồng nhận tin nhắn từ Server
void *receive_messages(void *arg)
{
    int sockfd = *(int *)arg;
    char buffer[2048];

    while (1)
    {
        memset(buffer, 0, sizeof(buffer));
        int bytes_read = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_read <= 0)
        {
            printf("\n[SERVER DISCONNECTED] Mat ket noi voi server.\n");
            exit(0);
        }
        printf("\n%s>> Nhap lenh: ", buffer);
        fflush(stdout);
    }
    return NULL;
}

int main()
{
    int sockfd;
    struct sockaddr_in serv_addr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("Loi tao socket\n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(9000);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
    {
        printf("Dia chi IP khong hop le\n");
        return -1;
    }

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("Ket noi toi server that bai\n");
        return -1;
    }

    // Tạo luồng nhận tin nhắn
    pthread_t recv_thread;
    pthread_create(&recv_thread, NULL, receive_messages, &sockfd);

    char input[2048];
    printf("Cac lenh ho tro: \n - SUB <topic>\n - UNSUB <topic>\n - PUB <topic> <msg>\n - EXIT (de thoat)\n");

    while (1)
    {
        printf(">> Nhap lenh: ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL)
        {
            break;
        }

        input[strcspn(input, "\n")] = 0;

        if (strcmp(input, "EXIT") == 0 || strcmp(input, "exit") == 0)
        {
            break;
        }

        if (strlen(input) > 0)
        {
            send(sockfd, input, strlen(input), 0);
        }
    }

    close(sockfd);
    return 0;
}