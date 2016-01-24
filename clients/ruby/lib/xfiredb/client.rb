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
  class Client
    def initialize
      @connected = false
      @ssl = false
      @authenticated = false
    end

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

      return true
    end

    def connected?
      @connected
    end

    def ssl?
      @ssl
    end
  end
end

