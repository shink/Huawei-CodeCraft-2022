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

#define TEST

#ifdef TEST
#define PrintLog(...)           \
    printf(__VA_ARGS__)

#define DEMAND_CSV "D:/GitHub Repository/Huawei-CodeCraft-2022/preliminary/data/demand.csv"
#define SITE_BANDWIDTH_CSV "D:/GitHub Repository/Huawei-CodeCraft-2022/preliminary/data/site_bandwidth.csv"
#define QOS_CSV "D:/GitHub Repository/Huawei-CodeCraft-2022/preliminary/data/qos.csv"
#define CONFIG_INI "D:/GitHub Repository/Huawei-CodeCraft-2022/preliminary/data/config.ini"
#define OUTPUT_PATH "D:/GitHub Repository/Huawei-CodeCraft-2022/preliminary/output/solution.txt"
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

struct Site {
    bandwidth_t totalBandwidth;
    bandwidth_t remainBandwidth;

    explicit Site(bandwidth_t bandwidth) : totalBandwidth(bandwidth), remainBandwidth(bandwidth) {}
};


#endif //HUAWEI_CODECRAFT_2022_STATEMENT_H
