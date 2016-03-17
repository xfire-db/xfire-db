#
#   XFireDB client
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

#
module XFireDB
  # Object representation of connecting clients
  class Client
    attr_accessor :stream, :cluster, :user
    attr_reader :request, :keep, :quit_recv, :cluster_bus, :failed

    @request = nil
    @stream = nil
    @cluster = nil
    @user = nil
    @keep = false
    @quit_recv = false
    @cluster_bus = false
    @failed = false

    # Initialize a new client.
    #
    # @param [TCPSocket] client TCP stream to communicate with the client.
    # @param [String] xql XQL query
    # @param [Boolean] cluster_bus Indicates whether the other end is a cluster node (true) or client (false).
    def initialize(client, xql = nil, cluster_bus = false)
      @request = XFireDB::XQL.parse(xql) unless xql.nil?
      @stream = client
      @cluster_bus = cluster_bus
    end

    # Factory method to create a new client from a stream.
    #
    # @param [TCPSocket] stream TCP stream to communicate with the client.
    # @param [Cluster] cluster Cluster object.
    # @param [Boolean] cbus Indicates whether the other end is a cluster node (true) or client (false).
    def Client.from_stream(stream, cluster = nil, cbus = false)
      client = Client.new(stream, nil, cbus)
      client.stream = stream
      client.cluster = cluster
      return client
    end

    # Get a user to allow authentication.
    #
    # @param [String] user Username to get meta information about.
    def get_user(user)
      query = "MREF xfiredb-users #{user}"
      id = @cluster.where_is? 'xfiredb-users'
      hash = @cluster.nodes[id].cluster_query(query)
      return nil if hash.nil?
      hash = hash.rchomp('+')
      hash, level = hash.split(' ')
      u = XFireDB::User.from_hash(user, hash)
      u.level = level.to_i
      return u
    end

    # Authenticate a client.
    #
    # @param [String] user Username.
    # @param [String] password Password.
    # @return [Boolean] true if the authentication succeeded, false otherise.
    def auth(user, password)
      if XFireDB.config.cluster_auth
        users = XFireDB.users
        u = users[user]
        @user = XFireDB::User.new(user, password)

        if u.nil?
          u = users[user] = self.get_user(user) if u.nil?
        else
          @user.authenticated = u.hash == password ? true : false
          @user.level = u.level
          return true if @user.authenticated
          u = users[user] = self.get_user(user)
          return false if u.nil?
        end

        @user.level = u.level
        @user.authenticated = u.hash == password ? true : false
        return @user.authenticated
      else
        @user = XFireDB::User.new(user, password)
        @user.level = XFireDB::User::ADMIN
        return @user.auth
      end
    end

    # Check whether the client is authenticated or not.
    #
    # @return [Boolean] true if authenticated, false otherwise.
    def auth?
      @user.authenticated
    end

    # Get a request from a client.
    #
    # @param [String] ip Source IP
    # @param [Fixnum] port Source port
    def read(ip = nil, port = nil)
      data = @stream.gets

      if data.nil?
        @quit_recv = true
        return
      else
        data.chomp!
      end

      if data.upcase == "STREAM"
        @keep = true
        data = @stream.gets
        if data.nil?
          @quit_recv = true
          return
        end
        data.chomp!
      end

      if data.upcase == "QUIT"
        @quit_recv = true
        return
      end

      @request = XFireDB::XQL.parse(data)
      @failed = true if @request.nil?
      @request.src_ip = ip
      @request.src_port = port
    end
  end
end

