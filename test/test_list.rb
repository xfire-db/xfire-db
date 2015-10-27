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
    @list = XFireDB::List.new

    @list.push("Test data 1")
    @list.push("Test data 2")
    @list.push("Test data 3")
    @list.push("Test data 4")
  end

  def teardown
  end

  def test_list_push
    puts @list[2]
    puts @list
  end

  def test_list_each
    puts @list.length
    @list.each { |x| puts x }
  end

  def test_list_pop
    @list.pop(0)
    @list.pop(0)
    @list.pop(0)
    @list.pop(0)

    assert_equal(0, @list.length)
  end
end

