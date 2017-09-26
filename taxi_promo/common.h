#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

typedef uint32_t result;

namespace result_code
{
	enum ErrorCodes
	{
		sOk = 0,
		sFalse, 
		eFailed,
		eNotFound, 
		eNoRides, // No rides allowed for that user
		eWrongInput, // Wrong input data
		ePromoCodeLocked // Someone is using this promo code
	};
	
};

inline bool SUCCEEDED(result res)
{
	return (res == result_code::sOk);
}

inline bool FAILED(result res)
{
	return (res > result_code::sOk);
}

#define CHECK_AND_RETURN(result) if (FAILED(result)) return result;

#define CHECK_AND_THROW(result) if (FAILED(result)) throw std::runtime_error("Check failed!");

// Settings
namespace taxi
{

#define DELAY_FOR_PENDING_TRANSACTIONS 		60 * 1000
#define DELAY_FOR_UNCOMPLETED_TRANSACTIONS  4 * 60 * 60 * 1000
#define MONGO_DB_NAME 				"taxi"
#define MONGO_DB_CONNECTION_STRING  "localhost"
#define USERS_COLLECTION 			"Users"
#define PROMO_CODES_COLLECTION  	"PromoCodes"
#define ORDERS_COLLECTION 			"Orders"


} // namespace taxi

#endif /* COMMON_H */

