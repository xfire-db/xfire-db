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

require 'xfiredb'
require 'test/unit'

class TestStorageEngine < Test::Unit::TestCase
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
    @engine = XFireDB::StorageEngine.new
  end

  def teardown
    @engine.delete
  end

  def test_disk
    key_arr = [TEST_HM_SUB_KEY1, TEST_HM_SUB_KEY2, TEST_HM_SUB_KEY3, TEST_HM_SUB_KEY4];
    assert(@engine.hm_set(TEST_HM_KEY, TEST_HM_SUB_KEY1, TEST_HM_DATA1), "Hashmap set failed")
    assert(@engine.hm_set(TEST_HM_KEY, TEST_HM_SUB_KEY2, TEST_HM_DATA2), "Hashmap set failed")
    assert(@engine.hm_set(TEST_HM_KEY, TEST_HM_SUB_KEY3, TEST_HM_DATA3), "Hashmap set failed")
    assert(@engine.hm_set(TEST_HM_KEY, TEST_HM_SUB_KEY4, TEST_HM_DATA4), "Hashmap set failed")

    @engine.delete
    @engine = XFireDB::StorageEngine.new

    assert_equal([TEST_HM_DATA1, TEST_HM_DATA2, TEST_HM_DATA3, TEST_HM_DATA4],
                 @engine.hm_get(TEST_HM_KEY, key_arr), "Hashmap get failed")
    assert_equal(4, @engine.hm_remove(TEST_HM_KEY, key_arr), "Hashmap remove failed")
  end

  def test_hashmap
    key_arr = [TEST_HM_SUB_KEY1, TEST_HM_SUB_KEY2, TEST_HM_SUB_KEY3, TEST_HM_SUB_KEY4];
    assert(@engine.hm_set(TEST_HM_KEY, TEST_HM_SUB_KEY1, TEST_HM_DATA1), "Hashmap set failed")
    assert(@engine.hm_set(TEST_HM_KEY, TEST_HM_SUB_KEY2, TEST_HM_DATA2), "Hashmap set failed")
    assert(@engine.hm_set(TEST_HM_KEY, TEST_HM_SUB_KEY3, TEST_HM_DATA3), "Hashmap set failed")
    assert(@engine.hm_set(TEST_HM_KEY, TEST_HM_SUB_KEY4, TEST_HM_DATA4), "Hashmap set failed")

    assert_equal([TEST_HM_DATA1, TEST_HM_DATA2, TEST_HM_DATA3, TEST_HM_DATA4],
                 @engine.hm_get(TEST_HM_KEY, key_arr), "Hashmap get failed")
    assert_equal(4, @engine.hm_remove(TEST_HM_KEY, key_arr), "Hashmap remove failed")
  end

  def test_string
    assert(@engine.string_set(TEST_STRING_KEY, TEST_STRING_DATA), "String set failed!")
    assert_equal(TEST_STRING_DATA, @engine.string_get(TEST_STRING_KEY), "String get failed!");
    assert_equal(1, @engine.key_delete(TEST_STRING_KEY), "String delete failed");
  end

  def test_list
    assert(@engine.list_set(TEST_LIST_KEY, 0, TEST_LIST_DATA1), "List set failed")
    assert(@engine.list_set(TEST_LIST_KEY, 1, TEST_LIST_DATA2), "List set failed")
    assert(@engine.list_set(TEST_LIST_KEY, 2, TEST_LIST_DATA3), "List set failed")
    assert(@engine.list_set(TEST_LIST_KEY, 3, TEST_LIST_DATA4), "List set failed")

    assert_equal([TEST_LIST_DATA1, TEST_LIST_DATA2,
                  TEST_LIST_DATA3, TEST_LIST_DATA4],
                  @engine.list_get_seq(TEST_LIST_KEY, 0, -1), "List get failed")
    assert_equal(4, @engine.list_pop_seq(TEST_LIST_KEY, 0, -1), "List pop failed")
  end
end

