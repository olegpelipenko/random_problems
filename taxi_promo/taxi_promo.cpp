#include <mongo/client/dbclient.h>
#include <mongo/bson/oid.h> 
#include <boost/date_time/posix_time/posix_time.hpp>

#include <iostream>
#include <mutex>

#include "named_mutex.h"
#include "taxi_promo.h"
#include "common.h"
#include "mongo.h"
#include "structs.h"

using mongo::BSONElement;
using mongo::BSONObj;
using mongo::BSONObjBuilder;

using namespace result_code;
using namespace boost::posix_time;

namespace taxi
{

NamedMutexArray g_promoCodesMxs;
NamedMutexArray g_ordersMxs;

int64_t GetCurrentDT()
{
	ptime const time_epoch(boost::gregorian::date(1970, 1, 1));
	return (microsec_clock::local_time() - time_epoch).total_milliseconds();
}

result CheckAndUpdateOrder(Order& order, bool& orderExists, bool wasOrderRemoved)
{
	orderExists = false;
	wasOrderRemoved = false;
	
	if (order.state == Order::Pending)
	{
		const auto nowPlusPendingDelay = (GetCurrentDT() + DELAY_FOR_PENDING_TRANSACTIONS);
		if (order.last_modified.asInt64() > nowPlusPendingDelay)
		{
			// Order hangs, we have to remove it
			CHECK_AND_RETURN(order.Remove());
			wasOrderRemoved = true;
		}
		else
		{
			orderExists = true;
		}
	}
	else if(order.state == Order::Started)
	{
		// Probably order hangs, cleanup
		const auto nowPlusUncompletedDelay = (GetCurrentDT() + DELAY_FOR_UNCOMPLETED_TRANSACTIONS);
		if (order.last_modified.asInt64() > nowPlusUncompletedDelay)
		{
			CHECK_AND_RETURN(order.Remove());
			wasOrderRemoved = true;
		}
		else
		{
			orderExists = true;
			return result_code::sOk;
		}
	}

	orderExists = false;
	return result_code::sOk;
}

result CreateOrder(const std::string& user_id, 
				   const std::string& promo_code, 
				   std::string& order_id)
{
    try 
    {
		if (user_id.empty() || promo_code.empty())
			return result_code::eWrongInput;

		order_id.clear();
		bool firstRideByThisUser(false);

		std::shared_ptr<MongoConnection> mongo = std::make_shared<MongoConnection>();
		PromoCode promoCode(mongo);
		User 	  user(mongo);
		Order 	  order(mongo);

		{
			AutoMutex mutex(g_promoCodesMxs, promo_code);
			promoCode.FindByName(promo_code);

			// Probably there is an order
			if (promoCode.order_id.isSet())
			{
				CHECK_AND_RETURN(order.FindById(promoCode.order_id));
				bool orderExists(false), wasOrderRemoved(false);

				CHECK_AND_RETURN(CheckAndUpdateOrder(order, orderExists, wasOrderRemoved));
				
				if (orderExists)
		        	return result_code::ePromoCodeLocked;

				if (wasOrderRemoved)
					CHECK_AND_RETURN(promoCode.Update(BSON("$unset" << BSON("order_id" << mongo::OID(order_id)))));	
			}

			if (promoCode.name.empty())
				return result_code::eNotFound; 

			if (promoCode.N <= 0)
		        return result_code::eNoRides; 

		    CHECK_AND_RETURN(user.FindByName(user_id));
		    const auto it = user.FindPromoCode(promo_code);
			if (it != user.activePCs.end())
			{
				if (it->K <= 0)
					return result_code::eNoRides;
			}
			else
			{
				firstRideByThisUser = true;
			}
			
			CHECK_AND_RETURN(order.Insert(BSON(mongo::GENOID << "user_id" << user_id << "state" << "Pending" 
                                                             << "promo_code_name" << promo_code 
                                                             << "last_modified" <<  mongo::Date_t(GetCurrentDT()))));

			CHECK_AND_RETURN(promoCode.Update(BSON("$set" << BSON("order_id" << order._id))));
		}
		
		if (firstRideByThisUser)
		{
			user.name = user_id;
			CHECK_AND_RETURN(user.Update(BSON("$push" << BSON("active_promo_codes" 
													  << BSON("promo_code_name" << promo_code 
													  << "K" << promoCode.maxK)))));
		}

		{
			AutoMutex mutex(g_ordersMxs, order_id);
			CHECK_AND_RETURN(order.Update(BSON("$set" << BSON("state" << "Started"))));
		}

		order_id = order._id.toString();
		return result_code::sOk;
	} 
	catch( const mongo::DBException &e ) 
	{
        std::cout << "CreateOrder error: " << e.what() << std::endl;
    }

    return result_code::eFailed;
}

result GetOrderInfo(const std::string& order_id, 
					std::string& promo_code,
					std::string& user_name,
					int& order_state)
{
	if (order_id.empty())
		return result_code::eWrongInput;

	try
	{
		Order order(order_id, std::make_shared<MongoConnection>());
		if (!order._id.isSet())
			return result_code::eNotFound;

		promo_code = order.promoCodeName;
		user_name = order.userId;
		order_state = order.state;
		return result_code::sOk;
	}
	catch( const mongo::DBException &e ) 
	{
        std::cout << "GetOrderInfo error: " << e.what() << std::endl;
    }

    return result_code::eFailed;
}

result UpdateOrder(const std::string& order_id, Completion completion)
{
	try
	{
		if (order_id.empty())
			return result_code::eWrongInput;
		
		if (completion == Cancel)
		{
			AutoMutex mutex(g_ordersMxs, order_id);

			std::shared_ptr<MongoConnection> mongo = std::make_shared<MongoConnection>();
			Order 	  order(order_id, mongo);
			PromoCode promoCode(order.promoCodeName, mongo, false);

			CHECK_AND_RETURN(promoCode.Update(BSON("$unset" << BSON("order_id" << mongo::OID(order_id)))));
			CHECK_AND_RETURN(order.Remove());

			return result_code::sOk;
		}
		else if (completion == Success)
		{
			AutoMutex mutex(g_ordersMxs, order_id);

			std::shared_ptr<MongoConnection> mongo = std::make_shared<MongoConnection>();
			Order 	  order(order_id, mongo);
			PromoCode promoCode(order.promoCodeName, mongo, false);
			User 	  user(mongo);

			CHECK_AND_RETURN(promoCode.Update(BSON("$inc" << BSON("N" << -1) << "$unset" << BSON("order_id" << mongo::OID(order_id)))));

			CHECK_AND_RETURN(user.Update(BSON("name" << order.userId << "active_promo_codes.promo_code_name" << order.promoCodeName),
										 BSON("$inc" << BSON("active_promo_codes.$.K" << -1))));

			CHECK_AND_RETURN(order.Update(BSON( "$set" << BSON("state" << "Completed"))));
			
			return result_code::sOk;
		}
		else
		{
			return result_code::eWrongInput;
		}
	}
	catch( const mongo::DBException &e ) 
	{
        std::cout << "UpdateOrder error: " << e.what() << std::endl;
    }
    
    return result_code::eFailed;
}

} // namespace taxi