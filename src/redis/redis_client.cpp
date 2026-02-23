#include "otlp_redis_metrics/redis/redis_client.hpp"

#include <sstream>

namespace otlp_redis_metrics::redis {

RedisClient::RedisClient() : ctx_(nullptr) {}
RedisClient::~RedisClient() {
  if (ctx_ != nullptr) {
    redisFree(ctx_);
  }
}

bool RedisClient::Connect(const ::otlp::redis::metrics::config::RedisConfig& cfg) {
  ctx_ = redisConnect(cfg.host().c_str(), static_cast<int>(cfg.port()));
  if (ctx_ == nullptr || ctx_->err) {
    return false;
  }
  if (cfg.db() != 0) {
    CommandArgv({"SELECT", std::to_string(cfg.db())});
  }
  if (!cfg.password().empty()) {
    CommandArgv({"AUTH", cfg.password()});
  }
  return true;
}

std::optional<std::string> RedisClient::Command(const std::string& cmd) {
  auto* reply = static_cast<redisReply*>(redisCommand(ctx_, cmd.c_str()));
  if (reply == nullptr) {
    return std::nullopt;
  }

  std::string out;
  if (reply->type == REDIS_REPLY_ERROR) {
    out = reply->str ? reply->str : "redis error";
  } else if (reply->str != nullptr) {
    out = reply->str;
  } else {
    out = "OK";
  }
  freeReplyObject(reply);
  return out;
}

std::optional<std::string> RedisClient::CommandArgv(const std::vector<std::string>& args) {
  if (args.empty()) {
    return std::nullopt;
  }

  std::vector<const char*> argv;
  std::vector<size_t> argvlen;
  argv.reserve(args.size());
  argvlen.reserve(args.size());
  for (const auto& arg : args) {
    argv.push_back(arg.c_str());
    argvlen.push_back(arg.size());
  }

  auto* reply = static_cast<redisReply*>(
      redisCommandArgv(ctx_, static_cast<int>(argv.size()), argv.data(), argvlen.data()));
  if (reply == nullptr) {
    return std::nullopt;
  }
  std::string out;
  if (reply->type == REDIS_REPLY_ERROR) {
    out = reply->str ? reply->str : "redis error";
  } else if (reply->str != nullptr) {
    out = reply->str;
  } else {
    out = "OK";
  }
  freeReplyObject(reply);
  return out;
}

bool RedisClient::Pipeline(const std::vector<std::string>& cmds) {
  for (const auto& cmd : cmds) {
    if (redisAppendCommand(ctx_, cmd.c_str()) != REDIS_OK) {
      return false;
    }
  }
  for (size_t i = 0; i < cmds.size(); ++i) {
    redisReply* reply = nullptr;
    if (redisGetReply(ctx_, reinterpret_cast<void**>(&reply)) != REDIS_OK) {
      return false;
    }
    freeReplyObject(reply);
  }
  return true;
}

}  // namespace otlp_redis_metrics::redis
