#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h> // 用於處理字母大小寫 

#define PORT 8080
#define serverIP "127.0.0.1"

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

#define BUFFER_SIZE 20  // Buffer 限制為 20

void send_large_message(int sockfd, const char *message) {
    size_t total_length = strlen(message);
    size_t sent = 0;

    while (sent < total_length) {
        size_t chunk_size = (total_length - sent > BUFFER_SIZE) ? BUFFER_SIZE : total_length - sent;
        
        // 印出本次要傳送的字串部分
        printf("Sending chunk: \"%.*s\"\n", (int)chunk_size, message + sent);

        // 傳送字串
        send(sockfd, message + sent, chunk_size, 0);
        sent += chunk_size;
    }

    // 傳送訊息結尾標記（例如 '\n'）
    printf("Sending end marker: \"\\n\"\n");
//    send(sockfd, "\n", 1, 0);
	send(sockfd, "\r\n", 2, 0);
}



int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};

    // 創建 Socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Socket creation error\n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("Invalid address/ Address not supported\n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Connection failed\n");
        return -1;
    }

    printf("Connected to server...\n");

    while (1) {
        char message[1024];
        printf("Enter message: ");
        fgets(message, sizeof(message), stdin);

        // 去掉換行符
        message[strcspn(message, "\n")] = 0;

        // 加密訊息
        atbash_cipher(message);

        // 傳送分段訊息
        send_large_message(sock, message);
		
        // 接收到 exit 指令就退出迴圈
        if (strcmp(message, "exit") == 0)
            break;

        memset(buffer, 0, sizeof(buffer));
        int valread = read(sock, buffer, BUFFER_SIZE);
//        printf("Server: %s\n", buffer); 
    }

    if (close(sock) < 0) {
        perror("close socket failed!");
    }
    return 0;
}


