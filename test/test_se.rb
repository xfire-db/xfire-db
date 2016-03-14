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

require 'xfiredb-serv'
require 'minitest/autorun'

class TestStorageEngine < Minitest::Test
  TEST_STRING_DATA = "Test string data"
  TEST_STRING_KEY  = "key-1"

  TEST_LIST_KEY = "key-2"
  TEST_LIST_DATA1 = "Test list data 1"
  TEST_LIST_DATA2 = "Test list data 2"
  TEST_LIST_DATA3 = "Test list data 3"
  TEST_LIST_DATA4 = "Test list data 4"

  TEST_HM_KEY = "key-3"
  TEST_HM_SUB_KEY1 = "sub-key-1"
  TEST_HM_SUB_KEY2 = "sub-key-2"
  TEST_HM_SUB_KEY3 = "sub-key-3"
  TEST_HM_SUB_KEY4 = "sub-key-4"
  TEST_HM_DATA1 = "Test hashmap data 1"
  TEST_HM_DATA2 = "Test hashmap data 2"
  TEST_HM_DATA3 = "Test hashmap data 3"
  TEST_HM_DATA4 = "Test hashmap data 4" 

  def setup
    puts ""
    @engine = XFireDB::Engine.new
  end

  def teardown
    @engine.exit
  end

  def test_hashmap
  end

  def test_list
  end

  def test_string
  end
end

