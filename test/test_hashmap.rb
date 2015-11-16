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
#
require 'xfiredb/storage_engine'
require 'test/unit'

class TestStorageEngine < Test::Unit::TestCase
  def setup
    puts ""
    @map = XFireDB::Hashmap.new

    @map.store("test-key1", "Test data 1")
    @map.store("test-key2", "Test data 2")
    @map.store("test-key3", "Test data 3")
    @map.store("test-key4", "Test data 4")
  end

  def teardown
  end

  def test_delete
    assert_equal(4, @map.size)
    @map.delete("test-key1")
    @map.delete("test-key2")
    @map.delete("test-key3")
    @map.delete("test-key4")
  end

  def test_lookup
    assert_equal("Test data 3", @map["test-key3"], "Lookup failed")
  end

  def test_clear
    @map.clear
    assert_equal(0, @map.size)
  end

  def test_iterate
    i = 0
    @map.each { |key, value| i += 1 }
    assert_equal(4, i)
  end
end
