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
  # XFireDB storage engine interface.
  class Engine
    attr_reader :db

    # Create a new storage engine.
    def initialize
      @db = XFireDB::Database.new
    end

    # Create a pre-init storage engine. This engine isn't fully
    # initialised either and can therefore by safely shutdown without
    # breaking anything.
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

    # Start the storage engine.
    def start
      set_loadstate(false)
      config = XFireDB.config
      self.init(config.log_file, config.err_log_file, config.db_file, config.persist_level, true)

      self.load.each.each do |key, hash, type, data|
        load_entry(key, hash, type, data)
      end

      self.set_loadstate(true)
    end

    # Stop the XFireDB engine. All data will be stored to disk.
    def exit
      self.set_loadstate(false)
      @db.each do |key, value|
        @db.delete(key)
      end
      self.stop(@db)
    end

    # Only delete all keys from the database to plug memory leaks. Backend
    # systems are not stopped. This is useful for unit tests that do not
    # start the systems in the first place.
    def exit_soft
      self.set_loadstate(false)
      @db.each do |key, value|
        @db.delete(key)
      end
    end

    private
    # Load a database entry from file.
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

    # Load a string into the DB.
    #
    # @param [String] key Key to load.
    # @param [String] data Data to load.
    def load_string(key, data)
      @db[key] = data
    end

    # Load a list entry into the DB.
    #
    # @param [String] key Key to load.
    # @param [String] data Data to load.
    def load_list_entry(key, data)
      @db[key] ||= XFireDB::List.new
      list = @db[key]
      return unless list.class == XFireDB::List
      list.push(data)
    end

    # Load a hashmap entry from disk.
    #
    # @param [String] key Key to load.
    # @param [String] hash Hash to load.
    # @param [String] data Data to load.
    def load_map_entry(key, hash, data)
      @db[key] ||= XFireDB::Hashmap.new
      map = @db[key]
      return unless map.class == XFireDB::Hashmap
      map[hash] = data
    end

    # Load a set entry from disk.
    #
    # @param [String] key Key to load.
    # @param [String] skey Set-key to load.
    def load_set_entry(key, skey)
      @db[key] ||= XFireDB::Set.new
      set = @db[key]
      return unless set.class == XFireDB::Set
      set.add(skey)
    end
  end
end

