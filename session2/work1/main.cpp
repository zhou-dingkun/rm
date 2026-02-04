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
		  uint16_t hpLimit,
		  uint16_t heatLimit,
		  uint16_t level)
		: team_(team),
		  id_(id),
		  type_(type),
		  hp_(hp),
		  heat_(heat),
		  hpLimit_(hpLimit),
		  heatLimit_(heatLimit),
		  level_(level) {}

	virtual ~Robot() = default;

	uint16_t team() const { return team_; }
	uint16_t id() const { return id_; }
	uint8_t type() const { return type_; }
	uint16_t hp() const { return hp_; }
	uint16_t heat() const { return heat_; }
	uint16_t hpLimit() const { return hpLimit_; }
	uint16_t heatLimit() const { return heatLimit_; }
	uint16_t level() const { return level_; }

	void setHp(uint16_t hp) { hp_ = hp; }
	void setHeat(uint16_t heat) { heat_ = heat; }
	void setHpLimit(uint16_t hpLimit) { hpLimit_ = hpLimit; }
	void setHeatLimit(uint16_t heatLimit) { heatLimit_ = heatLimit; }
	void setLevel(uint16_t level) { level_ = level; }

	void tickSeconds(uint32_t seconds) {
		if (seconds == 0) {
			return;
		}

		uint32_t heatValue = heat_;
		uint32_t limit = heatLimit_;
		uint32_t damage = 0;

		if (heatValue > limit + 1) {
			uint32_t maxOverSeconds = heatValue - limit - 1;
			damage = (seconds < maxOverSeconds) ? seconds : maxOverSeconds;
		}

		uint32_t newHeat = (heatValue > seconds) ? (heatValue - seconds) : 0;
		uint32_t newHp = (hp_ > damage) ? (hp_ - damage) : 0;

		heat_ = static_cast<uint16_t>(newHeat);
		hp_ = static_cast<uint16_t>(newHp);
	}

	virtual std::string name() const = 0;

private:
	uint16_t team_;
	uint16_t id_;
	uint8_t type_;
	uint16_t hp_;
	uint16_t heat_;
	uint16_t hpLimit_;
	uint16_t heatLimit_;
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

	void applyTime(uint32_t deltaSeconds) {
		if (deltaSeconds == 0) {
			return;
		}
		std::vector<std::shared_ptr<Robot>> alive;
		alive.reserve(normal_.size());
		for (const auto& robot : normal_) {
			robot->tickSeconds(deltaSeconds);
			if (robot->hp() == 0) {
				destroyed_.push_back(robot);
				printDestroyed(*robot);
			} else {
				alive.push_back(robot);
			}
		}
		normal_.swap(alive);
	}

	std::shared_ptr<Robot> findNormal(uint16_t team, uint16_t id) {
		return findBy(normal_, team, id);
	}

	std::shared_ptr<Robot> findDestroyed(uint16_t team, uint16_t id) {
		return findBy(destroyed_, team, id);
	}

	void addNormal(const std::shared_ptr<Robot>& robot) {
		normal_.push_back(robot);
	}

	void moveToDestroyed(const std::shared_ptr<Robot>& robot) {
		removeBy(normal_, robot);
		destroyed_.push_back(robot);
		printDestroyed(*robot);
	}

	void reviveToNormal(const std::shared_ptr<Robot>& robot) {
		removeBy(destroyed_, robot);
		normal_.push_back(robot);
	}

private:
	static std::shared_ptr<Robot> findBy(
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

	static void removeBy(std::vector<std::shared_ptr<Robot>>& list,
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

	void printDestroyed(const Robot& robot) {
		logs_.push_back("D " + std::to_string(robot.team()) + " " + std::to_string(robot.id()));
	}

	std::vector<std::shared_ptr<Robot>> normal_;
	std::vector<std::shared_ptr<Robot>> destroyed_;
	std::vector<std::string>& logs_;
};

static uint16_t clampU16(uint32_t value) {
	return static_cast<uint16_t>(value > 65535 ? 65535 : value);
}

static std::shared_ptr<Robot> createRobot(uint16_t team,
						   uint16_t id,
						   uint8_t type,
						   uint16_t hp,
						   uint16_t heat,
						   uint16_t hpLimit,
						   uint16_t heatLimit,
						   uint16_t level) {
	if (type == kEngineerType) {
		return std::make_shared<EngineerRobot>(team, id, type, hp, heat, hpLimit, heatLimit, level);
	}
	return std::make_shared<InfantryRobot>(team, id, type, hp, heat, hpLimit, heatLimit, level);
}

int main() {
	std::vector<std::string> logs;
	RobotPool pool(logs);
	uint32_t lastTime = 0;
	bool hasTime = false;

	uint32_t timeValue = 0;
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
		iss >> timeValue >> command >> team >> id;
		uint32_t delta = 0;
		if (!hasTime) {
			lastTime = timeValue;
			hasTime = true;
		} else if (timeValue > lastTime) {
			delta = timeValue - lastTime;
			lastTime = timeValue;
		}
		pool.applyTime(delta);

		if (command == 'A') {
			uint32_t type = 0;
			uint32_t hpLimit = 0;
			uint32_t heatLimit = 0;
			iss >> type;
			hpLimit = (type == kEngineerType) ? 300 : 100;
			heatLimit = 0;
			iss >> hpLimit >> heatLimit;

			auto destroyed = pool.findDestroyed(clampU16(team), clampU16(id));
			if (destroyed && destroyed->type() == static_cast<uint8_t>(type)) {
				destroyed->setHp(destroyed->hpLimit());
				destroyed->setHeat(0);
				pool.reviveToNormal(destroyed);
				continue;
			}

			auto normal = pool.findNormal(clampU16(team), clampU16(id));
			if (normal) {
				continue;
			}

			uint16_t hpLimitValue = clampU16(hpLimit);
			uint16_t heatLimitValue = clampU16(heatLimit);
			auto robot = createRobot(
				clampU16(team),
				clampU16(id),
				static_cast<uint8_t>(type),
				hpLimitValue,
				0,
				hpLimitValue,
				heatLimitValue,
				1);
			pool.addNormal(robot);
			continue;
		}

		if (command == 'F') {
			uint32_t hpValue = 0;
			iss >> hpValue;
			auto normal = pool.findNormal(clampU16(team), clampU16(id));
			if (!normal) {
				continue;
			}
			uint16_t currentHp = normal->hp();
			uint16_t damage = clampU16(hpValue);
			normal->setHp(static_cast<uint16_t>(currentHp > damage ? currentHp - damage : 0));
			if (normal->hp() == 0) {
				pool.moveToDestroyed(normal);
			}
			continue;
		}

		if (command == 'H') {
			uint32_t heatValue = 0;
			iss >> heatValue;
			auto normal = pool.findNormal(clampU16(team), clampU16(id));
			if (!normal) {
				continue;
			}
			if (normal->type() != kInfantryType) {
				continue;
			}
			normal->setHeat(clampU16(heatValue));
			continue;
		}

		if (command == 'U') {
			uint32_t targetLevel = 0;
			uint32_t newHpLimit = 0;
			uint32_t newHeatLimit = 0;
			iss >> targetLevel;
			auto normal = pool.findNormal(clampU16(team), clampU16(id));
			if (!normal) {
				continue;
			}
			if (normal->type() != kInfantryType) {
				continue;
			}
			if (targetLevel <= normal->level()) {
				continue;
			}
			newHpLimit = normal->hpLimit();
			newHeatLimit = normal->heatLimit();
			iss >> newHpLimit >> newHeatLimit;
			normal->setLevel(clampU16(targetLevel));
			normal->setHpLimit(clampU16(newHpLimit));
			normal->setHeatLimit(clampU16(newHeatLimit));
			normal->setHp(normal->hpLimit());
			continue;
		}
	}

	for (const auto& entry : logs) {
		std::cout << entry << std::endl;
	}

	return 0;
}