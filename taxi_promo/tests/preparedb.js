db.dropDatabase()
db.createCollection("Orders")
db.createCollection("Users")
db.createCollection("PromoCodes")

db.Users.insert([
    { 'name' : 'U1'},
    {'name' : 'U2'},
    {'name' : 'U3'}
])

db.PromoCodes.insert(
[{
	'name': '3xfde2R4',
    'N' : 7,
    'maxK' : 3
},
{
	'name' : '8Ies38s1',
    'N' : 10,
    'maxK' : 3
}]
)