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

    void Initialize();

    void UseUpChance();

    void SetPrioritySiteList();

    void HandleDailyDemand(const uint16_t &t);

    void InitializeGraph(const uint16_t &t);

    bandwidth_t Dinic();

    bool Bfs();

    bandwidth_t Dfs(const uint8_t &from, const bandwidth_t &capacity);

    void AddEdge(const uint8_t &from, const uint8_t &to, const bandwidth_t &capacity);

    void AddPriorityNode();

    void Check();

    void ReadDemand(const string &demandPath);

    void ReadSiteBandwidth(const string &siteBandwidthPath);

    void ReadQos(const string &qosPath);

    void ReadConfig(const string &configPath);

    void ReadLine(const string &line, std::vector<string> &vec);

    uint8_t IntToCharArray(char *buffer, bandwidth_t bandwidth);

    inline uint8_t NodeName(char *buffer, const node_name_t &nodeName);

    inline node_name_t NodeName(const char *buffer, uint8_t len);

    inline node_name_t NodeName(const char &ch);

    inline node_name_t NodeName(const char &ch1, const char &ch2);

    inline base62_t Base63(const char &ch);

    static const uint16_t QOS_MAX = 1000u;
    static const uint8_t MAX_N = 135u;
    static const uint8_t MAX_M = 35u;
    static const uint16_t MAX_T = 8928u;
    static const uint8_t MAX_NODE_COUNT = MAX_N + MAX_M + 2u;
    const bandwidth_t INF = 0x3f3f3f3f;

    static const node_name_t MAX_CUSTOMER_ID = 62 * 63 + 63;
    static const node_name_t MAX_SITE_ID = 62 * 63 + 63;

    static const uint32_t OUTPUT_BUFFER_SIZE = 1 << 12;

    const char BASE62[62] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
                             'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
                             'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
                             'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
                             'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'};

    uint8_t N{0u};
    uint8_t M{0u};
    uint16_t T{0u};
    uint16_t p95Idx{0u};   // 95 百分位带宽下标
    qos_t qosLimit{0u};

    uint8_t vs{0u};         // 超级源
    uint8_t vt{0u};         // 超级汇
    uint8_t nodeCount{0u};  // 节点数量
    uint16_t edgeCount{0u}; // 边数量

    uint16_t graph[MAX_NODE_COUNT][MAX_NODE_COUNT];
    std::vector<Edge> edgeList;                 // 链式前向星，存放图中的所有边
    int16_t head[MAX_NODE_COUNT];               // 节点的第一条边的索引位置，-1 表示没有边
    int16_t cur[MAX_NODE_COUNT];                // 当前弧优化，DFS 时记录当前节点循环到了哪一条边，避免重复计算
    uint8_t depth[MAX_NODE_COUNT];              // BFS 分层图中节点深度

    bool aliveSiteList[MAX_N];                  // 标记边缘节点是否在网络图中
    uint8_t prioritySiteList[MAX_N];            // 边缘节点优先级队列

    node_name_t customerNameMap[MAX_M];         // customerId -> name
    uint8_t customerIdMap[MAX_CUSTOMER_ID];     // name -> customerId

    node_name_t siteNameMap[MAX_N];             // siteId -> name
    uint8_t siteIdMap[MAX_SITE_ID];             // name -> siteId

    char outputBuffer[OUTPUT_BUFFER_SIZE];      // 存放每一行的结果

    /**
     * 从 site_bandwidth.csv 读入
     * 边缘节点列表，包含每个边缘节点的使用带宽和剩余带宽
     * 维度为 N
     */
    bandwidth_t siteCapacityList[MAX_N];

    /**
     * 从 demand.csv 读入
     * 客户节点的带宽需求列表，包含所有客户节点在不同时刻的带宽需求信息
     * 维度为 T x M
     */
    std::vector<std::vector<bandwidth_t>> customerDemandList;

    /**
     * 每个客户节点的所有可服务的边缘节点，按照边缘节点入度升序排列
     * 维度为 M x n
     */
    std::vector<std::vector<uint8_t>> serviceSiteList;

    /**
     * 每个边缘节点的所有可服务的客户节点
     * 维度为 N x m
     */
    std::vector<std::vector<uint8_t>> serviceCustomerList;

    /**
     * 边缘节点某天是否是热点
     * 维度为 N x T
     */
    std::vector<std::vector<bool>> hotspotList;

    /**
     * 最终结果
     * 维度为 T x M x N
     */
    std::vector<std::vector<std::vector<bandwidth_t>>> result;

#ifdef TEST
    /**
     * 用于检查，同 {@code customerDemandList}
     * 客户节点的带宽需求列表，包含所有客户节点在不同时刻的带宽需求信息
     */
    std::vector<std::vector<bandwidth_t>> tmpBandwidthDemandList;
#endif
};


#endif //HUAWEI_CODECRAFT_2022_HANDLER_H
