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

    siteBandwidthList.resize(N, std::vector<SiteInfo>(T));
    result.resize(T, std::vector<std::vector<bandwidth_t >>(M, std::vector<bandwidth_t>(N, 0u)));
    siteBandwidthTimeSeries.resize(N, std::vector<std::pair<uint16_t, bandwidth_t >>(T));

    customerOrderList.resize(M);
    std::vector<std::pair<uint8_t, uint8_t>> serviceSiteCountList;
    for (uint8_t m = 0u; m < M; ++m) {
        serviceSiteCountList.emplace_back(m, (uint8_t) serviceSiteList[m].size());
    }
    std::sort(serviceSiteCountList.begin(), serviceSiteCountList.end(),
              [](const std::pair<uint8_t, uint8_t> &a, const std::pair<uint8_t, uint8_t> &b) {
                  return a.second < b.second;
              });
    for (uint8_t i = 0u; i < M; ++i) {
        customerOrderList[i] = serviceSiteCountList[i].first;
    }
}

void Handler::Handle() {
#ifdef TEST
    std::cout << "[Handling]" << std::endl;
    auto start = std::chrono::system_clock::now();
#endif

    Initialize();

    // 1. 事先把所有边缘节点的拉满机会用完
    // 所有边缘节点每天的负载需求，维度为 N x T
    std::vector<std::vector<bandwidth_t>> demandSumList(N, std::vector<bandwidth_t>(T, 0u));
    for (uint8_t n = 0u; n < N; ++n) {
        for (uint16_t t = 0u; t < T; ++t) {
            siteBandwidthList[n][t] = SiteInfo(siteList[n].capacity);
            const std::vector<uint8_t> &customerIdList = serviceCustomerList[n];
            for (const uint8_t &m: customerIdList) {
                demandSumList[n][t] += bandwidthDemandList[t][m];
            }
        }
    }

    // 依次处理负载最多的边缘节点
    uint16_t totalChance = T - p95Idx - 1;
    for (uint16_t i = 0; i < N * totalChance; ++i) {
        bandwidth_t maxDemand = 0u;
        uint8_t n;
        uint16_t t;
        for (uint8_t j = 0u; j < N; ++j) {
            for (uint16_t k = 0; k < T && siteList[j].usedChance < totalChance; ++k) {
                if (demandSumList[j][k] >= maxDemand) {
                    maxDemand = demandSumList[j][k];
                    n = j;
                    t = k;
                }
            }
        }

        std::vector<uint8_t> &customerIdList = serviceCustomerList[n];
        std::vector<bandwidth_t> &demandOfDay = bandwidthDemandList[t];

        // 按照需求降序排序
        SortDemand(customerIdList, 0, (int) customerIdList.size() - 1, t);

        bandwidth_t &capacity = siteBandwidthList[n][t].remainBandwidth;
        ++siteList[n].usedChance;
        siteBandwidthList[n][t].alive = false;

        for (const uint8_t &m: customerIdList) {
            bandwidth_t allocatedBandwidth = std::min(demandOfDay[m], capacity);
            capacity -= allocatedBandwidth;
            siteBandwidthList[n][t].allocatedBandwidth += allocatedBandwidth;
            demandOfDay[m] -= allocatedBandwidth;
            demandSumList[n][t] -= allocatedBandwidth;
            result[t][m][n] += allocatedBandwidth;
            for (const uint8_t &siteId: serviceSiteList[m]) {
                if (siteId != n) demandSumList[siteId][t] -= allocatedBandwidth;
            }
        }
    }

    // 2. 后续采用均衡策略
    for (uint16_t t = 0u; t < T; ++t) {
        HandlePerDay(t);
        // BalancePerDay(t);
    }

    // 3. p95 均衡
    // Balance();

#ifdef TEST
    std::chrono::duration<double, std::milli> duration = std::chrono::system_clock::now() - start;
    PrintLog("Handle elapsed time: %.4fms\n", duration.count());
    Check();
#endif
}

void Handler::HandlePerDay(const uint16_t &t) {
    // 处理每一个客户节点
    for (uint8_t i = 0; i < M; ++i) {
        HandlePerCustomer(t, i);
    }
}

void Handler::HandlePerCustomer(const uint16_t &t, const uint8_t &m) {
    bandwidth_t bandwidthDemand = bandwidthDemandList[t][m];
    if (bandwidthDemand == 0u) return;

    uint8_t size = serviceSiteList[m].size();
    uint8_t n;
    bandwidth_t sum = 0u;

    for (uint8_t i = 0; i < size; ++i) {
        n = serviceSiteList[m][i];
        sum += siteBandwidthList[n][t].remainBandwidth;
    }
    assert(sum >= bandwidthDemand);

    bandwidth_t tmpSum = 0u;
    std::vector<std::vector<bandwidth_t>> &resultOfDay = result[t];
    bandwidth_t allocatedBandwidth;
    for (uint8_t i = 0u; i < size; ++i) {
        n = serviceSiteList[m][i];
        const bandwidth_t &remainBandwidth = siteBandwidthList[n][t].remainBandwidth;
        if (i != size - 1u) {
            allocatedBandwidth = std::min(remainBandwidth,
                                          (bandwidth_t) floor(bandwidthDemand * 1.0 * remainBandwidth / sum));

            tmpSum += allocatedBandwidth;
        } else {
            allocatedBandwidth = bandwidthDemand - tmpSum;
        }

        if (remainBandwidth < allocatedBandwidth) {
            while (allocatedBandwidth > 0) {
                for (uint8_t j = 0u; j < size && allocatedBandwidth > 0; ++j) {
                    n = serviceSiteList[m][j];
                    if (siteBandwidthList[n][t].remainBandwidth > 0u) {
                        --siteBandwidthList[n][t].remainBandwidth;
                        ++siteBandwidthList[n][t].allocatedBandwidth;
                        --allocatedBandwidth;
                        ++resultOfDay[m][n];
                    }
                }
            }
            continue;
        }

        resultOfDay[m][n] += allocatedBandwidth;
        siteBandwidthList[n][t].remainBandwidth -= allocatedBandwidth;
        siteBandwidthList[n][t].allocatedBandwidth += allocatedBandwidth;
    }
}

void Handler::BalancePerDay(const uint16_t &t) {
    // 将负载压力大于均值的非热点边缘节点迁移至小于均值的非热点边缘节点
    bandwidth_t totalBandwidth, migratedBandwidth, originAllocatedBandwidth, destAllocatedBandwidth;;
    uint8_t count = 0u;
    for (uint8_t n = 0u; n < N; ++n) {
        // 如果该边缘节点当天已拉满，则跳过
        if (!siteBandwidthList[n][t].alive) continue;
        ++count;
        totalBandwidth += siteBandwidthList[n][t].allocatedBandwidth;
    }
    bandwidth_t averageBandwidth = totalBandwidth / count;
    bool quit = false;

    for (uint8_t originSiteId = 0u; originSiteId < N; ++originSiteId) {
        originAllocatedBandwidth = siteBandwidthList[originSiteId][t].allocatedBandwidth;
        if (originAllocatedBandwidth <= averageBandwidth) continue;

        const std::vector<uint8_t> &serviceCustomerOfSite = serviceCustomerList[originSiteId];
        for (const uint8_t &m: serviceCustomerOfSite) {

            bandwidth_t &allocatedBandwidth = result[t][m][originSiteId];
            const std::vector<uint8_t> &serviceSiteOfCustomer = serviceCustomerList[m];
            for (const uint8_t &destSiteId: serviceSiteOfCustomer) {
                destAllocatedBandwidth = siteBandwidthList[destSiteId][t].allocatedBandwidth;
                if (originSiteId != destSiteId && destAllocatedBandwidth < averageBandwidth) {
                    // 迁移
                    migratedBandwidth = std::min(allocatedBandwidth, std::min(originAllocatedBandwidth - averageBandwidth,
                                                                              averageBandwidth - destAllocatedBandwidth));
                    allocatedBandwidth -= migratedBandwidth;
                    result[t][m][destSiteId] += migratedBandwidth;
                    siteBandwidthList[originSiteId][t].remainBandwidth += migratedBandwidth;
                    siteBandwidthList[originSiteId][t].allocatedBandwidth -= migratedBandwidth;
                    siteBandwidthList[destSiteId][t].remainBandwidth -= migratedBandwidth;
                    siteBandwidthList[destSiteId][t].allocatedBandwidth += migratedBandwidth;

                    quit = true;
                    break;
                }
            }
            if (quit) break;
        }
    }
}

/**
 * @brief 均衡每个边缘节点的 p95Idx 值
 */
void Handler::Balance() {
    std::pair<uint16_t, bandwidth_t> p95Info;

    for (uint8_t n = 0u; n < N; ++n) {
        // n 的 p95Idx 值及对应的天数
        p95Info = GetP95Info(n);
        uint16_t t = p95Info.first;
        bandwidth_t p95 = p95Info.second;

        // 尝试将带宽分配给其他兄弟节点
        bandwidth_t brotherP95, brotherBandwidthTotal, migratedBandwidth;
        uint16_t brotherP95Day;
        const std::vector<uint8_t> &serviceCustomerOfSite = serviceCustomerList[n];
        for (const uint8_t &m: serviceCustomerOfSite) {
            bandwidth_t &allocatedBandwidth = result[t][m][n];
            if (allocatedBandwidth == 0u) continue;

            const std::vector<uint8_t> &serviceSiteOfCustomer = serviceCustomerList[m];
            for (const uint8_t &brotherId: serviceSiteOfCustomer) {
                if (allocatedBandwidth == 0u) break;
                if (n == brotherId) continue;

                // 兄弟节点对应的 p95Idx 值及对应的天数
                p95Info = GetP95Info(brotherId);
                brotherP95Day = p95Info.first;
                brotherP95 = p95Info.second;

                if (t == brotherP95Day) continue;

                // 兄弟节点第 t 天的带宽负载
                brotherBandwidthTotal = siteBandwidthList[brotherId][t].allocatedBandwidth;
                if (brotherBandwidthTotal >= brotherP95) continue;

                // 在不影响兄弟节点 p95Idx 值的前提下，取已分配带宽与兄弟节点能承受带宽的较小值
                migratedBandwidth = std::min(allocatedBandwidth,
                                             std::min(siteBandwidthList[brotherId][t].remainBandwidth, brotherP95 - brotherBandwidthTotal));

                // 迁移
                allocatedBandwidth -= migratedBandwidth;
                result[t][m][brotherId] += migratedBandwidth;
                siteBandwidthList[n][t].remainBandwidth += migratedBandwidth;
                siteBandwidthList[n][t].allocatedBandwidth -= migratedBandwidth;
                siteBandwidthList[brotherId][t].remainBandwidth -= migratedBandwidth;
                siteBandwidthList[brotherId][t].allocatedBandwidth += migratedBandwidth;
            }
        }
    }
}

std::pair<uint16_t, bandwidth_t> Handler::GetP95Info(const uint8_t &n) {
    std::vector<std::pair<uint16_t, bandwidth_t>> &series = siteBandwidthTimeSeries[n];
    for (uint16_t t = 0u; t < T; ++t) {
        series[t].first = t;
        series[t].second = siteBandwidthList[n][t].allocatedBandwidth;
    }

    std::sort(series.begin(), series.end(),
              [](const std::pair<uint16_t, bandwidth_t> &a, const std::pair<uint16_t, bandwidth_t> &b) {
                  return a.second < b.second;
              });

    return std::pair<uint16_t, bandwidth_t>({series[p95Idx].first, series[p95Idx].second});
}

void Handler::SortDemand(std::vector<uint8_t> &customerIdList, const int &left, const int &right, const uint16_t &t) {
    if (left > right) return;
    const std::vector<bandwidth_t> demandOfDay = bandwidthDemandList[t];

    int l = left, r = right;
    bandwidth_t pivot = customerIdList[left];
    while (l < r) {
        while (l < r && demandOfDay[pivot] >= demandOfDay[customerIdList[r]]) --r;
        while (l < r && demandOfDay[pivot] <= demandOfDay[customerIdList[l]]) ++l;
        if (l < r) std::swap(customerIdList[l], customerIdList[r]);
    }
    customerIdList[left] = customerIdList[l];
    customerIdList[l] = pivot;

    SortDemand(customerIdList, left, r - 1, t);
    SortDemand(customerIdList, r + 1, right, t);
}

void Handler::Output() {
#ifdef TEST
    std::cout << "[Outputting]" << std::endl;
    auto start = std::chrono::system_clock::now();
#endif

    FILE *fp = fopen(OUTPUT_PATH, "w");

    uint32_t outputIdx;
    for (uint16_t t = 0; t < T; ++t) {
        for (uint8_t m = 0; m < M; ++m) {
            outputIdx = 0u;
            outputIdx += NodeName(outputBuffer + outputIdx, customerNameMap[m]);
            outputBuffer[outputIdx++] = ':';

            for (uint8_t n = 0; n < N; ++n) {
                if (result[t][m][n] > 0) {
                    outputBuffer[outputIdx++] = '<';
                    outputIdx += NodeName(outputBuffer + outputIdx, siteNameMap[n]);
                    outputBuffer[outputIdx++] = ',';
                    outputIdx += Int2charArray(outputBuffer + outputIdx, result[t][m][n]);
                    outputBuffer[outputIdx++] = '>';
                    outputBuffer[outputIdx++] = ',';
                }
            }
            if (outputBuffer[outputIdx - 1] == ',') --outputIdx;
            if (t < T - 1 || m < M - 1) outputBuffer[outputIdx++] = '\n';
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
    for (uint16_t t = 0u; t < T; ++t) {
        for (uint8_t m = 0u; m < M; ++m) {
            bandwidth_t sum = 0u;
            for (uint8_t n = 0u; n < N; ++n) sum += result[t][m][n];
            if (sum != tmpBandwidthDemandList[t][m]) {
                PrintLog("Wrong answer, day: %d, demand: %d, sum: %d\n", t, tmpBandwidthDemandList[t][m], sum);
            }
        }

        for (uint8_t n = 0u; n < N; ++n) {
            bandwidth_t sum = 0u;
            for (uint8_t m = 0u; m < M; ++m) {
                sum += result[t][m][n];
            }
            assert(sum + siteBandwidthList[n][t].remainBandwidth == siteList[n].capacity);
        }
    }

    uint32_t cost = 0u;
    for (uint8_t n = 0u; n < N; ++n) GetP95Info(n);
    for (std::vector<std::pair<uint16_t, bandwidth_t>> &series: siteBandwidthTimeSeries) {
        cost += series[p95Idx].second;
    }
    PrintLog("Total cost: %d\n", cost);
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
        siteList.emplace_back(Site(atoi(vec[1].c_str())));
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
                serviceSiteList[m].emplace_back(n);
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
    while (std::getline(ifs, line)) {
        ReadLine(line, vec);
        bandwidthDemandList.emplace_back(std::vector<bandwidth_t>(M));
        for (uint8_t i = 0u; i < M; ++i) {
            bandwidthDemandList[t][tmpCustomerIdList[i]] = atoi(vec[i + 1u].c_str());
        }
        ++t;
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
