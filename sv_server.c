#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>

typedef struct
{
    char mssv[20];
    char ho_ten[50];
    char ngay_sinh[20];
    float diem_tb;
} SinhVien;

void get_current_time(char *buffer, int size)
{
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", tm_info);
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Cú pháp: %s <cổng> <tên file log>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    char *log_filename = argv[2];

    // Tạo Socket & Cấu hình
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0)
    {
        perror("Lỗi tạo socket");
        return 1;
    }

    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // Bind & Listen
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Lỗi bind");
        return 1;
    }

    if (listen(server_socket, 5) < 0)
    {
        perror("Lỗi listen");
        return 1;
    }

    printf("Server đang lắng nghe ở cổng %d...\n", port);
    printf("Dữ liệu log sẽ được ghi vào file: %s\n", log_filename);

    // Vòng lặp chờ và xử lý Client
    while (1)
    {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket < 0)
        {
            perror("Lỗi accept");
            continue;
        }

        // Lấy IP của client
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);

        // Lấy thời gian hiện tại
        char time_str[30];
        get_current_time(time_str, sizeof(time_str));

        // Nhận dữ liệu struct từ Client
        SinhVien sv;
        int bytes_received = recv(client_socket, &sv, sizeof(SinhVien), 0);

        if (bytes_received == sizeof(SinhVien))
        {
            // -- IN RA MÀN HÌNH --
            printf("\n[+] Nhận dữ liệu từ %s lúc %s\n", client_ip, time_str);
            printf("MSSV: %s | Họ tên: %s | Ngày sinh: %s | Điểm TB: %.2f\n",
                   sv.mssv, sv.ho_ten, sv.ngay_sinh, sv.diem_tb);

            // -- GHI VÀO FILE LOG --
            FILE *flog = fopen(log_filename, "a");
            if (flog == NULL)
            {
                perror("Lỗi mở file log");
            }
            else
            {
                fprintf(flog, "%s %s %s %s %s %.2f\n",
                        client_ip, time_str, sv.mssv, sv.ho_ten, sv.ngay_sinh, sv.diem_tb);
                fclose(flog);
            }
        }
        else
        {
            printf("\n[-] Nhận dữ liệu lỗi hoặc client ngắt kết nối không hợp lệ.\n");
        }

        // Đóng kết nối
        close(client_socket);
    }

    close(server_socket);
    return 0;
}