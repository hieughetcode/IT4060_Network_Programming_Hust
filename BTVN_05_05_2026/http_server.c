#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#define PORT 8080
#define WORKER_PROCESSES 5

// Hàm xử lý vòng lặp accept của từng tiến trình con
void worker_process(int listener)
{
    char buf[256];
    char *msg = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<html><body><h1>Xin chao cac ban</h1></body></html>";

    while (1)
    {
        // Các worker cùng cạnh tranh nhau gọi accept trên chung 1 listener
        int client = accept(listener, NULL, NULL);
        if (client < 0)
        {
            continue;
        }

        // In ra PID của worker đang xử lý để bạn dễ quan sát
        printf("[Worker PID: %d] New client connected: %d\n", getpid(), client);

        // Nhận dữ liệu từ client (Request)
        int ret = recv(client, buf, sizeof(buf) - 1, 0);
        if (ret > 0)
        {
            buf[ret] = 0;
            puts(buf);
        }

        send(client, msg, strlen(msg), 0);

        close(client);
    }
}

int main()
{
    int listener;
    struct sockaddr_in server_addr;

    // Khởi tạo server socket
    listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener == -1)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Tùy chọn để tránh lỗi "Address already in use" khi restart server
    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(listener, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(listener, 10) < 0)
    {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("HTTP Server (Preforking) dang lang nghe o cong %d\n", PORT);
    printf("Dang tao %d worker processes...\n", WORKER_PROCESSES);

    // Kỹ thuật Preforking: Tạo sẵn các tiến trình con
    for (int i = 0; i < WORKER_PROCESSES; i++)
    {
        pid_t pid = fork();

        if (pid == 0)
        {
            worker_process(listener);
            exit(0);
        }
        else if (pid < 0)
        {
            perror("Fork failed");
        }
    }

    // Tiến trình cha không làm nhiệm vụ accept, nó chỉ đứng đợi các con
    // Nếu một tiến trình con nào đó bị chết, có thể thêm logic để spawn lại tại đây
    while (wait(NULL) > 0)
        ;

    close(listener);
    return 0;
}