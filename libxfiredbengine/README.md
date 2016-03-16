XFireDB storage engine
======================

The storage engine is responsible for storing the data internally in a
proper manner. The backbone of the storage engine are the data storage models.

Storage models
--------------

There are several main storage models:

	* Dictionary's
	* Database's
	* Skiplists
	* Hashmaps
	* Sets
	* Lists
	* Strings

The dictionary storage container forms the base for XFireDB. The implementation of
the database container uses a dictionary as backend and the database data structure
is used as the main database for XFireDB.

Red-black tree's, hashmaps and sets are tied together aswell, as the hashmap and set
use a red-black tree as a backend. Even though the dictionary and red-black tree's are
both used for key-value storage, the first is optimized for speed where as the latter 
is optimized for memory usage.

The lists and strings are the most simple data structures. The lists are simple link lists
and strings are plain C-strings.

Disk persistency
----------------

Another responsibility of libxfiredbengine is making sure all data is backed up on disk. This
disk is read-out on start up and the configuration settings dictate how often data is stored to
the disk. The backend of the volatile storage is a standard SQL database: sqlite3.

Concurrency
-----------

Libxfiredbengine has its own system to deal with matters that have to be done concurrently: the
BIO subsystem (short for background I/O). This systems spawns threads as required and puts them to
sleep when they fall idle.

