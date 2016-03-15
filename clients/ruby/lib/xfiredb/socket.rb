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
  # Socket helper class
  class Socket
    # Connect a socket.
    # @param [String] addr Server address.
    # @param [Fixnum] port Server port.
    # @param [Boolean] ssl Set to true to enable SSL.
    def self.connect(addr, port, ssl)
      sock = TCPSocket.new addr, port

      return sock unless ssl
      return XFireDB::SSLSocket.new sock
    end
  end

  # Connect SSL version of the TCPSocket class.
  class SSLSocket < OpenSSL::SSL::SSLSocket
    @sock = nil

    # Create a new SSL socket.
    # @param [TCPSocket] sock Base socket.
    def initialize(sock)
      super(sock)

      @sock = sock
      self.sync_close = true
      self.connect
    end
  end
end
