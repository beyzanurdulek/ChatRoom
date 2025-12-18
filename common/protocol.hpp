#pragma once
#include <string>
#include <vector>
#include <sstream>

inline std::vector<std::string> split(const std::string& s, char delim = '|') {
    std::vector<std::string> out; std::stringstream ss(s); std::string item;
    while (std::getline(ss, item, delim)) out.push_back(item);
    return out;
}
inline std::string join(const std::vector<std::string>& v, char delim = '|') {
    std::ostringstream os;
    for (size_t i = 0;i < v.size();++i) { if (i) os << delim; os << v[i]; }
    return os.str();
}
