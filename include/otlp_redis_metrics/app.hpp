#pragma once

#include <string>

namespace otlp_redis_metrics {

int Run(int argc, char** argv);

void LogInfo(const std::string& msg);
void LogError(const std::string& msg);

}  // namespace otlp_redis_metrics
