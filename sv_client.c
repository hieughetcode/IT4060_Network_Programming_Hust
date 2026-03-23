#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

typedef struct
{
    char mssv[20];
    char ho_ten[50];
    char ngay_sinh[20];
    float diem_tb;
} SinhVien;

void remove_newline(char *str)
{
    int len = strlen(str);
    if (len > 0 && str[len - 1] == '\n')
    {
        str[len - 1] = '\0';
    }
}

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

    // Khai báo biến cấu trúc và nhập dữ liệu
    SinhVien sv;
    memset(&sv, 0, sizeof(SinhVien));
    char temp_buffer[20];

    printf("--- NHẬP THÔNG TIN SINH VIÊN ---\n");

    printf("MSSV: ");
    fgets(sv.mssv, sizeof(sv.mssv), stdin);
    remove_newline(sv.mssv);

    printf("Họ và tên: ");
    fgets(sv.ho_ten, sizeof(sv.ho_ten), stdin);
    remove_newline(sv.ho_ten);

    printf("Ngày sinh (dd/mm/yyyy): ");
    fgets(sv.ngay_sinh, sizeof(sv.ngay_sinh), stdin);
    remove_newline(sv.ngay_sinh);

    printf("Điểm trung bình: ");
    fgets(temp_buffer, sizeof(temp_buffer), stdin);
    sv.diem_tb = atof(temp_buffer);

    // Gửi toàn bộ cấu trúc sang Server
    if (send(client_socket, &sv, sizeof(SinhVien), 0) < 0)
    {
        perror("Gửi dữ liệu thất bại");
    }
    else
    {
        printf("\n=> Đã đóng gói và gửi thông tin sinh viên thành công!\n");
    }

    // Đóng kết nối
    close(client_socket);

    return 0;
}