XFireDB README
=============

About
-----

XFireDB is a lightweight in memory storage system, a database. It supports various
data structures such as strings, lists, sets and hashes. All data is stored using
a key-value model. XFireDB supports clustering and various levels of on-disk
persistence.

License
-------

XFireDB is licenced as GNU General Public Licence version 3 or newer. 
The COPYING file should contian more information about the licence, or
if it's not included, look at http://www.gnu.org/licences/.

    XFIREDB
    Copyright (C) 2014, 2015, 2016

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

Dependancies
------------

The XFireDB currently depends on sqlite and openssl. Make sure that these
libraries are installed on your system (including the development files).

Compiling XFireDB
----------------

To retrieve and build XFireDB the following tools are required:

Retreiving:
  * Git
or
  * a tarball download

Build tools:
  * GCC tool chain (binutils and gcc)
    - gcc
    - ld
    - as
  * A working Linux distribution (windows isn't supported (yet))

Before you can build XFireDB you have to configure it:

    cd dir/to/xfire
    mkdir build
    cd build
    cmake ..

Instead of cmake, ccmake can be used to configure libxfiredbengine. ccmake is a cmake
configuration tool using the curses library to provide a graphical user interface.

Various options can be passed to cmake, check the documentation for more
information. When you are finished configuring the project run the following make 
commands to fully build XFireDB:

    make
    make install

Contributing
------------

XFireDB is free software, and anyone is more then welcome to contribute to this
project. If you have written a patch, you can send this patch or pull request to 
dev@michelmegens.net. Pull requests using GitHub are not preferred.

