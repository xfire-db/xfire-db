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

    def shards
      query = "CLUSTER SLOTS"
      cluster_query(query)
    end

    def migrate_query(num, dst)
      query = "CLUSTER MIGRATE #{num} #{dst}"
      cluster_query(query)
    end

    def add_slots(slots)
      socket = TCPSocket.new(@addr, @cluster_port)
      socket.puts "ADDSLOTS"
      socket.puts slots
      rv = socket.gets
      socket.close

      return rv
    end

    def cluster_query(query)
      socket = TCPSocket.new(@addr, @cluster_port)
      socket.puts "QUERY"
      socket.puts query
      rv = socket.gets.chomp
      socket.close

      return rv
    end

    def query(client, query)
      socket = TCPSocket.new(@addr, @port)
      socket.puts "AUTH #{client.user.user} #{client.user.password}" if XFireDB.config.auth
      socket.puts(query)

      rv = Array.new
      while line = socket.gets
        line = line.chomp
        rv.push line
      end
      socket.close
      return rv
    end

    def gossip(gossip)
      socket = TCPSocket.new(@addr, @cluster_port)
      socket.puts "GOSSIP"
      socket.puts gossip
      rv = socket.gets
      socket.close

      return rv
    end

    def to_s
      "#{@addr} #{@port}"
    end
  end
end

