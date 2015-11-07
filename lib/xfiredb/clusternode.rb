#
#   XFireDB clusternode
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
  class ClusterNode
    attr_accessor :shard
    attr_reader :addr, :port

    @addr = nil
    @port = nil
    @cluster_port = nil

    def initialize(addr, port)
      @addr = addr
      @port = port
      @cluster_port = port + 10000
    end

    def cluster_query(query)
      socket = TCPSocket.new(@addr, @cluster_port)
      socket.print(query)
      return socket.read
    end

    def query(client, query)
      socket = TCPSocket.new(@addr, @port)
      socket.puts "AUTH #{client.user.user} #{client.user.password}"
      socket.puts(query)
      rv = socket.gets
      socket.close
      return rv
    end

    def to_s
      "#{@addr} #{@port}"
    end
  end
end

