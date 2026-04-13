#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/select.h>

#define PORT 9000
#define MAX_FD 1024

/*-----------------------------*/
// Le Huu Hieu - 20225315
/*-----------------------------*/

// Cấu trúc lưu thông tin client
typedef struct
{
    char id[50];
    // Trạng thái đăng nhập
    int is_logged_in;
} Client;

Client clients[MAX_FD];

// Hàm lấy thời gian hiện tại
void get_current_time(char *buffer, int size)
{
    time_t now = time(NULL);
    struct tm *ltm = localtime(&now);
    strftime(buffer, size, "%Y/%m/%d %I:%M:%S%p", ltm);
}

int main()
{
    // Mảng client
    for (int i = 0; i < MAX_FD; i++)
    {
        clients[i].is_logged_in = 0;
        memset(clients[i].id, 0, sizeof(clients[i].id));
    }

    int listener = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    bind(listener, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(listener, 10);

    fd_set master, read_fds;
    FD_ZERO(&master);
    FD_SET(listener, &master);
    int fd_max = listener;

    printf("Chat Server dang chay o port %d...\n", PORT);

    while (1)
    {
        read_fds = master;
        if (select(fd_max + 1, &read_fds, NULL, NULL, NULL) == -1)
        {
            perror("select");
            break;
        }

        for (int i = 0; i <= fd_max; i++)
        {
            if (FD_ISSET(i, &read_fds))
            {
                if (i == listener)
                {
                    // Có kết nối mới
                    int newfd = accept(listener, NULL, NULL);
                    FD_SET(newfd, &master);
                    if (newfd > fd_max)
                        fd_max = newfd;

                    const char *prompt = "Vui long nhap theo cu phap: client_id: client_name\n";
                    send(newfd, prompt, strlen(prompt), 0);
                }
                else
                {
                    // Dữ liêu có đươc từ client
                    char buffer[1024];
                    memset(buffer, 0, sizeof(buffer));
                    int nbytes = recv(i, buffer, sizeof(buffer) - 1, 0);

                    if (nbytes <= 0)
                    {
                        // Client ngắt kết nối
                        close(i);
                        FD_CLR(i, &master);
                        clients[i].is_logged_in = 0; // Reset trạng thái
                    }
                    else
                    {
                        buffer[strcspn(buffer, "\r\n")] = 0;
                        if (strlen(buffer) == 0)
                            continue;

                        if (clients[i].is_logged_in == 0)
                        {
                            // Xử lý cho việc đăng nhập
                            char client_id[50], client_name[50];
                            if (sscanf(buffer, "%[^:]: %s", client_id, client_name) == 2)
                            {
                                strcpy(clients[i].id, client_id);
                                clients[i].is_logged_in = 1;

                                const char *ok_msg = "Dang nhap thanh cong. Ban co the bat dau chat!\n";
                                send(i, ok_msg, strlen(ok_msg), 0);
                            }
                            else
                            {
                                const char *err_msg = "Sai cu phap! Yeu cau: client_id: client_name\n";
                                send(i, err_msg, strlen(err_msg), 0);
                            }
                        }
                        else
                        {
                            // Đã đăng nhập thì xử lý tin nhắn chat
                            char time_str[80];
                            get_current_time(time_str, sizeof(time_str));

                            char msg_to_send[2048];
                            sprintf(msg_to_send, "%s %s: %s\n", time_str, clients[i].id, buffer);

                            for (int j = 0; j <= fd_max; j++)
                            {
                                // Gửi cho mọi người trừ chính mình và listener (phải login rồi)
                                if (FD_ISSET(j, &master) && j != listener && j != i && clients[j].is_logged_in == 1)
                                {
                                    send(j, msg_to_send, strlen(msg_to_send), 0);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    close(listener);
    return 0;
}