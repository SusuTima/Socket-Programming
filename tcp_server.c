#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h> 

#define PORT 8080
#define MAX_CLIENTS 5

#define BACKLOG 5

pthread_t clients[BACKLOG];
pthread_t tid;

void atbash_cipher(char *message) {
    for (int i = 0; message[i] != '\0'; i++) {
        if (isalpha(message[i])) { // 如果是字母
            if (islower(message[i])) {
                // 小寫字母範圍：a->z
                message[i] = 'z' - (message[i] - 'a');
            } else if (isupper(message[i])) {
                // 大寫字母範圍：A->Z
                message[i] = 'Z' - (message[i] - 'A');
            }
        }
        // 標點符號等非字母內容不變
    }
}

char *convert(char *src) {
    char *iter = src;
    char *result = malloc(sizeof(src));
    char *it = result;
    if (iter == NULL) return iter;

    while (*iter) {
        *it++ = toupper(*iter++);
    }
    return result;
}

struct client_info{
    int sockfd;
    struct sockaddr_in clientAddr;
};

#define BUFFER_SIZE 20  // Buffer 限制為 20

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    char buffer[BUFFER_SIZE];              // 每次接收的分塊最大為 20 bytes
    char full_message[1024] = {0}; // 用來累積完整的訊息
    size_t full_len = 0;          // 累積的訊息長度
    const char *welcome_message = "Hello from server!\n";

    // 創建 Socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // 設定 Socket 選項
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // 開始監聽
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }

        printf("New connection accepted...\n");

        if (fork() == 0) {
            close(server_fd); // 子進程不需要 Server 的 socket

            // 發送歡迎訊息
            send(new_socket, welcome_message, strlen(welcome_message), 0);
            printf("Welcome message sent.\n");

            while (1) {
                memset(buffer, 0, sizeof(buffer));
                //接收client訊息 
                int received_bytes = recv(new_socket, buffer, sizeof(buffer) - 1, 0);

                if (received_bytes <= 0) break; // 客戶端斷開或接收失敗

                buffer[received_bytes] = '\0'; // 確保字串結尾
                printf("Received chunk: %s\n", buffer);

                // 檢查累積是否超出緩衝區大小
                if (full_len + received_bytes >= sizeof(full_message)) {
                    fprintf(stderr, "Message too large to handle!\n");
                    break;
                }

                // 累積到完整訊息
                strncat(full_message, buffer, received_bytes);
                full_len += received_bytes;

                // 檢測是否完成（檢測 "\r\n" 或其他結尾標記）
                if (strstr(full_message, "\r\n") != NULL) {
                    printf("Complete message received: %s", full_message);

                    // 移除 "\r\n" 並處理訊息
                    *strstr(full_message, "\r\n") = '\0';
                    atbash_cipher(full_message);

                    printf("Processed message: %s\n\n", full_message);

                    // 回傳處理後的訊息
                    send(new_socket, full_message, strlen(full_message), 0);

                    // 重置緩衝區
                    memset(full_message, 0, sizeof(full_message));
                    full_len = 0;
                }
            }

            close(new_socket);
            exit(0); // 結束
        }

        close(new_socket); // 關閉客戶端 socket
    }

    return 0;
}
