#
#   XFireDB engine
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
    end

    def pre_init
      config = XFireDB.config
      self.init(config.log_file, config.err_log_file, config.db_file, config.persist_level, false)

      XFireDB.preinit_keys.each do |key|
        self.load_key(key).each do |key,hash,type,data|
          load_entry key, hash, type, data
        end
      end

      set_loadstate(true)
    end

    def start
      set_loadstate(false)
      config = XFireDB.config
      self.init(config.log_file, config.err_log_file, config.db_file, config.persist_level, true)

      self.load.each.each do |key, hash, type, data|
        load_entry(key, hash, type, data)
      end

      self.set_loadstate(true)
    end

    def exit
      self.set_loadstate(false)
      @db.each do |key, value|
        @db.delete(key)
      end
      self.stop(@db)
    end

    private
    def load_entry(key, hash, type, data)
        case type
        when "string"
          load_string(key, data)
        when "list"
          load_list_entry(key, data)
        when "hashmap"
          load_map_entry(key, hash, data)
        when "set"
          load_set_entry(key, hash)
        end
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

    def load_set_entry(key, skey)
      @db[key] ||= XFireDB::Set.new
      set = @db[key]
      return unless set.class == XFireDB::Set
      set.add(skey)
    end
  end
end

