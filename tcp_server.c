#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        printf("Cú pháp: %s <cổng> <tệp tin chứa câu chào> <tệp tin lưu nội dung>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    char *greeting_file = argv[2];
    char *output_file = argv[3];

    // Tạo Socket cho Server
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0)
    {
        perror("Lỗi tạo socket");
        return 1;
    }

    // Thiết lập tùy chọn để có thể sử dụng lại port ngay lập tức sau khi tắt server
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Khai báo và cấu hình địa chỉ Server
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // Gắn socket với địa chỉ và cổng (Bind)
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Lỗi bind (cổng có thể đang được sử dụng)");
        close(server_socket);
        return 1;
    }

    // Chuyển socket sang trạng thái lắng nghe (Listen)
    if (listen(server_socket, 5) < 0)
    {
        perror("Lỗi listen");
        close(server_socket);
        return 1;
    }

    printf("Server đang lắng nghe ở cổng %d...\n", port);

    // Vòng lặp vô hạn để liên tục phục vụ các client kết nối đến
    while (1)
    {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        // Chấp nhận kết nối từ Client (Accept)
        // Hàm này sẽ block (dừng chương trình) cho đến khi có client gọi tới
        int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket < 0)
        {
            perror("Lỗi accept");
            continue;
        }

        // Lấy IP của client để in ra màn hình
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        printf("\n[+] Có client kết nối từ: %s:%d\n", client_ip, ntohs(client_addr.sin_port));

        // Mở tệp chứa câu chào và gửi cho client
        FILE *fgreet = fopen(greeting_file, "r");
        if (fgreet == NULL)
        {
            perror("Không thể mở tệp chứa câu chào");
            char *error_msg = "Server loi: Khong tim thay file chao.\n";
            send(client_socket, error_msg, strlen(error_msg), 0);
        }
        else
        {
            char greet_buf[BUFFER_SIZE];
            // Đọc nội dung tệp và gửi đi
            size_t bytes_read = fread(greet_buf, 1, sizeof(greet_buf), fgreet);
            send(client_socket, greet_buf, bytes_read, 0);
            fclose(fgreet);
        }

        // Mở tệp để lưu nội dung client gửi
        FILE *fout = fopen(output_file, "a");
        if (fout == NULL)
        {
            perror("Không thể mở tệp để ghi dữ liệu");
        }
        else
        {
            char buffer[BUFFER_SIZE];
            int bytes_received;

            // Nhận dữ liệu liên tục cho đến khi client ngắt kết nối
            while ((bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0)) > 0)
            {
                // Ghi dữ liệu nhận được vào tệp tin
                fwrite(buffer, 1, bytes_received, fout);
                fflush(fout);

                buffer[bytes_received] = '\0';
                printf("Nhận được: %s\n", buffer);
            }

            fclose(fout);
        }

        // Đóng kết nối với client hiện tại và quay lại đợi client mới
        close(client_socket);
        printf("[-] Đã đóng kết nối với client %s.\n", client_ip);
    }

    // Đóng server socket
    close(server_socket);
    return 0;
}