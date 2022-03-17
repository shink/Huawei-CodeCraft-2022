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
    void ReadDemand(const string &demandPath);

    void ReadSiteBandwidth();

    void ReadQos();

    void ReadConfig(const string &configPath);

    void ReadDemandCsvHeader();

    template<typename E>
    uint8_t ReadLine(const char *buffer, uint32_t startIdx, E *res, bool skipFirstColumn, node_name_t &siteNameValue);

    template<typename E>
    uint8_t ReadValue(const char *buffer, E &value);

    inline char *NodeNameString(const node_name_t &customerName);

    inline node_name_t NodeName(const char *buffer, uint8_t len);

    inline node_name_t NodeName(const char &ch);

    inline node_name_t NodeName(const char &ch1, const char &ch2);

    inline base62_t Base62(const char &ch);

    static const uint16_t QOS_MAX = 1000u;
    static const uint8_t MAX_N = 135u;
    static const uint8_t MAX_M = 35u;
    static const uint16_t MAX_T = 8928u;

    static const node_name_t MAX_CLIENT_ID = 62 * 62 + 62;
    static const node_name_t MAX_SITE_ID = 62 * 62 + 62;

    constexpr const static char BASE62[62] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
                                              'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
                                              'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
                                              'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
                                              'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'};

    uint8_t N{0u};
    uint8_t M{0u};
    uint16_t T{0u};
    qos_t qosLimit;

    node_name_t clientNameMap[MAX_M];
    uint8_t clientIdMap[MAX_CLIENT_ID];

    /*
     * 从 demand.csv 读入
     * 客户节点的带宽需求列表，包含所有客户节点在不同时刻的带宽需求信息
     * 维度为 T x M
     */
    bandwidth_t **bandwidthDemandList;

    /*
     * 从 site_bandwidth.csv 读入
     * 边缘节点列表，包含每个边缘节点的使用带宽和剩余带宽
     * 维度为 N
     */
    Site *siteList;

    /*
     * 从 qos.csv 读入
     * QOS 列表，包含每个边缘节点到每个客户节点的时延
     * 维度为 N x M
    */
    qos_t *qosList;

    //

    /*
     * 计算结果
     * 维度为 T x M x N
     */
    bandwidth_t ***result;
};


#endif //HUAWEI_CODECRAFT_2022_HANDLER_H
