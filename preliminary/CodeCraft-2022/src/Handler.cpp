/*
 * @author: shenke
 * @date: 2022/3/14
 * @project: HuaWei_CodeCraft_2022
 * @desp:
 */


#include "Handler.h"

Handler::Handler() {}

void Handler::Read() {
#ifdef TEST
    PrintLog("[Reading]\n");
    auto start = std::chrono::system_clock::now();
#endif

    ReadConfig(CONFIG_INI);
    ReadSiteBandwidth(SITE_BANDWIDTH_CSV);
    ReadQos(QOS_CSV);
    ReadDemand(DEMAND_CSV);

#ifdef TEST
    std::chrono::duration<double, std::milli> duration = std::chrono::system_clock::now() - start;
    PrintLog("Read elapsed time: %.4fms\n", duration.count());
    PrintLog("T: %d, M: %d, N: %d\n", T, M, N);
#endif
}

void Handler::Initialize() {
#ifdef TEST
    PrintLog("[Initializing]\n");
#endif

    vs = 0u;
    vt = M + N + 1u;
    nodeCount = M + N + 2u;
    siteBandwidthList.resize(N, std::vector<SiteBandwidthInfo>(T));
    result.resize(T, std::vector<std::vector<bandwidth_t >>(M, std::vector<bandwidth_t>(N, 0u)));
    siteBandwidthTimeSeries.resize(N, std::vector<std::pair<uint16_t, bandwidth_t >>(T));
    hotspotList.resize(N, std::vector<bool>(T, false));

    // 按照边缘节点入度升序排列
    for (uint8_t m = 0; m < M; ++m) {
        std::vector<SiteNode> &siteList = serviceSiteList[m];
        for (SiteNode &siteNode: siteList) {
            siteNode.inDegree = serviceCustomerList[siteNode.siteId].size();
        }
        std::sort(siteList.begin(), siteList.end(), [](const SiteNode &a, const SiteNode &b) {
            return a.inDegree < b.inDegree;
        });
    }

    // 设置边缘节点优先级列表
    SetPrioritySiteList();
}

void Handler::Handle() {
#ifdef TEST
    std::cout << "[Handling]" << std::endl;
    auto start = std::chrono::system_clock::now();
#endif

    Initialize();
    // UseUpChance();

    // 按每日总需求降序处理
    std::sort(dailyBandwidthDemandList.begin(), dailyBandwidthDemandList.end(), [](const DailyDemand &a, const DailyDemand &b) {
        return a.bandwidthDemand > b.bandwidthDemand;
    });
    uint16_t t;
    for (uint16_t i = 0u; i < T; ++i) {
        t = dailyBandwidthDemandList[i].dayId;

#ifdef TEST
        PrintLog("Handling %d day\n", t);
#endif

        HandleDailyDemand(t);
    }

#ifdef TEST
    std::chrono::duration<double, std::milli> duration = std::chrono::system_clock::now() - start;
    PrintLog("Handle elapsed time: %.4fms\n", duration.count());
    Check();
#endif
}

/**
 * @brief 事先把所有边缘节点的拉满机会用完
 */
void Handler::UseUpChance() {
    // TODO

    // 每个边缘的拉满次数，不能超过 5% * T
    std::vector<uint16_t> counts(N, 0u);

    uint16_t totalChance = T - p95Idx - 1;
    for (uint16_t i = 0; i < N * totalChance; ++i) {
        bool quit = false;

        // 按每日总需求降序处理
        std::sort(dailyBandwidthDemandList.begin(), dailyBandwidthDemandList.end(), [](const DailyDemand &a, const DailyDemand &b) {
            return a.bandwidthDemand > b.bandwidthDemand;
        });

        for (uint16_t j = 0u; j < T && !quit; ++j) {
            DailyDemand &dailyDemand = dailyBandwidthDemandList[j];
            uint16_t t = dailyDemand.dayId;

            // 按客户节点当天的带宽需求降序处理
            std::vector<bandwidth_t> &customerDailyDemandList = bandwidthDemandList[t];
            std::vector<std::pair<uint8_t, bandwidth_t>> customerOrderList(M);
            for (uint8_t m = 0u; m < M; ++m) {
                customerOrderList[m] = std::pair<uint8_t, bandwidth_t>(m, customerDailyDemandList[m]);
            }
            std::sort(customerOrderList.begin(), customerOrderList.end(),
                      [](const std::pair<uint8_t, bandwidth_t> &a, const std::pair<uint8_t, bandwidth_t> &b) {
                          return a.second > b.second;
                      });

            for (uint8_t k = 0u; k < M && !quit; ++k) {
                // 将客户节点所连接的入度最少的边缘节点拉满
                uint8_t customerId = customerOrderList[k].first;
                const std::vector<SiteNode> &siteList = serviceSiteList[customerId];
                for (const SiteNode &site: siteList) {
                    uint8_t n = site.siteId;
                    if (counts[n] >= totalChance || hotspotList[n][t]) continue;

                    ++counts[n];
                    hotspotList[n][t] = true;
                    quit = true;

                    bandwidth_t &capacity = siteBandwidthList[n][t].remainBandwidth;
                    std::vector<uint8_t> &customerIdList = serviceCustomerList[n];

                    // 按客户节点带宽需求降序处理
                    SortCustomerDailyDemand(customerIdList, t);

                    for (const uint8_t &m: customerIdList) {
                        bandwidth_t allocatedBandwidth = std::min(customerDailyDemandList[m], capacity);
                        capacity -= allocatedBandwidth;
                        customerDailyDemandList[m] -= allocatedBandwidth;
                        dailyDemand.bandwidthDemand -= allocatedBandwidth;
                        // result[t][m][n] += allocatedBandwidth;
                    }
                }
            }
        }
    }
}

void Handler::HandleDailyDemand(const uint16_t &t) {
    // 初始化边缘节点使用情况
    memset(aliveSiteList, 0, sizeof(aliveSiteList));

#ifdef TEST
    for (const bool &b: aliveSiteList) assert(!b);
#endif

    // 构造初始图，只包含超级源、超级汇、所有客户节点、当天的热点边缘节点
    InitializeGraph(t);

    // 跑最大流，尝试用所有热点边缘节点填满客户需求
    bandwidth_t dailyDemand = accumulate(bandwidthDemandList[t].begin(), bandwidthDemandList[t].end(), 0u);
    bandwidth_t maxFlow = 0u;
    while (maxFlow < dailyDemand) {
        maxFlow += Dinic();
        if (maxFlow < dailyDemand) {
            AddPriorityNode();  // 当需求无法满足时，从优先级列表中取出一个边缘节点继续跑最大流，直至满足需求
        }
    }

    // 保存结果，边缘节点指向客户节点的边上（即反向边）的 flow 即为分配的流量
    std::vector<std::vector<bandwidth_t>> &res = result[t];
    uint16_t edgeIdx;
    for (uint8_t n = 0u; n < N; ++n) {
        for (uint8_t m = 0u; m < M; ++m) {
            edgeIdx = graph[M + n + 1u][m + 1];
            res[m][n] += edgeList[edgeIdx].capacity;
        }
    }
}

void Handler::InitializeGraph(const uint16_t &t) {
    const std::vector<bandwidth_t> &dailyDemandList = bandwidthDemandList[t];
    edgeCount = 0u;
    edgeList.clear();
    std::memset(head, -1, sizeof(head));
    for (uint8_t i = 0u; i < nodeCount; ++i) std::memset(graph[i], 0, sizeof(graph[i]));

#ifdef TEST
    for (uint8_t i = 0u; i < nodeCount; ++i) assert(head[i] == -1);
    for (uint8_t i = 0u; i < nodeCount; ++i) {
        for (uint8_t j = 0u; j < nodeCount; ++j) assert(graph[i][j] == 0u);
    }
#endif

    // 所有客户节点连接超级源，容量为带宽需求
    for (uint8_t m = 0u; m < M; ++m) {
        AddEdge(vs, m + 1u, dailyDemandList[m]);
        AddEdge(m + 1u, vs, 0u);
    }

    // 客户节点与热点边缘节点连接，热点与超级汇连接
    for (uint8_t n = 0u; n < N; ++n) {
        if (hotspotList[n][t]) {
            uint8_t to = M + n + 1u;

            // 客户节点连接热点边缘节点，容量为 INF
            for (const uint8_t &m: serviceCustomerList[n]) {
                uint8_t from = m + 1u;
                AddEdge(from, to, INF);
                AddEdge(to, from, 0u);
            }

            // 热点边缘节点连接超级汇，容量为节点带宽容量
            AddEdge(to, vt, siteCapacityList[n]);
            AddEdge(vt, to, 0u);

            // 标记节点
            aliveSiteList[n] = true;
        }
    }
}

/**
 * @brief Dinic 最大流
 */
bandwidth_t Handler::Dinic() {
    bandwidth_t flow = 0u;
    while (Bfs()) {
        // 初始化当前弧，每一次 BFS 建立完分层图后都要把 cur 置为每一个节点的第一条边
        std::memcpy(cur, head, sizeof(head));
        flow += Dfs(vs, INF);
    }
    return flow;
}

bool Handler::Bfs() {
    std::memset(depth, 0u, sizeof(depth));
    depth[vs] = 1u;  // 超级源深度为 1
    std::queue<uint8_t> queue;
    queue.push(vs);
    while (!queue.empty()) {
        uint8_t from = queue.front(), to;
        queue.pop();
        for (int16_t eg = head[from]; eg != -1; eg = edgeList[eg].next) {
            to = edgeList[eg].to;
            if (depth[to] == 0u && edgeList[eg].capacity > 0u) {
                depth[to] = depth[from] + 1u;
                queue.push(to);
            }
        }
    }
    // 当未访问到超级汇 t 时，说明已经不存在其他的增广路径
    return depth[vt] != 0u;
}

/**
 * @return 返回本次增广的流量
 */
bandwidth_t Handler::Dfs(const uint8_t &from, const bandwidth_t &capacity) {
    if (from == vt || capacity == 0u) return capacity;
    bandwidth_t flow = capacity;    // 当前要通过的流量大小
    for (int16_t &eg = cur[from]; eg != -1 && flow; eg = edgeList[eg].next) {
        uint8_t to = edgeList[eg].to;
        bandwidth_t &cap = edgeList[eg].capacity;

        if (depth[to] == depth[from] + 1u && cap > 0u) {
            bandwidth_t residual = Dfs(to, std::min(cap, flow));
            flow -= residual;                       // 减小当前可通过流量
            cap -= residual;                        // 正向增流，减小容量
            edgeList[eg ^ 1].capacity += residual;  // 反向减流，增大容量
        }
    }
    return capacity - flow;
}

void Handler::AddEdge(const uint8_t &from, const uint8_t &to, const bandwidth_t &capacity) {
    edgeList.emplace_back(Edge(head[from], to, capacity));
    head[from] = (int16_t) edgeCount;
    graph[from][to] = edgeCount++;
}

void Handler::SetPrioritySiteList() {
    // TODO
    for (uint8_t i = 0u; i < N; ++i) {
        prioritySiteList[i] = i;
    }
}

void Handler::AddPriorityNode() {
    uint8_t n, from, to;
    for (uint8_t i = 0u; i < N; i++) {
        n = prioritySiteList[i];
        if (aliveSiteList[n]) continue;

        to = M + n + 1u;
        // 边缘节点连接客户节点
        for (uint8_t &m: serviceCustomerList[n]) {
            from = m + 1u;
            AddEdge(from, to, INF);
            AddEdge(to, from, 0u);
        }
        // 边缘节点连接超级汇
        AddEdge(to, vt, siteCapacityList[n]);
        AddEdge(vt, to, 0u);
    }
}

/**
 * @brief 按用户需求降序排序
 */
void Handler::SortCustomerDailyDemand(std::vector<uint8_t> &customerIdList, const uint16_t &t) {
    std::vector<std::pair<uint8_t, bandwidth_t>> demandInfoList;
    const std::vector<bandwidth_t> &customerDailyDemandList = bandwidthDemandList[t];
    for (const uint8_t &m: customerIdList) {
        demandInfoList.emplace_back(m, customerDailyDemandList[m]);
    }
    std::sort(demandInfoList.begin(), demandInfoList.end(),
              [](const std::pair<uint8_t, bandwidth_t> &a, const std::pair<uint8_t, bandwidth_t> &b) {
                  return a.second > b.second;
              });
    for (uint8_t i = 0u; i < (uint8_t) customerIdList.size(); ++i) {
        customerIdList[i] = demandInfoList[i].first;
    }
}

void Handler::Output() {
#ifdef TEST
    std::cout << "[Outputting]" << std::endl;
    auto start = std::chrono::system_clock::now();
#endif

    FILE *fp = fopen(OUTPUT_PATH, "w");

    uint32_t outputIdx;
    for (uint16_t t = 0u; t < T; ++t) {
        for (uint8_t m = 0u; m < M; ++m) {
            outputIdx = 0u;
            outputIdx += NodeName(outputBuffer + outputIdx, customerNameMap[m]);
            outputBuffer[outputIdx++] = ':';

            for (uint8_t n = 0u; n < N; ++n) {
                if (result[t][m][n] > 0u) {
                    outputBuffer[outputIdx++] = '<';
                    outputIdx += NodeName(outputBuffer + outputIdx, siteNameMap[n]);
                    outputBuffer[outputIdx++] = ',';
                    outputIdx += Int2charArray(outputBuffer + outputIdx, result[t][m][n]);
                    outputBuffer[outputIdx++] = '>';
                    outputBuffer[outputIdx++] = ',';
                }
            }
            if (outputBuffer[outputIdx - 1u] == ',') --outputIdx;
            if (t < T - 1u || m < M - 1u) outputBuffer[outputIdx++] = '\n';
            fwrite(outputBuffer, outputIdx, sizeof(char), fp);
        }
    }

    fclose(fp);

#ifdef TEST
    std::chrono::duration<double, std::milli> duration = std::chrono::system_clock::now() - start;
    PrintLog("Output elapsed time: %.4fms\n", duration.count());
#endif
}

void Handler::Check() {
#ifdef TEST
    // 检查输出的分配方案是否与需求相符
    for (uint16_t t = 0u; t < T; ++t) {
        for (uint8_t m = 0u; m < M; ++m) {
            bandwidth_t sum = 0u;
            for (uint8_t n = 0u; n < N; ++n) sum += result[t][m][n];
            if (sum != tmpBandwidthDemandList[t][m]) {
                PrintLog("Wrong answer, day: %d, demand: %d, sum: %d\n", t, tmpBandwidthDemandList[t][m], sum);
            }
        }
    }
#endif
}

void Handler::ReadConfig(const string &configPath) {
    std::ifstream ifs;
    ifs.open(configPath.c_str());
    std::string line;

    // 忽略第一行 '[config]'
    std::getline(ifs, line);

    // 读第二行 'qos_constraint=400'
    std::getline(ifs, line);

    uint16_t i = 0u;
    qos_t num = 0u;

    while (line[i] != '=') ++i;
    ++i;
    while (std::isdigit(line[i])) num = num * 10u + (line[i++] - '0');
    qosLimit = num;

    ifs.close();
}

void Handler::ReadSiteBandwidth(const string &siteBandwidthPath) {
    std::ifstream ifs;
    ifs.open(siteBandwidthPath);
    std::string line;

    // 跳过第一行，'site_name,bandwidth'
    std::getline(ifs, line);

    // 读取剩余 N 行
    std::vector<string> vec;
    uint8_t n = 0u;
    while (std::getline(ifs, line)) {
        ReadLine(line, vec);
        node_name_t siteName = NodeName(vec[0].c_str(), vec[0].size());
        siteIdMap[siteName] = n++;
        siteNameMap.emplace_back(siteName);
        siteCapacityList.emplace_back(atoi(vec[1].c_str()));
    }
    N = n;
    ifs.close();
}

/**
 * example:
 *     site_name,CB,CA,CE,CX
 *     S1,200,186,125,89
 *     SB,190,48,458,45
 *     SD,340,45,124,335
 *
 * @brief 读取 qos
 * @param qosPath /data/qos.csv
 */
void Handler::ReadQos(const string &qosPath) {
    std::ifstream ifs;
    ifs.open(qosPath);
    std::string line;

    // 读取表头
    std::getline(ifs, line);
    std::vector<std::string> vec;
    ReadLine(line, vec);
    M = (uint8_t) (vec.size() - 1);

    // QOS 列表，包含每个边缘节点到每个客户节点的时延
    // 维度为 M x N
    std::vector<std::vector<qos_t>> qosList(M, std::vector<qos_t>(N, 0));
    node_name_t demandName;

    // 客户节点名称和 ID 的映射关系
    customerNameMap.resize(M);
    for (uint8_t i = 0u; i < M; ++i) {
        demandName = NodeName(vec[i + 1].c_str(), vec[i + 1].size());
        customerIdMap[demandName] = i;
        customerNameMap[i] = demandName;
    }

    // 读取剩余 N 行
    node_name_t siteName;
    for (uint8_t i = 0u; i < N && std::getline(ifs, line); ++i) {
        ReadLine(line, vec);
        siteName = NodeName(vec[0].c_str(), vec[0].size());
        uint8_t n = siteIdMap[siteName];

        for (uint8_t m = 0u; m < M; ++m) {
            qosList[m][n] = atoi(vec[m + 1].c_str());
        }
    }

    ifs.close();

    // 客户节点和边缘节点的关联关系
    serviceSiteList.resize(M);
    serviceCustomerList.resize(N);
    for (uint8_t m = 0; m < M; ++m) {
        for (uint8_t n = 0; n < N; ++n) {
            if (qosList[m][n] < qosLimit) {
                serviceSiteList[m].emplace_back(SiteNode(n));
                serviceCustomerList[n].emplace_back(m);
            }
        }
    }
}

/**
 * example:
 *     mtime,CB,CA,CE,CX
 *     2021-10-19T00:00,9706,13409,5209,0
 *     2021-10-19T00:05,8127,11154,4262,300
 *     2021-10-19T00:10,7243,9865,3713,500
 *
 * @brief 读取
 * @param demandPath /data/demand.csv
 */
void Handler::ReadDemand(const string &demandPath) {
    std::ifstream ifs;
    ifs.open(demandPath);
    std::string line;

    // 读取表头
    std::getline(ifs, line);
    std::vector<std::string> vec;
    ReadLine(line, vec);

    // 防止 demand.csv 和 qos.csv 的表头不一致
    uint8_t tmpCustomerIdList[M];
    for (uint8_t i = 0u; i < M; ++i) {
        node_name_t demandName = NodeName(vec[i + 1].c_str(), vec[i + 1].size());
        tmpCustomerIdList[i] = customerIdMap[demandName];
    }

    // 读取剩余行
    uint16_t t = 0u;
    bandwidth_t bandwidthPerDay, bandwidth;
    while (std::getline(ifs, line)) {
        ReadLine(line, vec);
        bandwidthPerDay = 0u;
        bandwidthDemandList.emplace_back(std::vector<bandwidth_t>(M));
        for (uint8_t i = 0u; i < M; ++i) {
            bandwidth = atoi(vec[i + 1u].c_str());
            bandwidthDemandList[t][tmpCustomerIdList[i]] = bandwidth;
            bandwidthPerDay += bandwidth;
        }
        dailyBandwidthDemandList.emplace_back(DailyDemand(t++, bandwidthPerDay));
    }
    T = t;
    p95Idx = (uint16_t) std::ceil(t * 0.95) - 1;
    ifs.close();

#ifdef TEST
    tmpBandwidthDemandList = bandwidthDemandList;
#endif
}

void Handler::ReadLine(const string &line, std::vector<string> &vec) {
    uint16_t n = line.size();
    uint16_t left = 0u, right = 0u;
    if (line.back() == '\r') --n;

    vec.clear();
    std::string tmp;

    while (right < n) {
        while (right < n && line[right] != ',') ++right;
        vec.emplace_back(line.substr(left, right - left));
        left = ++right;
    }
}

/**
 * @brief 62 进制数转 char
 */
uint8_t Handler::NodeName(char *buffer, const node_name_t &nodeName) {
    base62_t idx1 = (base62_t) (nodeName / 63u), idx2 = nodeName - idx1 * 63U;
    assert(idx1 >= 1 || idx2 >= 1);

    if (idx1 >= 1 && idx2 >= 1) {
        *buffer = BASE62[idx1 - 1];
        *(buffer + 1) = BASE62[idx2 - 1];
        return 2u;
    } else {
        *buffer = BASE62[idx2 - 1];
        return 1u;
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
    return Base63(ch);
}

node_name_t Handler::NodeName(const char &ch1, const char &ch2) {
    return Base63(ch1) * 63u + Base63(ch2);
}

/**
 * char 转 63 进制数，[0, 62]，0 表示为空
 *
 * example:
 *  '' -> 0
 *  '0' -> 1
 *  'A' -> 11
 *  'z' -> 62
 *  '01' -> 1 * 63 + 2 = 65
 *  'Cz' -> 14 * 63 + 62 = 944
 */
base62_t Handler::Base63(const char &ch) {
    return std::isdigit(ch) ? (ch - 47u) : (std::isupper(ch) ? (ch - 54u) : ch - 60u);
}

uint8_t Handler::Int2charArray(char *buffer, bandwidth_t bandwidth) {
    uint8_t size = 1u + (uint8_t) log10(bandwidth);
    char *p = buffer + size - 1u;
    for (uint8_t i = 0u; i < size; ++i) {
        *p = (uint8_t) (bandwidth % 10u) + '0';
        bandwidth /= 10u;
        --p;
    }
    return size;
}
