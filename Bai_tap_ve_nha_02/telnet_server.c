#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define PORT 9001
#define MAX_FD 1024
#define MAX_USERS 100

/*-----------------------------*/
// Le Huu Hieu - 20225315
/*-----------------------------*/

// Cau truc thong tin tai khoan
typedef struct
{
    char user[50];
    char pass[50];
} UserAccount;

UserAccount db[MAX_USERS];
int db_size = 0;

// Trang thai dang nhap cua tung client
int logged_in[MAX_FD];

// Đọc database từ file
void load_database(const char *filename)
{
    FILE *f = fopen(filename, "r");
    if (!f)
    {
        // File mau neu chua co
        f = fopen(filename, "w");
        fprintf(f, "admin admin\nguest nopass\n");
        fclose(f);
        f = fopen(filename, "r");
    }

    while (fscanf(f, "%s %s", db[db_size].user, db[db_size].pass) == 2)
    {
        db_size++;
        if (db_size >= MAX_USERS)
            break;
    }
    fclose(f);
}

// Ham kiem tra dang nhap
int check_login(const char *u, const char *p)
{
    for (int i = 0; i < db_size; i++)
    {
        if (strcmp(db[i].user, u) == 0 && strcmp(db[i].pass, p) == 0)
        {
            return 1;
        }
    }
    return 0;
}

int main()
{
    load_database("database.txt");
    memset(logged_in, 0, sizeof(logged_in)); // Khởi tạo mảng đăng nhập bằng 0

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

    printf("Telnet Server dang chay o port %d...\n", PORT);

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
                    int newfd = accept(listener, NULL, NULL);
                    FD_SET(newfd, &master);
                    if (newfd > fd_max)
                        fd_max = newfd;

                    const char *prompt = "Nhap tai khoan va mat khau (user pass):\n";
                    send(newfd, prompt, strlen(prompt), 0);
                }
                else
                {
                    char buffer[1024];
                    memset(buffer, 0, sizeof(buffer));
                    int nbytes = recv(i, buffer, sizeof(buffer) - 1, 0);

                    if (nbytes <= 0)
                    {
                        close(i);
                        FD_CLR(i, &master);
                        logged_in[i] = 0; // Reset trang thai khi ngat ket noi
                    }
                    else
                    {
                        buffer[strcspn(buffer, "\r\n")] = 0;
                        if (strlen(buffer) == 0)
                            continue;

                        if (logged_in[i] == 0)
                        {
                            // Xu ly cho dang nhap
                            char user[50], pass[50];
                            if (sscanf(buffer, "%s %s", user, pass) == 2)
                            {
                                if (check_login(user, pass))
                                {
                                    logged_in[i] = 1;
                                    const char *ok_msg = "Dang nhap thanh cong! Nhap lenh:\n";
                                    send(i, ok_msg, strlen(ok_msg), 0);
                                }
                                else
                                {
                                    const char *err_msg = "Tai khoan hoac mat khau khong dung!\n";
                                    send(i, err_msg, strlen(err_msg), 0);
                                }
                            }
                            else
                            {
                                const char *err_fmt = "Vui long nhap du user va pass cach nhau khoang trang.\n";
                                send(i, err_fmt, strlen(err_fmt), 0);
                            }
                        }
                        else
                        {
                            // Chay lenh khi da dang nhap
                            char out_file[32];
                            sprintf(out_file, "out_%d.txt", i); // File riêng cho từng client

                            char cmd[1100];
                            sprintf(cmd, "%s > %s 2>&1", buffer, out_file);
                            system(cmd);

                            // Doc ket qua va gui cho client
                            FILE *f_res = fopen(out_file, "r");
                            if (f_res)
                            {
                                char file_buf[1024];
                                int bytes_read;
                                int has_output = 0;

                                while ((bytes_read = fread(file_buf, 1, sizeof(file_buf), f_res)) > 0)
                                {
                                    send(i, file_buf, bytes_read, 0);
                                    has_output = 1;
                                }
                                fclose(f_res);

                                if (!has_output)
                                {
                                    const char *empty_msg = "[Lenh thuc thi khong co output]\n";
                                    send(i, empty_msg, strlen(empty_msg), 0);
                                }
                            }

                            // Xóa file temp
                            // remove(out_file);
                        }
                    }
                }
            }
        }
    }
    close(listener);
    return 0;
}