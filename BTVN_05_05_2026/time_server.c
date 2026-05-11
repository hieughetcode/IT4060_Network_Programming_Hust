#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>

#define PORT 9000
#define MAX_CLIENTS 10
#define BUFFER_SIZE 256

// Chống zombie process bằng cách bắt tín hiệu SIGCHLD
void sigchld_handler(int s)
{
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
}

// Hàm phục vụ từng client riêng biệt
void handle_client(int client_fd)
{
    char buffer[BUFFER_SIZE];
    int n;

    char welcome_msg[] = "Ket noi thanh cong den Time Server!\nGo lenh: GET_TIME [format]\n";
    send(client_fd, welcome_msg, strlen(welcome_msg), 0);

    while (1)
    {
        // Nhận dữ liệu từ client
        n = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0)
        {
            break;
        }

        buffer[n] = '\0';
        buffer[strcspn(buffer, "\r\n")] = 0;

        if (strlen(buffer) == 0)
            continue;
        if (strcmp(buffer, "exit") == 0)
            break; // Lệnh thoát vòng lặp

        char cmd[50] = {0};
        char format[50] = {0};

        int parsed_args = sscanf(buffer, "%s %s", cmd, format);

        // KIỂM TRA TÍNH ĐÚNG ĐẮN CỦA LỆNH
        if (strcmp(cmd, "GET_TIME") != 0)
        {
            char err_cmd[] = "Loi: Lenh khong hop le. Hay su dung GET_TIME [format]\n";
            send(client_fd, err_cmd, strlen(err_cmd), 0);
            continue;
        }

        if (parsed_args < 2)
        {
            char err_fmt[] = "Loi: Thieu format. Hay su dung GET_TIME [format]\n";
            send(client_fd, err_fmt, strlen(err_fmt), 0);
            continue;
        }

        // LẤY VÀ ĐỊNH DẠNG THỜI GIAN
        time_t t = time(NULL);
        struct tm tm = *localtime(&t);
        char time_str[100];
        int valid_format = 1;

        // So sánh format và sử dụng hàm strftime để chuyển đổi thời gian
        if (strcmp(format, "dd/mm/yyyy") == 0)
        {
            strftime(time_str, sizeof(time_str), "%d/%m/%Y\n", &tm);
        }
        else if (strcmp(format, "dd/mm/yy") == 0)
        {
            strftime(time_str, sizeof(time_str), "%d/%m/%y\n", &tm);
        }
        else if (strcmp(format, "mm/dd/yyyy") == 0)
        {
            strftime(time_str, sizeof(time_str), "%m/%d/%Y\n", &tm);
        }
        else if (strcmp(format, "mm/dd/yy") == 0)
        {
            strftime(time_str, sizeof(time_str), "%m/%d/%y\n", &tm);
        }
        else
        {
            valid_format = 0;
            char err_unsupported[] = "Loi: Format khong duoc ho tro.\nCac format hop le: dd/mm/yyyy, dd/mm/yy, mm/dd/yyyy, mm/dd/yy\n";
            send(client_fd, err_unsupported, strlen(err_unsupported), 0);
        }

        // Nếu format hợp lệ, gửi kết quả trả về cho client
        if (valid_format)
        {
            send(client_fd, time_str, strlen(time_str), 0);
        }
    }

    close(client_fd);
    printf("Mot client da ngat ket noi.\n");
}

int main()
{
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // Bắt tín hiệu SIGCHLD
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("sigaction error");
        exit(1);
    }

    // Tạo socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Tránh lỗi "Address already in use" khi chạy lại server nhiều lần
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, MAX_CLIENTS) < 0)
    {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Time Server dang lang nghe tren cong %d...\n", PORT);

    while (1)
    {
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0)
            continue;

        printf("Co client moi ket noi tu %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        // TẠO TIẾN TRÌNH CON XỬ LÝ CLIENT
        pid_t pid = fork();

        if (pid < 0)
        {
            perror("Fork failed");
            close(client_fd);
        }
        else if (pid == 0)
        {
            close(server_fd);
            handle_client(client_fd);
            exit(0);
        }
        else
        {
            close(client_fd);
        }
    }

    close(server_fd);
    return 0;
}