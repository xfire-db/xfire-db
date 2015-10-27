#
#   XFireDB StorageEngine unit test
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

require 'xfiredb/storage_engine'
require 'test/unit'

class TestStorageEngine < Test::Unit::TestCase
  def setup
    puts ""
    @db = XFireDB::Database.new
    @db["key1"] = "Test data 1";
    @db["key2"] = "Test data 2";
    @db["key3"] = "Test data 3";
  end

  def teardown
  end

  def test_ref
    assert_equal("Test data 2", @db["key2"], "Db ref failed")
    @db["key2"] = "Test data 2, updated"
    assert_equal("Test data 2, updated", @db["key2"], "DB entry update failed")
  end

  def test_delete
    @db.delete("key3")
    assert_equal(nil, @db["key3"], "DB delete failed")
  end

  def test_list
    list = XFireDB::List.new
    list.push("List data 1")
    list.push("List data 2")
    list.push("List data 3")
    @db["key4"] = list

    tmp = @db["key4"]
    tmp.each do |value|
      puts value
    end

    assert_equal(4, @db.size, "Database size failed")
  end
end

