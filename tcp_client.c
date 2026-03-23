#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Cú pháp: %s <địa chỉ IP> <cổng>\n", argv[0]);
        return 1;
    }

    char *ip_address = argv[1];
    int port = atoi(argv[2]);

    // Tạo Socket
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0)
    {
        perror("Lỗi tạo socket");
        return 1;
    }

    // Cấu hình địa chỉ Server
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip_address, &server_addr.sin_addr) <= 0)
    {
        perror("Lỗi địa chỉ IP không hợp lệ");
        close(client_socket);
        return 1;
    }

    // Kết nối đến Server
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Kết nối đến server thất bại");
        close(client_socket);
        return 1;
    }

    printf("Đã kết nối thành công đến server %s:%d\n", ip_address, port);

    // Nhận lời chào từ server
    char buffer[BUFFER_SIZE];
    int bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received > 0)
    {
        buffer[bytes_received] = '\0';
        printf("\n--- LỜI CHÀO TỪ SERVER ---\n");
        printf("%s", buffer);
        printf("\n--------------------------\n\n");
    }
    else
    {
        printf("\n[Không nhận được lời chào từ server hoặc server đã ngắt kết nối]\n\n");
    }
    // ==========================================

    printf("Nhập dữ liệu để gửi (gõ 'exit' để thoát):\n");

    // Vòng lặp gửi dữ liệu
    while (1)
    {
        printf("> ");
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL)
        {
            break;
        }

        buffer[strcspn(buffer, "\n")] = 0;

        if (strcmp(buffer, "exit") == 0)
        {
            break;
        }

        // Gửi dữ liệu
        buffer[strlen(buffer)] = '\n';
        buffer[strlen(buffer) + 1] = '\0';

        if (send(client_socket, buffer, strlen(buffer), 0) < 0)
        {
            perror("Gửi dữ liệu thất bại");
            break;
        }
    }

    // Đóng kết nối
    close(client_socket);
    printf("Đã đóng kết nối.\n");

    return 0;
}