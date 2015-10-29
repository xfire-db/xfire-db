#
#   XFireDB module
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
  class Engine
    attr_reader :db

    def initialize
      @db = XFireDB::Database.new
      self.init

      self.load.each.each do |key, hash, type, data|
        case type
        when "string"
          self.load_string(key, data)
        when "list"
          self.load_list_entry(key, data)
        when "hashmap"
          self.load_map_entry(key, hash, data)
        end
      end

      self.set_loadstate(true)
    end

    def load_string(key, data)
      @db[key] = data
    end

    def load_list_entry(key, data)
      @db[key] ||= XFireDB::List.new
      list = @db[key]
      return unless list.class == XFireDB::List
      list.push(data)
    end

    def load_map_entry(key, hash, data)
      @db[key] ||= XFireDB::Hashmap.new
      map = @db[key]
      return unless map.class == XFireDB::Hashmap
      map[hash] = data
    end
  end
end
