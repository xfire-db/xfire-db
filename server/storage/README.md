Storage types
=============

XFire storage supports serval storage types:

	* string:string storage
	* string:list storage
	* string:hashmap storage

string:string storage
---------------------

string:string storage is the standard form of key-value storage. Each
stored value (a string) is identified by a unique key (string). An example:

	> STRING INSERT user:1234:password "p455w0rd"
	> STRING INSERT user:1234:email "email@email.com"
	> STRING GET user:1234:password
	(1) (p455w0rd)
	> STRING GET user:1234:email
	(1) (email@email.com)

string:list storage
-------------------

This is an alternative form of key-value storage. Again, each value is
identified by a unique key (a string). The value in this case is a list
(vector) of strings. An example usage of string:list storage:

	> LIST INSERT user:1234 "p455w0rd" "email@email.com"
	> LIST GET user:1234
	(1) (p455w0rd)
	(2) (email@email.com)

string:hashmap storage
----------------------

This is the last alternative of key-value storage. As you guessed, each
value is again identified by a unique string. Value's are then stored
in a hashmap. An example:

	> HMAP INSERT user:1234 password:"p455w0rd" email:"email@email.com"
	> HMAP GET user:1234
	(1) "password"
	(2) "p455w0rd"
	(3) "email"
	(4) "email@email.com"

