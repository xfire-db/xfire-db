#
#   XFireDB client library
#   Copyright (C) 2016  Michel Megens <dev@michelmegens.net>
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
  # Representation of the connecting client.
  class Client
    # Create a new client.
    def initialize
      @connected = false
      @ssl = false
      @authenticated = false
    end

    # Connect to an XFireDB server
    #
    # @param [String] host Server host.
    # @param [Fixnum] port Server port.
    # @param [Fixnum] flags Connection flags.
    # @param [String] user Username.
    # @param [String] pass Password.
    # @return [Boolean] True if the connection was succesful, false otherwise.
    def connect(host, port, flags = nil, user = nil, pass = nil)
      @socket = XFireDB::Socket.connect(host, port, flags & XFireDB::SSL)
      @ssl = true if @socket.class == XFireDB::SSLSocket
      @stream = true if flags & XFireDB::STREAM == XFireDB::STREAM

      if flags & XFireDB::AUTH == XFireDB::AUTH
        @socket.puts "AUTH #{user} #{pass}"
        result = @socket.gets.chomp

        if result == "OK"
          @authenticated = true
        else
          return false
        end

        @socket.puts "STREAM" if @stream
      elsif @stream
        @socket.puts "STREAM"
      end

      @connected = true
      return true
    end

    # Query a client.
    # @param [String] q Query to send to the server.
    # @return [true]
    def query(q)
      return false unless @connected

      @socket.puts q
      reply = @socket.gets
      reply.chomp! if reply
      num = reply.scan(/\ /).count + 1

      num.times do
        data = @socket.gets
        data.chomp! if data
        res = XFireDB::Result.new data
        res.process
        yield res
      end

      true
    end

    # Close a connection.
    def close
      @connected = false
      @authenticated = false
      @stream = false

      @socket.puts "QUIT" if @connected
      @socket.close
    end

    # Check if a client is connected.
    def connected?
      @connected
    end

    # Check if the client is using SSL.
    def ssl?
      @ssl
    end

    # -- String commands --- #

    # Lookup a string key.
    # @param [String] key Key to lookup.
    # @return [String] The value of key. nil is returned if the key doesn't exist.
    def get(key)
      str = nil
      self.query("GET #{key}") {|result|
        str = result.data unless result.null?
      }

      str
    end

    # Set a key to a string value.
    # @param [String] key Key to set.
    # @param [String] data Data to set the key to.
    # @return [Boolean] True on success, false otherwise.
    def set(key, data)
      success = false
      self.query("SET #{key} \"#{data}\"") { |result|
        success = result.success?
      }
      success
    end

    # Delete a key.
    # @param [String] key Key to delete.
    # @return [Boolean] True on success, false otherwise.
    def delete(key)
      success = false
      self.query("DELETE #{key}") {|result|
        success = result.success?
      }
      success
    end

    # Add a hashmap node to a hashmap.
    # @param [String] key Hashmap to add a key to.
    # @param [String] hkey Hashmap key to add.
    # @param [String] data Data to store under hkey.
    # @return [Boolean] True on success, false otherwise.
    def map_add(key, hkey, data)
      success = false
      self.query("MADD #{key} #{hkey} \"#{data}\"") {|result|
        success = result.success?
      }
      success
    end

    # Reference a hashmap.
    # @param [String] key Hashmap to lookup.
    # @param [String] args List of hashmap keys to lookup.
    # @return [Hash] Hashmap of the lookup keys.
    #
    # @example map_ref
    #   map_ref('test-map', 'key1', 'key2', 'key3')
    # 
    # @example returned data
    #   key1 => Key1 data
    #   key2 => nil # This key didn't exist in 'test-map'
    #   key3 => Key3 data
    def map_ref(key, *args)
      map = Hash.new
      hkeys = args.join(' ')
      i = 0
      self.query("MREF #{key} #{hkeys}") {|result|
        if result.success?
          map[args[i]] = result.data
        else
          map[args[i]] = nil
        end
        i += 1
      }

      map
    end

    # Clear a hashmap.
    # @param [String] key Hashmap to clear.
    # @return [Boolean] True on success, false otherwise.
    def map_clear(key)
      self.delete(key)
    end

    # Get the size of a hashmap.
    # @param [String] key Hashmap to get the size of.
    # @return [Fixnum] Number of hashmap entry's in the hashmap.
    def map_size(key)
      size = 0
      self.query("MSIZE #{key}") {|result|
        size = result.data if result.success?
      }
      size
    end

    # Delete a number of keys from a hashmap.
    # @param [String] key Hashmap to delete from.
    # @param [String] args List of keys to delete.
    # @return [Fixnum] Number of keys deleted.
    #
    # map_delete('test-map', 'key1', 'key2', 'key3') will
    # attempt to delete the keys key1, key2 and key3 from the
    # hashmap stored at 'test-map'.
    def map_delete(key, *args)
      num = 0
      hkeys = args.join ' '
      self.query("MDEL #{key} #{hkeys}") {|result|
        num = result.data if result.success?
      }
      num
    end

    # Check if a key is included in a set.
    # @param [String] key Set to look into.
    # @param [String] set_key The set will be checked for this key.
    # @return [Boolean] True if set_key exists, false otherwise.
    def set_include?(key, set_key)
      included = false
      self.query("SINCLUDE #{key} \"#{set_key}\"") {|result|
        included = result.data if result.success?
      }
      included
    end

    # Add one or more keys to a set.
    # @param [String] key Set to add the keys to.
    # @param [String] args Keys which have to be added.
    # @return [Fixnum] Number of keys added.
    # @example set_add
    #   set_add('test-set', 'set key 1', 'set key 2', 'set key 3')
    def set_add(key, *args)
      args = array_add_quotes(args)
      keys = args.join ' '
      num = 0
      self.query("SADD #{key} #{keys}") {|result|
        num = result.data if result.success?
      }
      num
    end

    # Delete one or more keys from a set.
    # @param [String] key Set to delete from.
    # @param [String] args Keys to delete from the set.
    # @return [Fixnum] Number of keys deleted.
    def set_delete(key, *args)
      args = array_add_quotes(args)
      keys = args.join ' '
      num = 0
      self.query("SDEL #{key} #{keys}") {|result|
        num = result.data if result.success?
      }
      num
    end

    # Clear out an entire set.
    # @param [String] key Set to delete.
    # @return [Boolean] True on success, false otherwise.
    def set_clear(key)
      self.delete(key)
    end

    # Add data to a list.
    # @param [String] key List to push into.
    # @param [String] data Data to push into the list.
    # @return [Boolean] True on success, false otherwise.
    def list_push(key, data)
      success = false
      self.query("LPUSH #{key} \"#{data}\"") {|result|
        success = result.success?
      }
      success
    end

    # Reference a list by index or range.
    # @param [String] key List to reference.
    # @param [String] idx Index or range.
    # @return [Array] Array of the results.
    #
    # @example by index
    #   list_ref('test-list', 1)
    # @example by range
    #   list_ref('test-list', '0..-1')
    def list_ref(key, idx)
      ary = Array.new
      self.query("LREF #{key} #{idx}") {|result|
        if result.success?
          ary.push result.data
        else
          ary.push nil
        end
      }
      ary
    end

    # Pop elements from a list by index or range.
    # @param [String] key List to reference.
    # @param [String] idx Index or range.
    # @return [Array] Array of the results.
    #
    # @example by index
    #   list_pop('test-list', 1)
    # @example by range
    #   list_pop('test-list', '0..-1')
    def list_pop(key, idx)
      ary = Array.new
      self.query("LPOP #{key} #{idx}") {|result|
        if result.success?
          ary.push result.data
        else
          ary.push nil
        end
      }
      ary
    end

    # Set the data in a list at a specific index.
    # @param [String] key List to reference.
    # @param [Fixnum] idx Index to edit.
    # @param [String] data Data to set.
    # @note The index has to exist before you are able to use list_set on it.
    def list_set(key, idx, data)
      success = false
      self.query("LSET #{key} #{idx} \"#{data}\"") {|result|
        success = result.success?
      }
      success
    end

    # Get the size of a list (number of elements).
    # @param [String] key List to get the size of.
    # @return [Fixnum] Number of elements in the list.
    def list_size(key)
      num = 0
      self.query("LSIZE #{key}") {|result|
        num = result.data if result.success?
      }
      num
    end

    # Clear out an entire list.
    # @param [String] key List to delete.
    # @return [Boolean] True on success, false otherwise.
    def list_clear(key)
      self.delete(key)
    end

    private
    def array_add_quotes(ary)
      return unless ary.kind_of? Array
      ary.collect! {|element|
        "\"#{element}\""
      }
      ary
    end
  end
end

