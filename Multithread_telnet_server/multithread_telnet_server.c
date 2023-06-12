#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>

void *client_thread(void *);

int users[64];
int num_users = 0;

void process_request(int client, char *buf);
void remove_user(int client);

int main() 
{
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == -1)
    {
        perror("socket() failed");
        return 1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(9000);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr))) 
    {
        perror("bind() failed");
        return 1;
    }

    if (listen(listener, 5)) 
    {
        perror("listen() failed");
        return 1;
    }

    while (1)
    {
        int client = accept(listener, NULL, NULL);
        if (client == -1)
        {
            perror("accept() failed");
            continue;
        }
        printf("New client connected: %d\n", client);

        pthread_t thread_id;
        pthread_create(&thread_id, NULL, client_thread, &client);
        pthread_detach(thread_id);
    }
    
    close(listener);    

    return 0;
}

void *client_thread(void *param)
{
    int client = *(int *)param;
    char buf[256];

    while (1)
    {
        int ret = recv(client, buf, sizeof(buf), 0);
        if (ret <= 0){
            printf("Client %d disconnected.\n", client);
            close(client);
            remove_user(client);
            break;
        }
        else
        {
            buf[ret] = 0;
            printf("Received from %d: %s\n", client, buf);

            // Process request
            process_request(client, buf);
        }
    }

    close(client);
}

void process_request(int client, char *buf)
{
    // Kiem tra trang thai dang nhap cua client
    int i = 0;
    for (; i < num_users; i++)
        if (users[i] == client) 
            break;

    if (i == num_users)
    {
        // Chua dang nhap
        char user[32], pass[32], tmp[66], line[66];
        int ret = sscanf(buf, "%s%s%s", user, pass, tmp);
        if (ret == 2)
        {
            sprintf(tmp, "%s %s\n", user, pass);
            FILE *f = fopen("users.txt", "r");
            int found = 0;
            while (fgets(line, sizeof(line), f) != NULL)
            {
                if (strcmp(line, tmp) == 0)
                {
                    found = 1;
                    break;
                }
            }
            fclose(f);

            if (found)
            {
                char *msg = "Dang nhap thanh cong. Hay nhap lenh de thuc hien.\n";
                send(client, msg, strlen(msg), 0);

                users[num_users] = client;
                num_users++;
            }
            else
            {
                char *msg = "Nhap sai tai khoan. Hay nhap lai.\n";
                send(client, msg, strlen(msg), 0);
            }
        }
        else
        {
            char *msg = "Nhap sai cu phap. Hay nhap lai.\n";
            send(client, msg, strlen(msg), 0);
        }
    }
    else
    {
        // Da dang nhap
        char tmp[256];

        // Xoa dau xuong dong neu co
        if (buf[strlen(buf) - 1] == '\n')
            buf[strlen(buf) - 1] = '\0';

        sprintf(tmp, "%s > out.txt", buf);
        int ret = system(tmp);

        if (ret == 0)
        {
            FILE *f = fopen("out.txt", "rb");
            while (!feof(f))
            {
                ret = fread(tmp, 1, sizeof(tmp), f);
                if (ret <= 0)
                    break;
                send(client, tmp, ret, 0);
            }
            fclose(f);
        }
        else
        {
            char *msg = "Lenh khong thuc hien duoc.\n";
            send(client, msg, strlen(msg), 0);
        }
    }
}

void remove_user(int client)
{
    int i = 0;
    for (; i < num_users; i++)
        if (users[i] == client)
            break;
    if (i < num_users)
    {
        if (i < num_users - 1)
            users[i] = users[num_users - 1];
        num_users--;
    }
}
