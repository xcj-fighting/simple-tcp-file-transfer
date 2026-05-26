#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdint>
#include <cstring>
#include <unistd.h>
#include <thread>
#include <vector>
#include <filesystem>
#include <fstream>

#include "tcp_config.h"

using namespace std;

int server_sockfd;

bool create_local_folder();
string recv_filename(int client_sockfd);
uint64_t recv_filesize(int client_sockfd);
void recv_file(int client_sockfd, ofstream& file, uint64_t file_size);
void handle_client(int client_sockfd);

int main()
{
    // create socket
    server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sockfd == -1)
    {
        perror("Failed to create server socket: ");
        return -1;
    }

    // set reuseaddr
    int opt = 1;
    if (setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
    {
        perror("setsockopt failed");
        close(server_sockfd);
        return -1;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(Config::SERVER_IP);
    server_addr.sin_port = htons(Config::SERVER_PORT);
    memset(server_addr.sin_zero, 0, sizeof(server_addr.sin_zero));

    // bind
    if (bind(server_sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("Failed to bind addr: ");
        close(server_sockfd);
        return -1;
    }

    // listen
    if (listen(server_sockfd, 1024) == -1)
    {
        perror("Failed to listen: ");
        close(server_sockfd);
        return -1;
    }

    cout << "服务器启动成功，等待客户端连接..." << endl;

    while (1)
    {
        int client_sockfd = accept(server_sockfd, NULL, NULL);
        if (client_sockfd == -1)
        {
            perror("accept error: ");
            continue;;
        }

        thread t(handle_client, client_sockfd);
        t.detach();
    }

    close(server_sockfd);

    return 0;
}

bool create_local_folder()
{
    string dir_path = filesystem::current_path().string()  + "/" + Config::SERVER_FILE_DIR_NAME;
    if (!filesystem::exists(dir_path)) return filesystem::create_directory(dir_path);
    return true;
}

string recv_filename(int client_sockfd)
{
    int total = 0;
    char buf[Config::FILENAME_DATA_LEN] = {0};
    while (total < Config::FILENAME_DATA_LEN)
    {
        int len = recv(client_sockfd, buf + total, Config::FILENAME_DATA_LEN - total, 0);
        if (len <= 0)
        {
            perror("Faile to receive file name: ");
            return "";
        }
        total += len;
    }
    return string(buf);
}

uint64_t recv_filesize(int client_sockfd)
{
    int total = 0;
    uint64_t file_size;
    int file_size_len = sizeof(file_size);

    while (total < file_size_len)
    {
        int len = recv(client_sockfd, (char*)&file_size + total, file_size_len - total, 0);
        if (len <= 0)
        {
            perror("Faile to receive file size: ");
            return -1;
        }
        total += len;
    }
    return be64toh(file_size);
}

void recv_file(int client_sockfd, ofstream& file, uint64_t file_size)
{
    uint64_t total = 0;
    char buff[Config::BUFF_SIZE] = {0};
    while (total < file_size)
    {
        uint64_t left = file_size - total;
        int read_len = left > Config::BUFF_SIZE ? Config::BUFF_SIZE : left;
        int len = recv(client_sockfd, buff, read_len, 0);
        if (len <= 0)
        {
            perror("文件接收中断: ");
            return;
        }

        file.write(buff, len);
        if (!file)
        {
            cerr << "写入文件失败！\n";
            close(client_sockfd);
            return;
        }
        total += len;
    }
}

void handle_client(int client_sockfd)
{
    string filename = recv_filename(client_sockfd);
    if (filename.empty())
    {
        close(client_sockfd);
        return;
    }

    uint64_t filesize = recv_filesize(client_sockfd);
    if (filesize <= 0)
    {
        close(client_sockfd);
        return;
    }

    if (!create_local_folder())
    {
        cerr << "create dir error\n";
        close(client_sockfd);
        return;
    }

    string file_abosulte_path = filesystem::current_path().string() + "/" + Config::SERVER_FILE_DIR_NAME + "/" + filename;
    ofstream file(file_abosulte_path, ios_base::binary);
    if (!file)
    {
        std::error_code ec;
        filesystem::file_status file_status = filesystem::status(file_abosulte_path, ec);
        cerr << "打开文件失败：" << ec.message() << std::endl;
        return;
    }

    recv_file(client_sockfd, file, filesize);

    close(client_sockfd);
}