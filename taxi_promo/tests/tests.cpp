#include <iostream>

#include "../taxi_promo.h"

#include <gtest/gtest.h>
#include <mongo/client/dbclient.h>

using namespace std;
using namespace taxi;

class TaxiPromoTest : public ::testing::Test
{

public:
	void SetUp()
	{
		mongo::client::initialize();
		system("mongo taxi preparedb.js");
	}

	void TearDown()
	{
		
	}
};

static const string user1("U1");
static const string user2("U2");
static const string user3("U3");
static const string promoCode1("8Ies38s1");
static const string promoCode2("3xfde2R4");

TEST_F(TaxiPromoTest, SimpleCreateAndFinishOrder) 
{
	string orderId;
	ASSERT_TRUE(SUCCEEDED(CreateOrder(user1, promoCode1, orderId)));
    ASSERT_FALSE(orderId.empty());
    ASSERT_TRUE(SUCCEEDED(UpdateOrder(orderId, Success)));
}

void MakeRideForUser(const string& userName, const string& promoCode)
{
	string orderId;
	ASSERT_TRUE(SUCCEEDED(CreateOrder(userName, promoCode, orderId)));
	ASSERT_FALSE(orderId.empty());
	ASSERT_TRUE(SUCCEEDED(UpdateOrder(orderId, Success)));	
}

TEST_F(TaxiPromoTest, CheckRideLimitsForK)
{
	for (uint16_t K = 0; K < 3; ++K)
		MakeRideForUser(user1, promoCode2);

	string orderId;
	ASSERT_TRUE(CreateOrder(user1, promoCode2, orderId) == result_code::eNoRides);
}

TEST_F(TaxiPromoTest, CheckRideLimitsForN)
{
	for (uint16_t K = 0; K < 3; ++K)
		MakeRideForUser(user1, promoCode2);

	for (uint16_t K = 0; K < 3; ++K)
		MakeRideForUser(user2, promoCode2);

	MakeRideForUser(user3, promoCode2);
	string orderId;
	ASSERT_TRUE(CreateOrder(user3, promoCode2, orderId) == result_code::eNoRides);
}

TEST_F(TaxiPromoTest, GetNotExistingOrder)
{
	string user, promo;
	int state;
	ASSERT_TRUE(FAILED(GetOrderInfo("SomeNotExistingOrder", user, promo, state)));
}

TEST_F(TaxiPromoTest, CreateSecondOrderByOnePromoCode) 
{
	string orderId;
	ASSERT_TRUE(SUCCEEDED(CreateOrder(user1, promoCode1, orderId)));
    ASSERT_FALSE(orderId.empty());
    ASSERT_TRUE(CreateOrder(user2, promoCode1, orderId) == result_code::ePromoCodeLocked);
    ASSERT_TRUE(orderId.empty());
}

TEST_F(TaxiPromoTest, CancelOrder)
{
	string orderId;
	ASSERT_TRUE(SUCCEEDED(CreateOrder(user1, promoCode1, orderId)));
    ASSERT_FALSE(orderId.empty());
    ASSERT_TRUE(SUCCEEDED(UpdateOrder(orderId, Cancel)));
    string user, promo;
	int state;
    ASSERT_TRUE(FAILED(GetOrderInfo(orderId, user, promo, state)));
}

TEST_F(TaxiPromoTest, CancelOrderAndThenRideAgain)
{
	string orderId;
	ASSERT_TRUE(SUCCEEDED(CreateOrder(user1, promoCode1, orderId)));
    ASSERT_FALSE(orderId.empty());

    ASSERT_TRUE(SUCCEEDED(UpdateOrder(orderId, Cancel)));
    string user, promo;
	int state;
    ASSERT_TRUE(FAILED(GetOrderInfo(orderId, user, promo, state)));

    ASSERT_TRUE(SUCCEEDED(CreateOrder(user2, promoCode1, orderId)));
    ASSERT_FALSE(orderId.empty());
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}