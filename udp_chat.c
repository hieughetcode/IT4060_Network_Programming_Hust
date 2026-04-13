#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        printf("Cu phap: %s port_s ip_d port_d\n", argv[0]);
        return 1;
    }

    int port_s = atoi(argv[1]);
    char *ip_d = argv[2];
    int port_d = atoi(argv[3]);

    int sockfd;
    struct sockaddr_in my_addr, dest_addr;
    char buffer[BUFFER_SIZE];

    // Tạo UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Loi tao socket");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));

    // Cấu hình địa chỉ nhận (port_s)
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = INADDR_ANY;
    my_addr.sin_port = htons(port_s);

    if (bind(sockfd, (const struct sockaddr *)&my_addr, sizeof(my_addr)) < 0)
    {
        perror("Loi bind");
        exit(EXIT_FAILURE);
    }

    // Cấu hình địa chỉ đích (ip_d, port_d)
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port_d);
    inet_pton(AF_INET, ip_d, &dest_addr.sin_addr);

    printf("Bat dau chat UDP tren port %d. Dang ket noi toi %s:%d\n", port_s, ip_d, port_d);
    printf("Nhap tin nhan va an Enter de gui...\n");

    fd_set readfds;
    int max_fd = sockfd > STDIN_FILENO ? sockfd : STDIN_FILENO;

    while (1)
    {
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds); // Lắng nghe bàn phím
        FD_SET(sockfd, &readfds);       // Lắng nghe socket

        int activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0)
        {
            perror("Loi select");
            break;
        }

        // Nếu người dùng nhập từ bàn phím
        if (FD_ISSET(STDIN_FILENO, &readfds))
        {
            fgets(buffer, BUFFER_SIZE, stdin);
            sendto(sockfd, buffer, strlen(buffer), 0, (const struct sockaddr *)&dest_addr, sizeof(dest_addr));
        }

        // Nếu có tin nhắn mạng gửi đến
        if (FD_ISSET(sockfd, &readfds))
        {
            socklen_t len = sizeof(dest_addr);
            int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&dest_addr, &len);
            buffer[n] = '\0';
            printf("\r[Doi tac]: %s", buffer);
        }
    }

    close(sockfd);
    return 0;
}