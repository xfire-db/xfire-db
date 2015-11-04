#
#   XFireDB Set test
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
    @set = XFireDB::Set.new

    assert_equal("key1", @set.add("key1"))
    assert_equal("key2", @set.add("key2"))
    assert_equal("key3", @set.add("key3"))
    assert_equal("key4", @set.add("key4"))
  end

  def teardown
    @set.clear
  end

  def test_delete
    assert_equal(4, @set.size)
    @set.clear
    assert_equal(0, @set.size)
  end

  def test_lookup
    assert(@set.include?("key1"))
    assert(@set.include?("key2"))
    assert(@set.include?("key3"))
    assert(@set.include?("key4"))

    @set.remove("key2")
    assert_equal(false, @set.include?("key2"))
    assert_equal(3, @set.size)
  end

  def test_iterate
    counter = 0
    @set.each { |key| counter += 1 }

    assert_equal(4, counter)
  end
end
