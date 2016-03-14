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
  # SSLSocket API
  class SSLSocket < OpenSSL::SSL::SSLSocket
    @sock = nil

    # Create a new SSL socket.
    #
    # @param [TCPSocket] sock Standard TCP socket.
    def initialize(sock)
      super(sock)
      @sock = sock
      self.sync_close = true
      self.connect
    end

    # Close a SSLSocket.
    def close
      @sock.close
    end
  end

  # Factory interface to create sockets transparantly.
  class SocketFactory

    # Create a socket. Based on the config settings the returned
    # socket will be a standard TCPSocket or an SSLSocket.
    #
    # @param [String] addr Socket address.
    # @param [Fixnum] port Socket port number.
    # @return [TCPSocket] The created socket.
    def SocketFactory.create_socket(addr, port)
      config = XFireDB.config
      begin
        socket = TCPSocket.new addr, port
      rescue Errno::ECONNREFUSED
        return nil
      end

      return socket unless config.ssl
      return XFireDB::SSLSocket.new(socket)
    end

    # Create a socket for inter-cluster communication.
    #
    # @param [String] addr Socket address.
    # @param [Fixnum] port Socket port.
    # @param [String] user User to authenticate with.
    # @param [String] pass Password for the user.
    # @return [TCPSocket] The created socket.
    def SocketFactory.create_cluster_socket(addr, port, user = nil, pass = nil)
      db = XFireDB.db
      config = XFireDB.config
      sock = SocketFactory.create_socket(addr, port)
      return sock unless config.cluster_auth

      secret = db['xfiredb']['secret']
      sock.puts "AUTH #{secret}"
      reply = sock.gets.chomp

      unless reply == "OK"
        sock.close
        return nil
      end

      return sock
    end

    # Create a server socket.
    #
    # @param [String] addr Server address.
    # @param [Fixnum] port Server port.
    # @return [TCPSocket] Socket which can be listened on.
    def SocketFactory.create_server_socket(addr, port)
    end
  end
end

