#include <iostream>
#include <string>
#include <cstring>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <filesystem>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <algorithm>

#include "tcp_config.h"

using namespace std;

int client_sockfd;

bool send_filename(const string& absolute_path);
bool send_filesize(uint64_t file_size);
bool send_file();

int main()
{
    client_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sockfd == -1)
    {
        perror("Failed to create socket: ");
        return -1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(Config::SERVER_IP);
    server_addr.sin_port = htons(Config::SERVER_PORT);  // 主机字节序 → 网络字节序（大端）

    if (connect(client_sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("Failed to connect server: ");
        return -1;
    }

    while (1)
    {
        // system("clear");
        cout << "----------------------------------\n";
        cout << "1.按s来发送文件\n";
        cout << "2.按q退出程序\n";
        cout << "----------------------------------\n";

        char choice;
        cin >> choice;

        switch (choice)
        {
        case 's':
            send_file();
            break;

        // exit program
        case 'q':
            close(client_sockfd);
            return 0;
        
        default:
            cout << "请选择程序支持的选项！\n\n";
            break;
        }

    }

    return 0;
}


// 发送文件名
bool send_filename(const string& absolute_path)
{
    string filename = std::filesystem::path(absolute_path).filename().string();

    char filename_buf[Config::FILENAME_DATA_LEN] = {0};
    size_t copy_len = min(Config::FILENAME_DATA_LEN-1, static_cast<int>(filename.size()));
    memcpy(filename_buf, filename.c_str(), copy_len);

    int total = 0;
    while(total < Config::FILENAME_DATA_LEN)
    {
        int len = send(client_sockfd, filename_buf + total, Config::FILENAME_DATA_LEN - total, 0);
        if (len <= 0)
        {
            perror("Failed to send filename: ");
            return false;
        }
        total += len;
    }
    return true;
}

// 发送文件大小
bool send_filesize(uint64_t file_size)
{
    // 转网络字节序大端
    uint64_t net_size = htobe64(file_size);
    int total = 0;
    int data_len = sizeof(net_size);

    while(total < data_len)
    {
        int len = send(client_sockfd, (char*)&net_size + total, data_len - total, 0);
        if (len <= 0)
        {
            perror("Failed to send file size: ");
            return false;
        }
        total += len;
    }
    return true;
}

bool send_file()
{
    string file_path;
    cout << "请输入要传输的文件的绝对路径：";
    cin.ignore();   // 忽略之前的换行符
    getline(cin, file_path);

    ifstream send_file(file_path, ios::binary);
    if (!send_file.is_open())
    {
        perror("Failed to open file: ");
        return false;
    }
    
    // 发送文件名称
    if (!send_filename(file_path)) return false;

    send_file.seekg(0, ios::end);   // 读指针移到末尾
    streampos pos = send_file.tellg(); // 获取文件大小
    if (pos == -1)
    {
        cerr << "获取文件大小失败\n";
        send_file.close();
        return false;
    }
    uint64_t file_len = static_cast<uint64_t>(pos);
    send_file.seekg(0, ios::beg);   // 读指针移到开头

    // 发送文件大小
    if (!send_filesize(file_len)) return false;

    // 发送文件内容
    char buf[Config::BUFF_SIZE] = {0};
    while(true)
    {
        send_file.read(buf, Config::BUFF_SIZE);
        int readLen = send_file.gcount();   // gcount返回实际读到的字节数
        if (readLen <= 0) break;

        int send_cnt = 0;
        while (send_cnt < readLen)
        {
            int len = send(client_sockfd, buf + send_cnt, readLen - send_cnt, 0);
            if (len <= 0)
            {
                perror("文件发送中断: ");
                return false;
            }
            send_cnt += len;
        }
    }

    send_file.close();
    cout << "文件发送完成!\n";
    return true;
}