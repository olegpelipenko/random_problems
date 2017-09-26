#ifndef TAXI_PROMO_H
#define TAXI_PROMO_H

#include "common.h"
#include <string>

enum Completion
{
	Cancel,
	Success,
};

using namespace result_code;

namespace taxi
{

result CreateOrder(const std::string& user_id, 
				   const std::string& promo_code, 
				   std::string& order_id);

result GetOrderInfo(const std::string& order_id, 
					std::string& promo_code,
					std::string& user_name,
					int& order_state);

result UpdateOrder(const std::string& order_id, Completion completion);

} // taxi

#endif // TAXI_PROMO_H