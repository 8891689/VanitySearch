/*MIT License

Copyright (c) 2025 8891689

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE. 
*/
#include <stdio.h>
#include <stdint.h> // For uint32_t
#include <cstdlib>

#include "Random.h"

#ifdef _WIN32
    #include <stdlib.h>  // 提供 rand_s
#else
    #include <fcntl.h>
    #include <unistd.h>
    #include <errno.h>
#endif

// 全局文件描述符，避免每次打开关闭 /dev/urandom
#ifndef _WIN32
static int urandom_fd = -1;

// 初始化 /dev/urandom 文件描述符
static void init_urandom_fd() {
    if (urandom_fd < 0) {
        urandom_fd = open("/dev/urandom", O_RDONLY);
        if (urandom_fd < 0) {
            perror("无法打开 /dev/urandom");
            // 致命错误，退出或者标记错误状态
            // 为了简单，这里选择退出
            exit(EXIT_FAILURE);
        }
    }
}
#endif


int generateRandomBinary(char *bin, int bits) {
    if (bits < 0 || bits > 512) {
        return -1;
    }
    if (bits == 0) {
        bin[0] = '\0';
        return 0;
    }

    int current_bin_idx = 0; // Current index in the output bin string

#ifdef _WIN32
    unsigned int random_dword;
    for (int i = 0; i < bits; ) { // Iterate until all 'bits' are filled
        if (rand_s(&random_dword) != 0) {
            perror("rand_s failed");
            return -1;
        }
        // Extract up to 32 bits from this random_dword, MSB first
        for (int j = 0; j < 32 && i < bits; ++j, ++i) {
            // Get the (31-j)-th bit of random_dword (0-indexed from LSB, so 31 is MSB)
            bin[current_bin_idx++] = ((random_dword >> (31 - j)) & 1) ? '1' : '0';
        }
    }
#else
    init_urandom_fd(); // Ensure fd is initialized (idempotent)
    unsigned char random_byte;
    for (int i = 0; i < bits; ) { // Iterate until all 'bits' are filled
        if (read(urandom_fd, &random_byte, 1) != 1) {
            perror("读取 /dev/urandom 失败");
            return -1;
        }
        // Extract up to 8 bits from this random_byte, MSB first
        for (int j = 0; j < 8 && i < bits; ++j, ++i) {
            // Get the (7-j)-th bit of random_byte (0-indexed from LSB, so 7 is MSB)
            bin[current_bin_idx++] = ((random_byte >> (7 - j)) & 1) ? '1' : '0';
        }
    }
#endif
    bin[bits] = '\0'; // Null-terminate the string
    return 0;
}

void convertBinaryToHex(const char *bin, char *hex, int bits) {
    int hexDigits = bits / 4;
    for (int i = 0; i < hexDigits; i++) {
        int value = 0;
        // 每4位二进制转换为一个0～15的数值
        for (int j = 0; j < 4; j++) {
            value = value * 2 + (bin[i * 4 + j] - '0');
        }
        // 转换为16进制字符
        hex[i] = (value < 10) ? ('0' + value) : ('A' + (value - 10));
    }
    hex[hexDigits] = '\0';
}


//  rndl() 的实现
uint32_t rndl() {
    uint32_t random_value;
#ifdef _WIN32
    if (rand_s(&random_value) != 0) {
        perror("rand_s failed in rndl");
        // 致命错误，无法生成随机数，退出
        exit(EXIT_FAILURE);
    }
#else
    init_urandom_fd(); // 确保 fd 已初始化
    if (read(urandom_fd, &random_value, sizeof(random_value)) != sizeof(random_value)) {
        perror("读取 /dev/urandom 失败 in rndl");
         // 致命错误，无法生成随机数，退出
        exit(EXIT_FAILURE);
    }
#endif
    return random_value;
}
