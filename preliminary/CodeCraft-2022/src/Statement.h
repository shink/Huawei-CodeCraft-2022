/*
 * @author: shenke
 * @date: 2022/3/14
 * @project: HuaWei_CodeCraft_2022
 * @desp:
 */


#ifndef HUAWEI_CODECRAFT_2022_STATEMENT_H
#define HUAWEI_CODECRAFT_2022_STATEMENT_H

#include <bits/stdc++.h>
#include <sys/stat.h>
#include <cstdint>

#define TEST
#define SIMULATE

#ifdef TEST
#define PrintLog(...)           \
    printf(__VA_ARGS__)

#ifdef SIMULATE
#define DEMAND_CSV "D:/GitHub Repository/Huawei-CodeCraft-2022/preliminary/CodeCraft2022-PressureGenerator/simulated_data/demand.csv"
#define SITE_BANDWIDTH_CSV "D:/GitHub Repository/Huawei-CodeCraft-2022/preliminary/CodeCraft2022-PressureGenerator/simulated_data/site_bandwidth.csv"
#define QOS_CSV "D:/GitHub Repository/Huawei-CodeCraft-2022/preliminary/CodeCraft2022-PressureGenerator/simulated_data/qos.csv"
#define CONFIG_INI "D:/GitHub Repository/Huawei-CodeCraft-2022/preliminary/CodeCraft2022-PressureGenerator/simulated_data/config.ini"
#define OUTPUT_PATH "D:/GitHub Repository/Huawei-CodeCraft-2022/preliminary/output/solution.txt"
#else
#define DEMAND_CSV "D:/GitHub Repository/Huawei-CodeCraft-2022/preliminary/data/demand.csv"
#define SITE_BANDWIDTH_CSV "D:/GitHub Repository/Huawei-CodeCraft-2022/preliminary/data/site_bandwidth.csv"
#define QOS_CSV "D:/GitHub Repository/Huawei-CodeCraft-2022/preliminary/data/qos.csv"
#define CONFIG_INI "D:/GitHub Repository/Huawei-CodeCraft-2022/preliminary/data/config.ini"
#define OUTPUT_PATH "D:/GitHub Repository/Huawei-CodeCraft-2022/preliminary/output/solution.txt"
#endif
#else
#define PrintLog(...)
#define DEMAND_CSV "/data/demand.csv"
#define SITE_BANDWIDTH_CSV "/data/site_bandwidth.csv"
#define QOS_CSV "/data/qos.csv"
#define CONFIG_INI "/data/config.ini"
#define OUTPUT_PATH "/output/solution.txt"
#endif


using std::string;


typedef uint8_t base62_t;
typedef uint16_t node_name_t, qos_t;
typedef uint32_t bandwidth_t;

struct Edge {
    int16_t next{-1};           // 与该边同起点的下一条边的索引位置
    uint8_t to{0u};             // 该边的终点
    bandwidth_t capacity{0u};   // 该边的容量

    Edge() = default;

    explicit Edge(int16_t next, uint8_t to, bandwidth_t capacity) : next(next), to(to), capacity(capacity) {}
};


#endif //HUAWEI_CODECRAFT_2022_STATEMENT_H
