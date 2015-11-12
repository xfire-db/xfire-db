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
  class SSLSocket < OpenSSL::SSL::SSLSocket
    @sock = nil

    def initialize(sock)
      super(sock)
      @sock = sock
      self.sync_close = true
      self.connect
    end

    def close
      @sock.close
    end
  end

  class SocketFactory
    def SocketFactory.create_socket(addr, port)
      config = XFireDB.config
      socket = TCPSocket.new addr, port

      return socket unless config.ssl
      return XFireDB::SSLSocket.new(socket)
    end

    def SocketFactory.create_server_socket(addr, port)
    end
  end
end

