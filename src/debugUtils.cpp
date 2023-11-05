#include "../include/debugUtils.h"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

std::string getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    std::tm now_tm = *std::localtime(&now_c);
    std::stringstream ss;
    ss << std::put_time(&now_tm, "%H:%M:%S");
    return ss.str();
}