/*
 * @author: shenke
 * @date: 2022/3/14
 * @project: HuaWei_CodeCraft_2022
 * @desp:
 */


#ifndef HUAWEI_CODECRAFT_2022_HANDLER_H
#define HUAWEI_CODECRAFT_2022_HANDLER_H

#include "Statement.h"

class Handler {
public:
    Handler();

    void Read();

    void Handle();

    void Output();

private:

    void HandlePerDay(const uint16_t &today);

    void HandlePerCustomer(const uint16_t &today, const uint8_t &customerId);

    void Check();

    void ReadDemand(const string &demandPath);

    void ReadSiteBandwidth(const string &siteBandwidthPath);

    void ReadQos(const string &qosPath);

    void ReadConfig(const string &configPath);

    void ReadLine(const string &line, std::vector<string> &vec);

    inline string NodeNameString(const node_name_t &customerName);

    inline node_name_t NodeName(const char *buffer, uint8_t len);

    inline node_name_t NodeName(const char &ch);

    inline node_name_t NodeName(const char &ch1, const char &ch2);

    inline base62_t Base62(const char &ch);

    static const uint16_t QOS_MAX = 1000u;
    static const uint8_t MAX_N = 135u;
    static const uint8_t MAX_M = 35u;
    static const uint16_t MAX_T = 8928u;

    static const node_name_t MAX_CUSTOMER_ID = 62 * 62 + 62;
    static const node_name_t MAX_SITE_ID = 62 * 62 + 62;

    static const uint32_t OUTPUT_BUFFER_SIZE = 1 << 12;

    const char BASE62[62] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
                             'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
                             'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
                             'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
                             'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'};

    uint8_t N{0u};
    uint8_t M{0u};
    uint16_t T{0u};
    qos_t qosLimit;

    // id -> name
    // std::vector<node_name_t> customerNameMap;
    std::vector<string> customerNameMap;

    // name -> id
    // uint8_t customerIdMap[MAX_CUSTOMER_ID];
    std::unordered_map<std::string, uint8_t> customerIdMap;

    // id -> name
    // std::vector<node_name_t> siteNameMap;
    std::vector<string> siteNameMap;

    // name -> id
    // uint8_t siteIdMap[MAX_SITE_ID];
    std::unordered_map<std::string, uint8_t> siteIdMap;

    // 存放每一行的结果
    char outputBuffer[OUTPUT_BUFFER_SIZE];

    /*
     * 从 demand.csv 读入
     * 客户节点的带宽需求列表，包含所有客户节点在不同时刻的带宽需求信息
     * 维度为 T x M
     */
    std::vector<std::vector<bandwidth_t>> bandwidthDemandList;

    /*
     * 从 site_bandwidth.csv 读入
     * 边缘节点列表，包含每个边缘节点的使用带宽和剩余带宽
     * 维度为 N
     */
    std::vector<Site> siteList;

    /*
     * 从 qos.csv 读入
     * QOS 列表，包含每个边缘节点到每个客户节点的时延
     * 维度为 M x N
    */
    std::vector<std::vector<qos_t>> qosList;

    /*
     * 每个客户节点的所有服务边缘节点
     * 维度为 M x n
     */
    std::vector<std::vector<uint8_t>> serviceSiteList;

    /*
     * 计算结果
     * 维度为 T x M x N
     */
    std::vector<std::vector<std::vector<bandwidth_t>>> result;
};


#endif //HUAWEI_CODECRAFT_2022_HANDLER_H
