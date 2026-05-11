#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT 8888
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

// Hàm xử lý tín hiệu SIGCHLD để tránh tạo ra zombie process
void sigchld_handler(int s)
{
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
}

// Hàm xử lý từng client
void handle_client(int client_fd)
{
    char buffer[BUFFER_SIZE];
    int n;

    // Yêu cầu nhập user và pass
    char prompt[] = "Please enter user and pass (format: <user> <pass>): ";
    send(client_fd, prompt, strlen(prompt), 0);

    n = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (n <= 0)
    {
        close(client_fd);
        return;
    }
    buffer[n] = '\0';

    buffer[strcspn(buffer, "\r\n")] = 0;

    // So sánh với file cơ sở dữ liệu
    FILE *db_file = fopen("users.txt", "r");
    int authenticated = 0;

    if (db_file != NULL)
    {
        char line[256];
        while (fgets(line, sizeof(line), db_file))
        {
            line[strcspn(line, "\r\n")] = 0; // Xóa newline của từng dòng trong file
            if (strcmp(line, buffer) == 0)
            {
                authenticated = 1;
                break;
            }
        }
        fclose(db_file);
    }
    else
    {
        printf("Khong the mo file users.txt\n");
    }

    // Xử lý kết quả xác thực
    if (!authenticated)
    {
        char err_msg[] = "Dang nhap that bai. Sai user hoac pass.\n";
        send(client_fd, err_msg, strlen(err_msg), 0);
        close(client_fd);
        return;
    }

    char success_msg[] = "Dang nhap thanh cong!\n";
    send(client_fd, success_msg, strlen(success_msg), 0);

    char cmd[BUFFER_SIZE];
    char sys_cmd[BUFFER_SIZE + 100];
    char out_filename[50];

    // Sử dụng PID của tiến trình con để tạo tên file riêng biệt,
    // tránh đụng độ khi có nhiều client cùng gửi lệnh
    snprintf(out_filename, sizeof(out_filename), "out_%d.txt", getpid());

    while (1)
    {
        char shell_prompt[] = "telnet> ";
        send(client_fd, shell_prompt, strlen(shell_prompt), 0);

        n = recv(client_fd, cmd, sizeof(cmd) - 1, 0);
        if (n <= 0)
        {
            break;
        }
        cmd[n] = '\0';
        cmd[strcspn(cmd, "\r\n")] = 0;

        if (strlen(cmd) == 0)
            continue;
        if (strcmp(cmd, "exit") == 0)
            break;

        sprintf(sys_cmd, "%s > %s 2>&1", cmd, out_filename);
        system(sys_cmd);

        // Đọc kết quả từ file và gửi lại cho client
        FILE *out_file = fopen(out_filename, "r");
        if (out_file)
        {
            int bytes_read;
            while ((bytes_read = fread(buffer, 1, sizeof(buffer), out_file)) > 0)
            {
                send(client_fd, buffer, bytes_read, 0);
            }
            fclose(out_file);
        }
        else
        {
            char file_err[] = "Loi doc file output.\n";
            send(client_fd, file_err, strlen(file_err), 0);
        }
    }

    // Dọn dẹp: Xóa file tạm và đóng kết nối
    remove(out_filename);
    close(client_fd);
    printf("Client disconnected. Xoa file tam %s\n", out_filename);
}

int main()
{
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // Chống Zombie process bằng cách bắt tín hiệu SIGCHLD
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(1);
    }

    // Tạo socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Cấu hình địa chỉ server
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind socket
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Lắng nghe kết nối
    if (listen(server_fd, MAX_CLIENTS) < 0)
    {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Telnet Server dang lang nghe tren cong %d...\n", PORT);

    // Vòng lặp chính của server
    while (1)
    {
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0)
        {
            perror("Accept failed");
            continue;
        }

        printf("Co ket noi moi tu %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        // TẠO TIẾN TRÌNH CON
        pid_t pid = fork();

        if (pid < 0)
        {
            perror("Fork failed");
            close(client_fd);
        }
        else if (pid == 0)
        {
            // Đây là tiến trình con
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