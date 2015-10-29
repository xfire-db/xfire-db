#
#   Fill database with dummy data
#   Copyright (C) 2015  Michel Megens <dev@michelmegens.net>
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

require 'xfiredb'

def test_hashmap
  db = @engine.db

  db['key1'] = XFireDB::Hashmap.new unless db['key1'].class == XFireDB::Hashmap
  hash = db['key1']
  hash['hkey1'] = "Test data 1"
  hash['hkey2'] = "Test data 2"
  hash['hkey3'] = "Test data 3"
  hash['hkey4'] = "Test data 4"
end

def test_list
  db = @engine.db
  db['key2'] = XFireDB::List.new unless db['key2'].class == XFireDB::List
  list = db['key2']

  list.push("List data 1")
  list.push("List data 2")
  list.push("List data 3")
  list.push("List data 4")
end

def test_string
  db = @engine.db
  db['key3'] = "String data 1"
  db['key4'] = "String data 2"
  db['key5'] = "String data 3"
  db['key6'] = "String data 4"
end

@engine = XFireDB::Engine.new
test_string
test_list
test_hashmap
@engine.stop

