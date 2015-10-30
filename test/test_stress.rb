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
  def setup
    puts ""
    @engine = XFireDB::Engine.new
    @engine.set_loadstate(false)
  end

  def teardown
    @engine.stop
  end

  def test_stress
    db = @engine.db

    40000.times do |x|
      key = "key#{x}"
      hkey = "hkey#{x}"
      data = "Test data #{x}"

      db[key] ||= XFireDB::Hashmap.new
      db[key][hkey] = data
    end

    assert_equal("Test data 800", db["key800"]["hkey800"], "First assert failed")
    assert_equal("Test data 800", db["key800"]["hkey800"], "Second assert failed")

    map = db['key4000']
    map.each do |k, v|
      assert_equal("hkey4000", k)
      assert_equal("Test data 4000", v)
    end

    40000.times do |x|
      key = "key#{x}"
      assert_equal(key, db.delete(key))
    end
  end
end
