#ifndef MONGOCONNECTION_H
#define MONGOCONNECTION_H

#include "common.h"
#include <string>
#include <memory>
#include <iostream>
#include <mongo/client/dbclientcursor.h>

namespace taxi
{

class MongoConnection
{
public:
	MongoConnection() : m_dbName(MONGO_DB_NAME)
	{
        m_connection = std::make_shared<mongo::DBClientConnection>();
  		m_connection->connect(MONGO_DB_CONNECTION_STRING);
	}

	template <class T>
	result FindFirst(const std::string& collection,
							 const mongo::BSONObj& query, 
							 T& out)
	{
		std::shared_ptr<mongo::DBClientCursor> cursor(m_connection->query(GetCollectionName(collection).c_str(), query));
		bool found(false);
		if (cursor->more())
		{
			found = true;
			out.Fill(cursor->next());
		}
		
		return found ? result_code::sOk : result_code::sFalse;
	}

	result Update(const std::string& collection, 
						  const mongo::BSONObj& query, 
						  const mongo::BSONObj& toUpdate)
	{
		m_connection->update(GetCollectionName(collection).c_str(), query, toUpdate);
		// TODO: Need error check
		return result_code::sOk;
	}

	template <class T>
	result Insert(const std::string& collection, const mongo::BSONObj& toInsert, T& out)
	{
		m_connection->insert(GetCollectionName(collection).c_str(), toInsert);
		// TODO: Need error check
		out.Fill(toInsert);
		return result_code::sOk;
	}

	result Remove(const std::string& collection, const mongo::BSONObj& toRemove)
	{
		// isConnected?!??!
		m_connection->remove(GetCollectionName(collection).c_str(), toRemove);
		return toRemove.hasField("_id") ? result_code::sOk : result_code::eFailed;
	}

private:

	inline std::string GetCollectionName(const std::string& collection) const
	{
		return std::string(m_dbName).append(".").append(collection);
	}

	const std::string m_dbName;
	std::shared_ptr<mongo::DBClientConnection> m_connection;
};

} // taxi

#endif // MONGOCONNECTION_H

