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

      puts num

      num.times do
        data = @socket.gets
        data.chomp! if data
        puts data
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
  end
end

