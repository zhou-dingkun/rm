#include <cstdint>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace {
const uint8_t kInfantryType = 0;
const uint8_t kEngineerType = 1;
}

class Robot {
 public:
  Robot(uint16_t team,
        uint16_t id,
        uint8_t type,
        uint16_t hp,
        uint16_t heat,
        uint16_t hp_limit,
        uint16_t heat_limit,
        uint16_t level)
      : team_(team),
        id_(id),
        type_(type),
        hp_(hp),
        heat_(heat),
        hp_limit_(hp_limit),
        heat_limit_(heat_limit),
        level_(level) {}

  virtual ~Robot() = default;

  uint16_t team() const { return team_; }
  uint16_t id() const { return id_; }
  uint8_t type() const { return type_; }
  uint16_t hp() const { return hp_; }
  uint16_t heat() const { return heat_; }
  uint16_t hp_limit() const { return hp_limit_; }
  uint16_t heat_limit() const { return heat_limit_; }
  uint16_t level() const { return level_; }

  void set_hp(uint16_t hp) { hp_ = hp; }
  void set_heat(uint16_t heat) { heat_ = heat; }
  void set_hp_limit(uint16_t hp_limit) { hp_limit_ = hp_limit; }
  void set_heat_limit(uint16_t heat_limit) { heat_limit_ = heat_limit; }
  void set_level(uint16_t level) { level_ = level; }

  void tick_seconds(uint32_t seconds) {
    if (seconds == 0) {
      return;
    }

    uint32_t heat_value = heat_;
    uint32_t limit = heat_limit_;
    uint32_t damage = 0;

    if (heat_value > limit + 1) {
      uint32_t max_over_seconds = heat_value - limit - 1;
      damage = (seconds < max_over_seconds) ? seconds : max_over_seconds;
    }

    uint32_t new_heat = (heat_value > seconds) ? (heat_value - seconds) : 0;
    uint32_t new_hp = (hp_ > damage) ? (hp_ - damage) : 0;

    heat_ = static_cast<uint16_t>(new_heat);
    hp_ = static_cast<uint16_t>(new_hp);
  }

  virtual std::string name() const = 0;

 private:
  uint16_t team_;
  uint16_t id_;
  uint8_t type_;
  uint16_t hp_;
  uint16_t heat_;
  uint16_t hp_limit_;
  uint16_t heat_limit_;
  uint16_t level_;
};

class InfantryRobot : public Robot {
 public:
  using Robot::Robot;

  std::string name() const override { return "Infantry"; }
};

class EngineerRobot : public Robot {
 public:
  using Robot::Robot;

  std::string name() const override { return "Engineer"; }
};

class RobotPool {
 public:
  explicit RobotPool(std::vector<std::string>& logs) : logs_(logs) {}

  void apply_time(uint32_t delta_seconds) {
    if (delta_seconds == 0) {
      return;
    }
    std::vector<std::shared_ptr<Robot>> alive;
    alive.reserve(normal_.size());
    for (const auto& robot : normal_) {
      robot->tick_seconds(delta_seconds);
      if (robot->hp() == 0) {
        destroyed_.push_back(robot);
        print_destroyed(*robot);
      } else {
        alive.push_back(robot);
      }
    }
    normal_.swap(alive);
  }

  std::shared_ptr<Robot> find_normal(uint16_t team, uint16_t id) {
    return find_by(normal_, team, id);
  }

  std::shared_ptr<Robot> find_destroyed(uint16_t team, uint16_t id) {
    return find_by(destroyed_, team, id);
  }

  void add_normal(const std::shared_ptr<Robot>& robot) {
    normal_.push_back(robot);
  }

  void move_to_destroyed(const std::shared_ptr<Robot>& robot) {
    remove_by(normal_, robot);
    destroyed_.push_back(robot);
    print_destroyed(*robot);
  }

  void revive_to_normal(const std::shared_ptr<Robot>& robot) {
    remove_by(destroyed_, robot);
    normal_.push_back(robot);
  }

 private:
  static std::shared_ptr<Robot> find_by(
      const std::vector<std::shared_ptr<Robot>>& list,
      uint16_t team,
      uint16_t id) {
    for (const auto& robot : list) {
      if (robot->team() == team && robot->id() == id) {
        return robot;
      }
    }
    return nullptr;
  }

  static void remove_by(std::vector<std::shared_ptr<Robot>>& list,
                        const std::shared_ptr<Robot>& robot) {
    std::vector<std::shared_ptr<Robot>> kept;
    kept.reserve(list.size());
    for (const auto& item : list) {
      if (item != robot) {
        kept.push_back(item);
      }
    }
    list.swap(kept);
  }

  void print_destroyed(const Robot& robot) {
    logs_.push_back("D " + std::to_string(robot.team()) + " " +
                    std::to_string(robot.id()));
  }

  std::vector<std::shared_ptr<Robot>> normal_;
  std::vector<std::shared_ptr<Robot>> destroyed_;
  std::vector<std::string>& logs_;
};

static uint16_t clamp_u16(uint32_t value) {
  return static_cast<uint16_t>(value > 65535 ? 65535 : value);
}

static std::shared_ptr<Robot> create_robot(uint16_t team,
                                           uint16_t id,
                                           uint8_t type,
                                           uint16_t hp,
                                           uint16_t heat,
                                           uint16_t hp_limit,
                                           uint16_t heat_limit,
                                           uint16_t level) {
  if (type == kEngineerType) {
    return std::make_shared<EngineerRobot>(team, id, type, hp, heat, hp_limit,
                                           heat_limit, level);
  }
  return std::make_shared<InfantryRobot>(team, id, type, hp, heat, hp_limit,
                                         heat_limit, level);
}

int main() {
  std::vector<std::string> logs;
  RobotPool pool(logs);
  uint32_t last_time = 0;
  bool has_time = false;

  uint32_t time_value = 0;
  char command = 0;
  uint32_t team = 0;
  uint32_t id = 0;
  int cnt = 0;
  std::cin >> cnt;
  std::string line;
  std::getline(std::cin, line);
  for (int i = 0; i < cnt; ++i) {
    std::getline(std::cin, line);
    if (line.empty()) {
      --i;
      continue;
    }
    std::istringstream iss(line);
    iss >> time_value >> command >> team >> id;
    uint32_t delta = 0;
    if (!has_time) {
      last_time = time_value;
      has_time = true;
    } else if (time_value > last_time) {
      delta = time_value - last_time;
      last_time = time_value;
    }
    pool.apply_time(delta);

    if (command == 'A') {
      uint32_t type = 0;
      uint32_t hp_limit = 0;
      uint32_t heat_limit = 0;
      iss >> type;
      hp_limit = (type == kEngineerType) ? 300 : 100;
      heat_limit = 0;

      auto destroyed = pool.find_destroyed(clamp_u16(team), clamp_u16(id));
      if (destroyed && destroyed->type() == static_cast<uint8_t>(type)) {
        destroyed->set_hp(destroyed->hp_limit());
        destroyed->set_heat(0);
        pool.revive_to_normal(destroyed);
        continue;
      }

      auto normal = pool.find_normal(clamp_u16(team), clamp_u16(id));
      if (normal) {
        continue;
      }

      uint16_t hp_limit_value = clamp_u16(hp_limit);
      uint16_t heat_limit_value = clamp_u16(heat_limit);
      auto robot = create_robot(clamp_u16(team), clamp_u16(id),static_cast<uint8_t>(type), hp_limit_value, 0,hp_limit_value, heat_limit_value, 1);
      pool.add_normal(robot);
      continue;
    }

    if (command == 'F') {
      uint32_t hp_value = 0;
      iss >> hp_value;
      auto normal = pool.find_normal(clamp_u16(team), clamp_u16(id));
      if (!normal) {
        continue;
      }
      uint16_t current_hp = normal->hp();
      uint16_t damage = clamp_u16(hp_value);
      normal->set_hp(static_cast<uint16_t>(current_hp > damage ?current_hp - damage :0));
      if (normal->hp() == 0) {
        pool.move_to_destroyed(normal);
      }
      continue;
    }

    if (command == 'H') {
      uint32_t heat_value = 0;
      iss >> heat_value;
      auto normal = pool.find_normal(clamp_u16(team), clamp_u16(id));
      if (!normal) {
        continue;
      }
      if (normal->type() != kInfantryType) {
        continue;
      }
      normal->set_heat(clamp_u16(heat_value));
      continue;
    }

    if (command == 'U') {
      uint32_t target_level = 0;
      uint32_t new_hp_limit = 0;
      uint32_t new_heat_limit = 0;
      iss >> target_level;
      auto normal = pool.find_normal(clamp_u16(team), clamp_u16(id));
      if (!normal) {
        continue;
      }
      if (normal->type() != kInfantryType) {
        continue;
      }
      if (target_level <= normal->level()) {
        continue;
      }
      new_hp_limit = normal->hp_limit();
      new_heat_limit = normal->heat_limit();
      normal->set_level(clamp_u16(target_level));
      normal->set_hp_limit(clamp_u16(new_hp_limit));
      normal->set_heat_limit(clamp_u16(new_heat_limit));
      normal->set_hp(normal->hp_limit());
      continue;
    }
  }

  for (size_t i = 0; i < logs.size(); ++i) {
    std::cout << logs[i] << std::endl;
  }

  return 0;
}