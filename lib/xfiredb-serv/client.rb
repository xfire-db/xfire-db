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

module XFireDB
  class Client
    attr_accessor :stream, :cluster, :user
    attr_reader :request, :keep, :quit_recv, :cluster_bus

    @request = nil
    @stream = nil
    @cluster = nil
    @user = nil
    @keep = false
    @quit_recv = false
    @cluster_bus = false

    def initialize(client, xql = nil, cluster_bus = false)
      @request = XFireDB::XQL.parse(xql) unless xql.nil?
      @stream = client
      @cluster_bus = cluster_bus
    end

    def Client.from_stream(stream, cluster = nil, cbus = false)
      client = Client.new(stream, nil, cbus)
      client.stream = stream
      client.cluster = cluster
      return client
    end

    def get_user(user)
      query = "MREF xfiredb-users #{user}"
      id = @cluster.where_is? 'xfiredb-users'
      hash = @cluster.nodes[id].cluster_query(query)
      return nil if hash.nil?
      hash = hash.rchomp('+')
      XFireDB::User.from_hash(user, hash)
    end

    def auth(user, password)
      if XFireDB.config.cluster_auth
        users = XFireDB.users
        u = users[user]
        if u.nil?
          u = users[user] = self.get_user(user) if u.nil?
        else
          return true if u.hash == password
          u = users[user] = self.get_user(user)
        end

        @user = XFireDB::User.new(user, password)
        @user.authenticated = u.hash == password ? true : false
        return @user.authenticated
      else
        @user = XFireDB::User.new(user, password)
        return @user.auth
      end
    end

    def auth?
      @user.authenticated
    end

    def read(ip = nil, port = nil)
      data = @stream.gets.chomp
      if data.upcase == "STREAM"
        @keep = true
        data = @stream.gets.chomp
      elsif data.upcase == "QUIT"
        @quit_recv = true
        return
      end

      @request = XFireDB::XQL.parse(data)
      @request.src_ip = ip
      @request.src_port = port
    end

    def process(xql = nil)
      if xql.nil? && @process.nil?
        raise ArgumentError.new("Cannot handle a process without a query")
      end

      @request = XFireDB::Request.new(xql) unless xql.nil?
      @request.handle
    end
  end
end
