#ifndef TCP_CONFIG_H
#define TCP_CONFIG_H

namespace Config {
    constexpr int FILENAME_DATA_LEN = 200;

    const char* const SERVER_IP = "127.0.0.1";
    constexpr uint16_t SERVER_PORT = 10000;

    const char* const SERVER_FILE_DIR_NAME = "files";

    constexpr long BUFF_SIZE = 4096;
}

#endif // TCP_CONFIG_H