/*
 * @author: shenke
 * @date: 2022/3/14
 * @project: HuaWei_CodeCraft_2022
 * @desp:
 */


#include "Handler.h"

Handler::Handler() {
    PrintLog("Initializing ...\n");
}

void Handler::Read() {
#ifdef TEST
    auto start = std::chrono::system_clock::now();
#endif

    ReadDemand(DEMAND_CSV);

#ifdef TEST
    std::chrono::duration<double, std::milli> duration = std::chrono::system_clock::now() - start;
    PrintLog("Read elapsed time: %.4fms\n", duration.count());
#endif
}

void Handler::ReadDemand(const string &demandPath) {
    int fd = open(demandPath.c_str(), O_RDONLY);
    long fileSize = lseek(fd, 0, SEEK_END);
    char *buffer = (char *) mmap(NULL, fileSize, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);

    uint32_t i = 0u;

    // 读取第一行表头

    while (buffer[i] != '\n') {
    }

    munmap(buffer, fileSize);
}

void Handler::ReadConfig(const string &configPath) {
    int fd = open(configPath.c_str(), O_RDONLY);
    long fileSize = lseek(fd, 0, SEEK_END);
    char *buffer = (char *) mmap(NULL, fileSize, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);

    uint32_t i = 0u;
    qos_t num = 0;

    // 忽略第一行 '[config]'
    while (buffer[i] != '\n') ++i;

    // 读第二行 'qos_constraint=400'
    while (!std::isdigit(buffer[i])) ++i;
    while (std::isdigit(buffer[i])) num = num * 10u + (buffer[i++] - '0');

    munmap(buffer, fileSize);
}


/**
 * @brief 用于读取文件某一行
 *
 * @param buffer mmap 指针
 * @param res 存放读取结果
 * @param skipFirstColumn 是否跳过读取第一列
 * @param siteNameValue 如果不跳过第一列的话，将第一列的值写入该变量
 * @return 读取值的个数
 */
template<typename E>
uint8_t Handler::ReadLine(const char *buffer, uint32_t startIdx, E *res, uint8_t &count, bool skipFirstColumn, node_name_t &siteNameValue) {
    uint32_t idx = startIdx;

    // 读取第一个值
    while (buffer[idx] != ',') ++idx;
    if (!skipFirstColumn) {
        siteNameValue = NodeName(&buffer[startIdx], idx++ - startIdx);
    }

    // 读取剩余值
    uint32_t sz;
    while ((sz = ReadValue(buffer[idx], res[count])) > 0) {
        idx += sz;
        ++count;
    }
    return count;
}

/**
 * @brief 读取以逗号为间隔的值
 *
 * @tparam E 值类型
 * @param buffer mmap 指针
 * @param value 读取的值写入该变量
 * @return 读取的长度
 */
template<typename E>
uint8_t Handler::ReadValue(const char *buffer, E &value) {
    const char *p = buffer;
    uint8_t size = 0u;
    E num = 0;
    if (*p == '\n') return -1;

    while (std::isdigit(*p)) {
        num = num * 10 + (*p - '0');
        ++p;
        ++size;
    }
    value = num;
    return size;
}

/**
 * @brief 62 进制数转 char
 */
char *Handler::NodeNameString(const node_name_t &customerName) {
    base62_t idx1 = (base62_t) (customerName / 62u), idx2 = customerName - idx1;
    if (idx1 >= 1 && idx2 >= 1) {
        return new char[]{BASE62[idx1 - 1], BASE62[idx2 - 1]};
    } else if (idx2 >= 1) {
        return new char[]{BASE62[idx2 - 1]};
    }
}

/**
 * @brief char 转 62 进制数
 */
node_name_t Handler::NodeName(const char *buffer, uint8_t len) {
    assert(len == 1 || len == 2);
    return len == 1 ? NodeName(*buffer) : NodeName(*buffer, *(buffer + 1));
}

node_name_t Handler::NodeName(const char &ch) {
    return Base62(ch);
}

node_name_t Handler::NodeName(const char &ch1, const char &ch2) {
    return Base62(ch1) * 62u + Base62(ch2);
}

/**
 * char 转 62 进制数
 */
base62_t Handler::Base62(const char &ch) {
    return std::isdigit(ch) ? (ch - 47u) : (std::isupper(ch) ? (ch - 54u) : ch - 60u);
}
