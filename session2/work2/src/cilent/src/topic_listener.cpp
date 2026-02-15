#include <chrono>
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
			: Node("elgamal_client"), private_key_(12345LL), waiting_param_(true),
				finished_count_(0) {
		params_sub_ = this->create_subscription<rm_server::msg::GetElGamalParams>(
				"elgamal_params", 10,
				std::bind(&client::on_params, this, std::placeholders::_1));

		result_pub_ = this->create_publisher<std_msgs::msg::Int64>("elgamal_result", 10);

		encrypt_client_ = this->create_client<rm_server::srv::ElGamalEncrypt>("elgamal_service");

		RCLCPP_INFO(this->get_logger(), "client started, private key = %lld",
							private_key_);
	}

private:
	static long long int mod_pow(long long int base, long long int exp, long long int mod) {
		long long int result = 1LL;
		base %= mod;
		while (exp > 0) {
			if (exp & 1LL) {
				result = static_cast<long long int>((__int128_t)result * base % mod);
			}
			base = static_cast<long long int>((__int128_t)base * base % mod);
			exp >>= 1LL;
		}
		return result;
	}

	static long long int mod_inv_prime(long long int x, long long int p) {
		return mod_pow(x, p - 2LL, p);
	}

	void on_params(const rm_server::msg::GetElGamalParams::SharedPtr msg) {
		if (!waiting_param_ || finished_count_ >= 5) {
			return;
		}

		current_p_ = msg->p;
		current_a_ = msg->a;

		const long long int public_key_b = mod_pow(current_a_, private_key_, current_p_);
		waiting_param_ = false;

		RCLCPP_INFO(this->get_logger(), "receive p=%lld, a=%lld, send b=%lld",
							current_p_,
							current_a_,
							public_key_b);

		if (!encrypt_client_->wait_for_service(2s)) {
			RCLCPP_WARN(this->get_logger(), "service error");
			waiting_param_ = true;
			return;
		}

		auto request = std::make_shared<rm_server::srv::ElGamalEncrypt::Request>();
		request->public_key = static_cast<decltype(request->public_key)>(public_key_b);

		auto future = encrypt_client_->async_send_request(
				request,
				std::bind(&client::on_encrypt_response, this, std::placeholders::_1));
		(void)future;
	}

	void on_encrypt_response(
			rclcpp::Client<rm_server::srv::ElGamalEncrypt>::SharedFuture future) {
		auto response = future.get();
		const long long int y1 = static_cast<long long int>(response->y1);
		const long long int y2 = static_cast<long long int>(response->y2);

		const long long int s = mod_pow(y1, private_key_, current_p_);
		const long long int s_inv = mod_inv_prime(s, current_p_);
		const long long int plain = static_cast<long long int>((__int128_t)y2 * s_inv % current_p_);

		std_msgs::msg::Int64 out;
		out.data = static_cast<long long int>(plain);
		result_pub_->publish(out);

		++finished_count_;
		RCLCPP_INFO(this->get_logger(),
							"receive y1=%lld, y2=%lld => plain=%lld (round %lld/5)",
							y1,
							y2,
							static_cast<long long int>(out.data), finished_count_);

		if (finished_count_ >= 5) {
			RCLCPP_INFO(this->get_logger(), "finished");
			rclcpp::shutdown();
			return;
		}

		waiting_param_ = true;
	}

private:
	rclcpp::Subscription<rm_server::msg::GetElGamalParams>::SharedPtr params_sub_;
	rclcpp::Publisher<std_msgs::msg::Int64>::SharedPtr result_pub_;
	rclcpp::Client<rm_server::srv::ElGamalEncrypt>::SharedPtr encrypt_client_;

	long long int private_key_;
	long long int current_p_{};
	long long int current_a_{};

	bool waiting_param_;
	long long int finished_count_;
};

int main(int argc, char **argv) {
	rclcpp::init(argc, argv);
	auto node = std::make_shared<client>();
	rclcpp::spin(node);
	rclcpp::shutdown();
	return 0;
}
