/*
 * @author: shenke
 * @date: 2022/3/14
 * @project: HuaWei_CodeCraft_2022
 * @desp:
 */


#include "Handler.h"

Handler::Handler() {
    PrintLog("[Initializing]\n");
}

void Handler::Read() {
#ifdef TEST
    PrintLog("[Reading]\n");
    auto start = std::chrono::system_clock::now();
#endif

    ReadConfig(CONFIG_INI);
    ReadDemand(DEMAND_CSV);
    ReadSiteBandwidth(SITE_BANDWIDTH_CSV);
    ReadQos(QOS_CSV);

#ifdef TEST
    std::chrono::duration<double, std::milli> duration = std::chrono::system_clock::now() - start;
    PrintLog("Read elapsed time: %.4fms\n", duration.count());
#endif
}

void Handler::ReadDemand(const string &demandPath) {
    std::ifstream ifs;
    ifs.open(demandPath);
    std::string line;

    // 读取表头
    std::getline(ifs, line);
    std::vector<std::string> vec;
    ReadLine(line, vec);

    M = (uint8_t) (vec.size() - 1);
    customerNameMap.resize(M);
    for (uint8_t i = 0u; i < M; ++i) {
        // node_name_t demandName = NodeName(vec[i + 1].c_str(), vec[i + 1].size());
        string demandName = vec[i + 1];
        customerIdMap[demandName] = i;
        customerNameMap[i] = demandName;
    }

    // 读取剩余行
    uint16_t t = 0u;
    while (std::getline(ifs, line)) {
        ReadLine(line, vec);
        bandwidthDemandList.emplace_back();
        for (uint8_t i = 1u; i <= M; ++i) {
            bandwidthDemandList[t].emplace_back(atoi(vec[i].c_str()));
        }
        ++t;
    }
    T = t;
    ifs.close();

#ifdef TEST
    // for (node_name_t &name: customerNameMap) {
    //     PrintLog("Customer id: %d, base62_name: %d, name: %s\n", customerIdMap[name], name, NodeNameString(name).c_str());
    // }

    for (string &name: customerNameMap) {
        PrintLog("Customer id: %d,  name: %s\n", customerIdMap[name], name.c_str());
    }
#endif
}

void Handler::ReadSiteBandwidth(const string &siteBandwidthPath) {
    std::ifstream ifs;
    ifs.open(siteBandwidthPath);
    std::string line;

    // 跳过第一行，'site_name,bandwidth'
    std::getline(ifs, line);

    // 读取剩余部分
    std::vector<string> vec;
    uint8_t n = 0u;
    while (std::getline(ifs, line)) {
        ReadLine(line, vec);
        // node_name_t siteName = NodeName(vec[0].c_str(), vec[0].size());
        string siteName = vec[0];

        siteIdMap[siteName] = n++;
        siteNameMap.emplace_back(siteName);
        siteList.emplace_back(Site(atoi(vec[1].c_str())));
    }
    N = n;
    ifs.close();
}

void Handler::ReadQos(const string &qosPath) {
    std::ifstream ifs;
    ifs.open(qosPath);
    std::string line;

    qosList.resize(M, std::vector<qos_t>(N, 0));

    // 读取表头
    std::getline(ifs, line);
    std::vector<std::string> vec;
    ReadLine(line, vec);

    uint8_t tmpCustomerIdList[M];
    for (uint8_t i = 0u; i < M; ++i) {
        // node_name_t demandName = NodeName(vec[i + 1].c_str(), vec[i + 1].size());
        string demandName = vec[i + 1];
        tmpCustomerIdList[i] = customerIdMap[demandName];
    }

    // 读取剩余内容
    for (uint8_t n = 0u; n < N && std::getline(ifs, line); ++n) {
        ReadLine(line, vec);

        // node_name_t siteName = NodeName(vec[0].c_str(), vec[0].size());
        string siteName = vec[0];

        uint8_t siteId = siteIdMap[siteName];
        for (uint8_t m = 0u; m < M; ++m) {
            qosList[tmpCustomerIdList[m]][siteId] = atoi(vec[m + 1].c_str());
        }
    }

    ifs.close();

    // 服务节点
    serviceSiteList.resize(M);
    for (uint8_t m = 0; m < M; ++m) {
        for (uint8_t n = 0; n < N; ++n) {
            if (qosList[m][n] < qosLimit) serviceSiteList[m].emplace_back(n);
        }
    }
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

void Handler::ReadLine(const string &line, std::vector<string> &vec) {
    uint16_t n = line.size();
    uint16_t left = 0u, right = 0u;
    if (line.back() == '\r') --n;

    vec.clear();
    std::string tmp;

    while (right < n) {
        while (right < n && line[right] != ',') right += 1;
        vec.push_back(line.substr(left, right - left));
        left = ++right;
    }
}

/**
 * @brief 62 进制数转 char
 */
string Handler::NodeNameString(const node_name_t &customerName) {
    base62_t idx1 = (base62_t) (customerName / 62u), idx2 = customerName - idx1 * 62U;
    assert(idx1 >= 1 || idx2 >= 1);

    string s;
    if (idx1 >= 1 && idx2 >= 1) {
        s += BASE62[idx1 - 1];
        s += BASE62[idx2 - 1];
    } else {
        s += BASE62[idx2 - 1];
    }
    return s;
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

void Handler::Handle() {
#ifdef TEST
    std::cout << "[Handling]" << std::endl;
    auto start = std::chrono::system_clock::now();
#endif

    result.resize(T, std::vector<std::vector<bandwidth_t>>(M, std::vector<bandwidth_t>(N, 0u)));
    uint16_t t = 0;

    for (std::vector<bandwidth_t> &demand: bandwidthDemandList) {
#ifdef TEST
        if (t % 10 == 0) PrintLog("Handling %d day\n", t);
#endif
        HandlePerDay(t++);
    }

#ifdef TEST
    std::chrono::duration<double, std::milli> duration = std::chrono::system_clock::now() - start;
    PrintLog("Output elapsed time: %.4fms\n", duration.count());
    Check();
#endif
}

void Handler::Output() {
#ifdef TEST
    std::cout << "[Outputting]" << std::endl;
    auto start = std::chrono::system_clock::now();
#endif

    std::ofstream ofs;
    ofs.open(OUTPUT_PATH);
    for (uint16_t t = 0; t < T; ++t) {
        for (uint8_t m = 0; m < M; ++m) {
            // std::string str = NodeNameString(customerNameMap[m]) + ":";
            std::string str = customerNameMap[m] + ":";
            for (uint8_t n = 0; n < N; ++n) {
                if (result[t][m][n] > 0) {
                    // str += "<" + NodeNameString(siteNameMap[n]) + "," + std::to_string(result[t][m][n]) + ">,";
                    str += "<" + siteNameMap[n] + "," + std::to_string(result[t][m][n]) + ">,";
                }
            }
            if (str.back() == ',') str.pop_back();
            if (t < T - 1 || m < M - 1) str += '\n';
            ofs << str;
            // std::cout << str;
        }
    }
    ofs.close();

#ifdef TEST
    std::chrono::duration<double, std::milli> duration = std::chrono::system_clock::now() - start;
    PrintLog("Output elapsed time: %.4fms\n", duration.count());
#endif
}

void Handler::Check() {
    for (uint16_t t = 0; t < T; ++t) {
        for (uint8_t m = 0; m < M; ++m) {
            bandwidth_t sum = 0;
            for (uint8_t n = 0; n < N; n += 1) {
                sum += result[t][m][n];
            }
            if (sum != bandwidthDemandList[t][m]) {
                PrintLog("Wrong answer, day: %d, demand: %d, sum: %d\n", t, bandwidthDemandList[t][m], sum);
                // exit(-1);
            }
        }
    }
}

void Handler::HandlePerDay(const uint16_t &today) {
    // 清空前一天的存放情况
    for (Site &site: siteList) {
        site.remainBandwidth = site.totalBandwidth;
    }

    // 处理每一个客户节点
    for (uint8_t m = 0; m < M; ++m) {
        HandlePerCustomer(today, m);
    }
}

void Handler::HandlePerCustomer(const uint16_t &today, const uint8_t &customerId) {
    bandwidth_t bandwidthDemand = bandwidthDemandList[today][customerId];
    uint8_t n = serviceSiteList[customerId].size();
    assert(n > 0);

    uint8_t siteId;
    uint32_t sum = 0;
    bandwidth_t remainBandwidth[n];

    for (uint8_t i = 0; i < n; ++i) {
        siteId = serviceSiteList[customerId][i];
        sum += (remainBandwidth[i] = siteList[siteId].remainBandwidth);
    }

    uint32_t tmpSum = 0;
    auto &res = result[today];
    bandwidth_t allocatedBandwidth;
    for (uint8_t i = 0; i < n; ++i) {
        siteId = serviceSiteList[customerId][i];
        if (i != n - 1) {
            allocatedBandwidth = (bandwidth_t) floor(bandwidthDemand * 1.0 * remainBandwidth[i] / sum);
            tmpSum += allocatedBandwidth;
        } else {
            allocatedBandwidth = bandwidthDemand - tmpSum;
        }
        res[customerId][siteId] = allocatedBandwidth;

#ifdef TEST
        if (siteList[siteId].remainBandwidth < allocatedBandwidth) {
            // PrintLog("Error, service (%d: %s) no enough space\n", siteId, NodeNameString(siteNameMap[siteId]).c_str());
            PrintLog("Error, service (%d: %s) no enough space\n", siteId, siteNameMap[siteId].c_str());
        }
#endif

        siteList[siteId].remainBandwidth -= allocatedBandwidth;
    }
}
