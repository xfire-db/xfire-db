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
  # Implementation of the cluster nodes. Each node in the cluster
  # is represented by a single ClusterNode object.
  class ClusterNode
    attr_accessor :shard
    attr_reader :addr, :port

    @addr = nil
    @port = nil
    @cluster_port = nil

    # Create a new ClusterNode.
    #
    # @param [String] addr Node address.
    # @param [Fixnum] port Node port number.
    def initialize(addr, port)
      @addr = addr
      @port = port
      @cluster_port = port + 10000
    end

    # Request the node to reveal which key shards it is controlling.
    #
    # @return [String] A string representation of a Fixnum, indicating how many
    #  slots this node is controlling.
    def shards
      query = "CLUSTER SLOTS"
      cluster_query(query)
    end

    # Migrate a number of slots from this node to another one.
    #
    # @param [Fixnum] Number of slots to migrate.
    # @param [String] Destination node ID.
    # @return [String] Error code.
    def migrate_query(num, dst)
      query = "CLUSTER MIGRATE #{num} #{dst}"
      cluster_query(query)
    end

    # Add slots to this node.
    #
    # @param [String] Space separated list of slots to be added.
    # @return [String] Error code.
    def add_slots(slots)
      socket = XFireDB::SocketFactory.create_cluster_socket @addr, @cluster_port
      socket.puts "ADDSLOTS"
      socket.puts slots
      rv = socket.gets
      socket.close

      return rv
    end

    # Send a cluster query to this node.
    #
    # @param [String] query Query to send.
    # @return [String] Return value of the query sent (differs).
    def cluster_query(query)
      socket = XFireDB::SocketFactory.create_cluster_socket @addr, @cluster_port
      unless socket.nil?
        socket.puts "QUERY"
        socket.puts query

        rv = Array.new
        while line = socket.gets
          line = line.chomp
          rv.push line
        end
        socket.close

        rv = rv.shift if rv.length == 1
      else
        rv = "nil"
      end

      return rv
    end

    # Send a normal query to this node.
    #
    # @param [Client] client Client object (usually the source of the query).
    # @param [String] query Query which has to be sent.
    # @return [String] Return value for the sent query.
    def query(client, query)
      socket = XFireDB::SocketFactory.create_socket @addr, @port
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

    # Sent a gossip packet to the node.
    #
    # @param [String] gossip Gossip information.
    # @return [String] Return code.
    def gossip(gossip)
      socket = XFireDB::SocketFactory.create_cluster_socket @addr, @cluster_port
      socket.puts "GOSSIP"
      socket.puts gossip
      rv = socket.gets
      socket.close

      return rv
    end

    # Convert the node to a string.
    #
    # @return [String] String representation of a ClusterNode.
    def to_s
      "#{@addr} #{@port}"
    end
  end
end

