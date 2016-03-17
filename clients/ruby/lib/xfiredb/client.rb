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

    def get(key)
      str = nil
      self.query("GET #{key}") {|result|
        str = result.data unless result.null?
      }

      str
    end

    def set(key, data)
      success = false
      self.query("SET #{key} \"#{data}\"") { |result|
        success = result.success?
      }
      success
    end

    def delete(key)
      success = false
      self.query("DELETE #{key}") {|result|
        success = result.success?
      }
      success
    end

    def map_add(key, hkey, data)
      success = false
      self.query("MADD #{key} #{hkey} \"#{data}\"") {|result|
        success = result.success?
      }
      success
    end

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

    def map_clear(key)
      self.delete(key)
    end

    def map_size(key)
      size = 0
      self.query("MSIZE #{key}") {|result|
        size = result.data if result.success?
      }
      size
    end

    def map_delete(key, *args)
      num = 0
      hkeys = args.join ' '
      self.query("MDEL #{key} #{hkeys}") {|result|
        num = result.data if result.success?
      }
      num
    end

    def set_include?(key, set_key)
      included = false
      self.query("SINCLUDE #{key} \"#{set_key}\"") {|result|
        included = result.data if result.success?
      }
      included
    end

    def set_add(key, *args)
      args = array_add_quotes(args)
      keys = args.join ' '
      num = 0
      self.query("SADD #{key} #{keys}") {|result|
        num = result.data if result.success?
      }
      num
    end

    def set_delete(key, *args)
      args = array_add_quotes(args)
      keys = args.join ' '
      num = 0
      self.query("SDEL #{key} #{keys}") {|result|
        num = result.data if result.success?
      }
      num
    end

    def set_clear(key)
      self.delete(key)
    end

    def list_push(key, data)
      success = false
      self.query("LPUSH #{key} \"#{data}\"") {|result|
        success = result.success?
      }
      success
    end

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

    def list_set(key, idx, data)
      success = false
      self.query("LSET #{key} #{idx} \"#{data}\"") {|result|
        success = result.success?
      }
      success
    end

    def list_size(key)
      num = 0
      self.query("LSIZE #{key}") {|result|
        num = result.data if result.success?
      }
      num
    end

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

