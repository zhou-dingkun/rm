#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/int64.hpp"

#include "rm_server/msg/get_el_gamal_params.hpp"
#include "rm_server/srv/el_gamal_encrypt.hpp"

using namespace std::chrono_literals;

class client : public rclcpp::Node {
public:
	client()
			: Node("elgamal_client"), private_key_(12345ULL), waiting_param_(true),
				finished_count_(0) {
		params_sub_ = this->create_subscription<rm_server::msg::GetElGamalParams>(
				"elgamal_params", 10,
				std::bind(&client::on_params, this, std::placeholders::_1));

		result_pub_ = this->create_publisher<std_msgs::msg::Int64>("elgamal_result", 10);

		encrypt_client_ = this->create_client<rm_server::srv::ElGamalEncrypt>("elgamal_service");

		RCLCPP_INFO(this->get_logger(), "ElGamal client started, private key = %llu",
								static_cast<unsigned long long>(private_key_));
	}

private:
	static uint64_t mod_pow(uint64_t base, uint64_t exp, uint64_t mod) {
		uint64_t result = 1ULL;
		base %= mod;
		while (exp > 0) {
			if (exp & 1ULL) {
				result = static_cast<uint64_t>((__uint128_t)result * base % mod);
			}
			base = static_cast<uint64_t>((__uint128_t)base * base % mod);
			exp >>= 1ULL;
		}
		return result;
	}

	static uint64_t mod_inv_prime(uint64_t x, uint64_t p) {
		return mod_pow(x, p - 2ULL, p);
	}

	void on_params(const rm_server::msg::GetElGamalParams::SharedPtr msg) {
		if (!waiting_param_ || finished_count_ >= 5) {
			return;
		}

		current_p_ = msg->p;
		current_a_ = msg->a;

		const uint64_t public_key_b = mod_pow(current_a_, private_key_, current_p_);
		waiting_param_ = false;

		RCLCPP_INFO(this->get_logger(), "recv p=%llu, a=%llu, send b=%llu",
								static_cast<unsigned long long>(current_p_),
								static_cast<unsigned long long>(current_a_),
								static_cast<unsigned long long>(public_key_b));

		if (!encrypt_client_->wait_for_service(2s)) {
			RCLCPP_WARN(this->get_logger(), "service error");
			waiting_param_ = true;
			return;
		}

		auto request = std::make_shared<rm_server::srv::ElGamalEncrypt::Request>();
		request->public_key = public_key_b;

		auto future = encrypt_client_->async_send_request(
				request,
				std::bind(&client::on_encrypt_response, this, std::placeholders::_1));
		(void)future;
	}

	void on_encrypt_response(
			rclcpp::Client<rm_server::srv::ElGamalEncrypt>::SharedFuture future) {
		auto response = future.get();
		const uint64_t y1 = response->y1;
		const uint64_t y2 = response->y2;

		const uint64_t s = mod_pow(y1, private_key_, current_p_);
		const uint64_t s_inv = mod_inv_prime(s, current_p_);
		const uint64_t plain = static_cast<uint64_t>((__uint128_t)y2 * s_inv % current_p_);

		std_msgs::msg::Int64 out;
		out.data = static_cast<int64_t>(plain);
		result_pub_->publish(out);

		++finished_count_;
		RCLCPP_INFO(this->get_logger(),
								"recv y1=%llu, y2=%llu => plain=%lld (round %d/5)",
								static_cast<unsigned long long>(y1),
								static_cast<unsigned long long>(y2),
								static_cast<long long>(out.data), finished_count_);

		if (finished_count_ >= 5) {
			RCLCPP_INFO(this->get_logger(), "finished 5 rounds, shutting down");
			rclcpp::shutdown();
			return;
		}

		waiting_param_ = true;
	}

private:
	rclcpp::Subscription<rm_server::msg::GetElGamalParams>::SharedPtr params_sub_;
	rclcpp::Publisher<std_msgs::msg::Int64>::SharedPtr result_pub_;
	rclcpp::Client<rm_server::srv::ElGamalEncrypt>::SharedPtr encrypt_client_;

	uint64_t private_key_;
	uint64_t current_p_{};
	uint64_t current_a_{};

	bool waiting_param_;
	int finished_count_;
};

int main(int argc, char **argv) {
	rclcpp::init(argc, argv);
	auto node = std::make_shared<client>();
	rclcpp::spin(node);
	rclcpp::shutdown();
	return 0;
}
