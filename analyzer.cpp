#include "analyzer.h"
#include <fstream>
#include <iostream>
#include <string_view>
#include <algorithm>
#include <cctype>

using namespace std;

static inline bool is_ws(unsigned char c) {
    return std::isspace(c);
}

static inline string_view trim_view(string_view s) {
    size_t a = 0, b = s.size();
    while (a < b && is_ws((unsigned char)s[a])) a++;
    while (b > a && is_ws((unsigned char)s[b - 1])) b--;
    return s.substr(a, b - a);
}

static inline bool parseHourRobust_sv(string_view dt, int& hourOut) {
    size_t pSpace = dt.find(' ');
    size_t pT = dt.find('T');
    size_t p;
    if (pSpace == string_view::npos) p = pT;
    else if (pT == string_view::npos) p = pSpace;
    else p = min(pSpace, pT);
    if (p == string_view::npos) return false;
    size_t i = p + 1;
    while (i < dt.size() && is_ws((unsigned char)dt[i])) i++;
    if (i >= dt.size() || !isdigit((unsigned char)dt[i])) return false;
    int h = dt[i++] - '0';
    if (i < dt.size() && isdigit((unsigned char)dt[i]))
        h = h * 10 + (dt[i++] - '0');
    if (h < 0 || h > 23) return false;
    hourOut = h;
    return true;
}

void TripAnalyzer::processLine(const std::string& line, bool& firstLine) {
    string_view sv = trim_view(line);
    if (sv.empty()) return;
    if (firstLine) {
        firstLine = false;
        return;
    }

    int colIdx = 0;
    size_t start = 0;
    string_view zoneSv, dtSv;
    bool anyEmpty = false;
    for (size_t i = 0; i <= sv.size(); ++i) {
        if (i == sv.size() || sv[i] == ',') {
            string_view tok = trim_view(sv.substr(start, i - start));
            if (tok.empty()) { anyEmpty = true; break; }
            if (colIdx == 1) zoneSv = tok;
            else if (colIdx == 3) dtSv = tok;
            colIdx++;
            start = i + 1;
        }
    }
    if (anyEmpty || colIdx < 6 || zoneSv.empty() || dtSv.empty())
        return;

    int hour;
    if (!parseHourRobust_sv(dtSv, hour)) return;

    string zone(zoneSv);
    zoneCount[zone]++;
    if (zoneHourCount.find(zone) == zoneHourCount.end()) {
        zoneHourCount[zone] = array<long long, 24>{};
    }
    zoneHourCount[zone][hour]++;
}

void TripAnalyzer::ingestFile(const std::string& csvPath) {
    zoneCount.clear();
    zoneHourCount.clear();
    zoneCount.reserve(1 << 15);
    zoneHourCount.reserve(1 << 15);

    std::ifstream file(csvPath);
    if (!file.is_open())
        return;

    string line;
    bool firstLine = true;
    while (getline(file, line)) {
        processLine(line, firstLine);
    }
}

void TripAnalyzer::ingestStdin() {
    zoneCount.clear();
    zoneHourCount.clear();
    zoneCount.reserve(1 << 15);
    zoneHourCount.reserve(1 << 15);

    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    string line;
    bool firstLine = true;
    while (getline(cin, line)) {
        processLine(line, firstLine);
    }
}

std::vector<ZoneCount> TripAnalyzer::topZones(int k) const {
    if (k <= 0) return {};
    std::vector<ZoneCount> result;
    result.reserve(zoneCount.size());

    for (const auto& p : zoneCount)
        result.push_back({p.first, p.second});

    auto cmp = [](const ZoneCount& a, const ZoneCount& b) {
        if (a.count != b.count) return a.count > b.count;
        return a.zone < b.zone;
    };

    if ((int)result.size() > k) {
        partial_sort(result.begin(), result.begin() + k, result.end(), cmp);
        result.resize(k);
    } else {
        sort(result.begin(), result.end(), cmp);
    }
    return result;
}

std::vector<SlotCount> TripAnalyzer::topBusySlots(int k) const {
    if (k <= 0) return {};
    std::vector<SlotCount> result;
    result.reserve(zoneHourCount.size() * 2);

    for (const auto& z : zoneHourCount) {
        for (int h = 0; h < 24; ++h) {
            long long c = z.second[h];
            if (c > 0)
                result.push_back({z.first, h, c});
        }
    }

    auto cmp = [](const SlotCount& a, const SlotCount& b) {
        if (a.count != b.count) return a.count > b.count;
        if (a.zone != b.zone) return a.zone < b.zone;
        return a.hour < b.hour;
    };

    if ((int)result.size() > k) {
        partial_sort(result.begin(), result.begin() + k, result.end(), cmp);
        result.resize(k);
    } else {
        sort(result.begin(), result.end(), cmp);
    }
    return result;
}
