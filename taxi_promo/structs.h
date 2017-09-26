#ifndef STRUCTS_H
#define STRUCTS_H

#include <string>
#include <mongo/client/dbclientcursor.h>
#include <mongo/client/dbclient.h>
#include "mongo.h"

namespace taxi
{

class BsonObjBase
{
public:

	BsonObjBase() : m_valid(false) {}
    BsonObjBase(std::shared_ptr<MongoConnection> mongo) : m_mongo(mongo), m_valid(false) {}	

	bool IsValid()
    {
    	return m_valid;
    }
 	
 	mongo::OID _id;

protected:

    void Fill(const mongo::BSONObj& obj)
    {
        if (!obj.valid()) 
            throw std::runtime_error(obj.toString().c_str());
        _id = obj["_id"].OID();
    }

	bool 	m_valid;
    std::shared_ptr<MongoConnection> m_mongo;
};

struct PromoCode : public BsonObjBase
{
    std::string   name;
    mongo::OID    order_id;
    uint32_t 	  N;
    uint32_t 	  maxK;

    PromoCode() : BsonObjBase() {}
    PromoCode(std::shared_ptr<MongoConnection> mongo) : BsonObjBase(mongo) {}
    PromoCode(const std::string& name_, std::shared_ptr<MongoConnection> mongo, bool find = true) : BsonObjBase(mongo)
    {
        this->name = name_;
        if (find)
            FindByName(name);
    }

    void Fill(const mongo::BSONObj& obj)
    {
    	try
    	{
            BsonObjBase::Fill(obj);
    		name = obj["name"].String();
    		N = obj["N"].Number();
    		
    		if (obj.hasField("order_id") )
    			order_id = obj["order_id"].OID();

    		maxK = obj["maxK"].Number();

    		m_valid = true;
    	}
    	catch(std::exception& e)
    	{
    		std::cout << "PromoCode deserialize error: " << e.what() << std::endl;
    		m_valid = false;
    	}
    }

    result FindByName(const std::string& name)
    {
        return m_mongo->FindFirst(PROMO_CODES_COLLECTION, BSON("name" << name), *this);
    }

    result Update(const mongo::BSONObj& query)
    {
        return m_mongo->Update(PROMO_CODES_COLLECTION, BSON("name" << this->name), query);
    }
};

struct User : public BsonObjBase
{
	User() : BsonObjBase() {}
    User(std::shared_ptr<MongoConnection> mongo) : BsonObjBase(mongo) {}
    User(const std::string& name_, std::shared_ptr<MongoConnection> mongo, bool find = true) : BsonObjBase(mongo) 
    {
        this->name = name_;
        if (find)
            FindByName(this->name);
    }

    result FindByName(const std::string& name)
    {
        return m_mongo->FindFirst(USERS_COLLECTION, BSON("name" << name), *this);
    }

    void Fill(const mongo::BSONObj& obj)
    {
		try
    	{
            BsonObjBase::Fill(obj);
    		const auto& activePCsObj = obj.getObjectField("active_promo_codes");
    		for (int i = 0; i < activePCsObj.nFields(); ++i)
    		{
    			const auto& current = activePCsObj[i];
    			activePCs.push_back(ActivePromoCode(current["promo_code_name"].String(), current["K"].Number()));
    		}
    		m_valid = true;
    	}
    	catch(std::exception& e)
    	{
    		std::cout << "User deserialize error: " << e.what() << std::endl;
    		m_valid = false;
    	}
    }

    struct ActivePromoCode
    {
    	std::string  promoCodeName;
    	uint32_t 	 K;
    	ActivePromoCode(const std::string& name, uint32_t K_) : promoCodeName(name), K(K_) {}
    };

    typedef std::vector<ActivePromoCode> ArrayOfPromoCodes;
    ArrayOfPromoCodes activePCs;

    ArrayOfPromoCodes::iterator FindPromoCode(const std::string& promo_code)
    {
    	return std::find_if(activePCs.begin(), 
    						activePCs.end(), 
    				 		[promo_code](const User::ActivePromoCode& code) 
		    				 { 
		    				 	return code.promoCodeName == promo_code; 
		    				 });
    }

    result Update(const mongo::BSONObj& query)
    {
        return m_mongo->Update(USERS_COLLECTION, BSON("name"  << this->name), query);
    }

    result Update(const mongo::BSONObj& query, const mongo::BSONObj& toUpdate)
    {
        return m_mongo->Update(USERS_COLLECTION, query, toUpdate);
    }
    
    std::string name;
private:

};

struct Order : public BsonObjBase
{
	enum State
	{
		Pending = 0,
		Started,
		Completed,
		Cancelled
	};

	Order(std::shared_ptr<MongoConnection> mongo) : BsonObjBase(mongo) {}
    Order() : BsonObjBase() {}
	Order(const std::string& _id, std::shared_ptr<MongoConnection> mongo, bool find = true) : BsonObjBase(mongo)
	{
        this->_id = mongo::OID(_id);
        if (find)
		  FindById(this->_id);
	}

    void Fill(const mongo::BSONObj& obj)
    {
		try
    	{
            BsonObjBase::Fill(obj);
    		const auto& stateStr = obj["state"].String();
    		if (stateStr == "Started")
    			state = Started;
    		else if (stateStr == "Completed")
    			state = Completed;
    		else if (stateStr == "Cancelled")
    			state = Cancelled;
    		else if (stateStr == "Pending")
    			state = Pending;
    		else
    			throw std::runtime_error(std::string("Current order state is not defined:").append(stateStr));
    		userId = obj["user_id"].String();
    		promoCodeName = obj["promo_code_name"].String();
    		if (obj.hasField("last_modified"))
    			last_modified = obj["last_modified"].Date();

            m_valid = true;
    	}
    	catch(std::exception& e)
    	{
    		std::cout << "Order deserialize error: " << e.what() << std::endl;
    		m_valid = false;
    	}
    }

    result FindById(const mongo::OID& _id)
    {
        result r = m_mongo->FindFirst(ORDERS_COLLECTION, BSON( "_id" << _id), *this);
        if (FAILED(r))
            this->_id.clear();
        return r;
    }

    result Remove()
    {
        result r = m_mongo->Remove(ORDERS_COLLECTION, BSON("_id" << _id));
        if (SUCCEEDED(r))
            m_valid = false;
        return r;
    }

    result Insert(const mongo::BSONObj& query)
    {
        return m_mongo->Insert(ORDERS_COLLECTION, query, *this);
    }

    static result Remove(std::shared_ptr<MongoConnection> mongo, const std::string& order_id)
    {
        return mongo->Remove(ORDERS_COLLECTION, BSON("_id" << mongo::OID(order_id)));
    }

    result Update(const mongo::BSONObj& query)
    {
        return m_mongo->Update(ORDERS_COLLECTION, BSON("_id" << this->_id), query);
    }
    
	State		  state;
	std::string   userId;
	std::string	  promoCodeName;
    mongo::Date_t last_modified;
};

} // namespace taxi

#endif // STRUCTS_H