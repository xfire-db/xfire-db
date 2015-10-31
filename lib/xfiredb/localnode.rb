#
#   XFireDB LocalNode class
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

module XFireDB
  class LocalNode < ClusterNode
    attr_reader :engine

    @engine = nil

    def initialize(shard)
      super(shard)
      @engine = XFireDB::Engine.new
    end

    def lookup(key)
      @engine.db[key]
    end

    def store(key, value)
      @engine.db[key] = value
    end

    def delete(key)
      @engine.db.delete(key)
    end

    def set(key, value)
      self.store(key, value)
    end

    def [](key)
      self.lookup(key)
    end

    def []=(key, value)
      self.store(key, value)
    end

    def query(query)
    end
  end
end

