/*
 * Họ tên: Lê Hữu Hiếu
 * MSSV: 20225315
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <poll.h>
#include <time.h>

#define PORT 9000
#define MAX_CLIENTS 100

// Cấu trúc lưu trạng thái của mỗi client
typedef struct
{
    int fd;
    int is_registered;
    char name[50];
} ClientState;

ClientState clients[MAX_CLIENTS];

// Hàm lấy thời gian hiện tại
void get_current_time(char *buffer, int size)
{
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(buffer, size, "%Y/%m/%d %I:%M:%S%p", timeinfo);
}

int main()
{
    int server_fd;
    struct sockaddr_in address;
    struct pollfd fds[MAX_CLIENTS];
    int nfds = 1;

    // Khởi tạo mảng trạng thái
    for (int i = 0; i < MAX_CLIENTS; i++)
        clients[i].fd = -1;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

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

    printf("[CHAT SERVER] Dang lang nghe tren cong %d...\n", PORT);

    memset(fds, 0, sizeof(fds));
    fds[0].fd = server_fd;
    fds[0].events = POLLIN;

    while (1)
    {
        if (poll(fds, nfds, -1) < 0)
        {
            perror("Poll error");
            break;
        }

        for (int i = 0; i < nfds; i++)
        {
            if (fds[i].revents & POLLIN)
            {
                // Co ket noi moi
                if (fds[i].fd == server_fd)
                {
                    int new_socket = accept(server_fd, NULL, NULL);
                    if (new_socket < 0)
                        continue;

                    fds[nfds].fd = new_socket;
                    fds[nfds].events = POLLIN;

                    clients[nfds].fd = new_socket;
                    clients[nfds].is_registered = 0;
                    nfds++;

                    printf("Client %d ket noi.\n", new_socket);
                    char *msg = "Vui long nhap ten theo cu phap: client_id: <ten_cua_ban>\n";
                    send(new_socket, msg, strlen(msg), 0);
                }
                // Có dữ liệu từ client
                else
                {
                    char buffer[1024] = {0};
                    int valread = recv(fds[i].fd, buffer, sizeof(buffer) - 1, 0);

                    if (valread <= 0)
                    {
                        printf("Client %d ngat ket noi.\n", fds[i].fd);
                        close(fds[i].fd);
                        fds[i] = fds[nfds - 1];
                        clients[i] = clients[nfds - 1];
                        nfds--;
                        i--;
                    }
                    else
                    {
                        buffer[strcspn(buffer, "\r\n")] = 0;
                        if (strlen(buffer) == 0)
                            continue;

                        if (!clients[i].is_registered)
                        {
                            // Kiểm tra cú pháp đăng ký
                            if (strncmp(buffer, "client_id: ", 11) == 0)
                            {
                                strcpy(clients[i].name, buffer + 11);
                                clients[i].is_registered = 1;
                                char ack[100];
                                sprintf(ack, "Dang ky thanh cong. Xin chao %s!\n", clients[i].name);
                                send(fds[i].fd, ack, strlen(ack), 0);
                                printf("Client %d dang ky ten: %s\n", fds[i].fd, clients[i].name);
                            }
                            else
                            {
                                char *err = "Sai cu phap. Vui long nhap lai (VD: client_id: hieu)\n";
                                send(fds[i].fd, err, strlen(err), 0);
                            }
                        }
                        else
                        {
                            // Broadcast tin nhắn tới các client khác
                            char time_str[50];
                            get_current_time(time_str, sizeof(time_str));

                            char send_buf[2048];
                            sprintf(send_buf, "%s %s: %s\n", time_str, clients[i].name, buffer);

                            // Gửi tới các client khác đã đăng ký
                            for (int j = 1; j < nfds; j++)
                            {
                                if (j != i && clients[j].is_registered)
                                {
                                    send(fds[j].fd, send_buf, strlen(send_buf), 0);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}