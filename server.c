#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <ctype.h>

#define PORT 8080
#define MAX_CLIENTS 30
#define BUFFER_SIZE 1024

// Hàm tạo email chuẩn ĐHBK
void format_hust_email(char *input, char *output)
{
    char temp[BUFFER_SIZE];
    strcpy(temp, input);

    char *words[20];
    int count = 0;

    // Tách chuỗi dựa vào khoảng trắng
    char *token = strtok(temp, " ");
    while (token != NULL && count < 20)
    {
        words[count++] = token;
        token = strtok(NULL, " ");
    }

    if (count >= 3)
    {
        char prefix[100] = {0};

        // 1. Lấy Tên (nằm ở vị trí áp chót)
        strcpy(prefix, words[count - 2]);
        strcat(prefix, ".");

        for (int j = 0; j < count - 2; j++)
        {
            int len = strlen(prefix);
            prefix[len] = words[j][0]; // Lấy ký tự đầu tiên
            prefix[len + 1] = '\0';
        }

        char *mssv = words[count - 1];
        int mssv_len = strlen(mssv);
        if (mssv_len > 6)
        {
            strcat(prefix, mssv + (mssv_len - 6));
        }
        else
        {
            strcat(prefix, mssv); // Đề phòng MSSV nhập vào ít hơn 6 số
        }

        sprintf(output, "Email cua ban la: %s@sis.hust.edu.vn\n", prefix);
    }
    else
    {
        sprintf(output, "Loi: Nhap sai dinh dang. Vui long nhap 'Ho Ten MSSV'\n");
    }
}

int main()
{
    int master_socket, addrlen, new_socket, client_socket[MAX_CLIENTS], max_clients = MAX_CLIENTS, activity, i, valread, sd;
    int max_sd;
    struct sockaddr_in address;
    char buffer[BUFFER_SIZE];
    fd_set readfds;

    char *welcome_msg = "Vui long nhap Ho ten va MSSV: \n";

    for (i = 0; i < max_clients; i++)
    {
        client_socket[i] = 0;
    }

    if ((master_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(master_socket, 3) < 0)
    {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server dang lang nghe tren port %d...\n", PORT);
    addrlen = sizeof(address);

    while (1)
    {
        FD_ZERO(&readfds);
        FD_SET(master_socket, &readfds);
        max_sd = master_socket;

        for (i = 0; i < max_clients; i++)
        {
            sd = client_socket[i];
            if (sd > 0)
                FD_SET(sd, &readfds);
            if (sd > max_sd)
                max_sd = sd;
        }

        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if (activity < 0)
        {
            printf("Loi select\n");
        }

        if (FD_ISSET(master_socket, &readfds))
        {
            if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
            {
                perror("Accept failed");
                exit(EXIT_FAILURE);
            }
            printf("Client moi ket noi, socket fd: %d, IP: %s, Port: %d\n", new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));

            send(new_socket, welcome_msg, strlen(welcome_msg), 0);

            for (i = 0; i < max_clients; i++)
            {
                if (client_socket[i] == 0)
                {
                    client_socket[i] = new_socket;
                    break;
                }
            }
        }

        // Xử lý dữ liệu từ các client cũ
        for (i = 0; i < max_clients; i++)
        {
            sd = client_socket[i];
            if (FD_ISSET(sd, &readfds))
            {
                if ((valread = read(sd, buffer, BUFFER_SIZE)) == 0)
                {
                    getpeername(sd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
                    printf("Client ngat ket noi, IP %s, port %d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
                    close(sd);
                    client_socket[i] = 0;
                }
                else
                {
                    buffer[valread] = '\0';
                    buffer[strcspn(buffer, "\r\n")] = 0;

                    char email[BUFFER_SIZE + 50];
                    format_hust_email(buffer, email);

                    send(sd, email, strlen(email), 0);

                    close(sd);
                    client_socket[i] = 0;
                }
            }
        }
    }
    return 0;
}