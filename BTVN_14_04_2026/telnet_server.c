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

#define PORT 9001
#define MAX_CLIENTS 100

typedef struct
{
    int fd;
    int is_logged_in;
} ClientState;

ClientState clients[MAX_CLIENTS];

// Hàm kiểm tra tài khoản
int check_login(const char *credentials)
{
    FILE *fp = fopen("database.txt", "r");
    if (fp == NULL)
    {
        perror("Khong tim thay file database.txt");
        return 0;
    }
    char line[100];
    while (fgets(line, sizeof(line), fp))
    {
        line[strcspn(line, "\r\n")] = 0;
        if (strcmp(line, credentials) == 0)
        {
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}

int main()
{
    int server_fd;
    struct sockaddr_in address;
    struct pollfd fds[MAX_CLIENTS];
    int nfds = 1;

    for (int i = 0; i < MAX_CLIENTS; i++)
        clients[i].fd = -1;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
        exit(EXIT_FAILURE);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
        exit(EXIT_FAILURE);
    if (listen(server_fd, 10) < 0)
        exit(EXIT_FAILURE);

    printf("[TELNET SERVER] Dang lang nghe tren cong %d...\n", PORT);

    memset(fds, 0, sizeof(fds));
    fds[0].fd = server_fd;
    fds[0].events = POLLIN;

    while (1)
    {
        if (poll(fds, nfds, -1) < 0)
            break;

        for (int i = 0; i < nfds; i++)
        {
            if (fds[i].revents & POLLIN)
            {
                if (fds[i].fd == server_fd)
                {
                    int new_socket = accept(server_fd, NULL, NULL);
                    if (new_socket < 0)
                        continue;

                    fds[nfds].fd = new_socket;
                    fds[nfds].events = POLLIN;
                    clients[nfds].fd = new_socket;
                    clients[nfds].is_logged_in = 0;
                    nfds++;

                    printf("Client %d ket noi.\n", new_socket);
                    send(new_socket, "Vui long dang nhap (user pass): ", 32, 0);
                }
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

                        if (!clients[i].is_logged_in)
                        {
                            if (check_login(buffer))
                            {
                                clients[i].is_logged_in = 1;
                                send(fds[i].fd, "Dang nhap thanh cong! Nhap lenh:\n", 33, 0);
                            }
                            else
                            {
                                send(fds[i].fd, "Tai khoan hoac mat khau sai. Thu lai:\n", 39, 0);
                            }
                        }
                        else
                        {
                            // Thực thi lệnh
                            char command[1100];
                            // Tạo file output riêng cho mỗi client để tránh xung đột khi nhiều client cùng thực thi lệnh
                            char out_file[50];
                            sprintf(out_file, "out_%d.txt", fds[i].fd);
                            sprintf(command, "%s > %s 2>&1", buffer, out_file);

                            system(command);

                            // Đọc kết quả từ file và gửi lại cho client
                            FILE *fp = fopen(out_file, "r");
                            if (fp != NULL)
                            {
                                char file_buf[2048];
                                size_t bytes_read;
                                while ((bytes_read = fread(file_buf, 1, sizeof(file_buf), fp)) > 0)
                                {
                                    send(fds[i].fd, file_buf, bytes_read, 0);
                                }
                                fclose(fp);
                                remove(out_file);
                            }
                            send(fds[i].fd, "\n>> ", 4, 0);
                        }
                    }
                }
            }
        }
    }
    return 0;
}