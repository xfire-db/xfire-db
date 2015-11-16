#
#   XFireDB commands
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
  class Command
    @cmd = nil
    @argv = nil
    @raw = nil
    @cluster = nil
    @client = nil

    def initialize(cluster, cmd, client, key = nil, raw = nil)
      @cluster = cluster
      @cmd = cmd
      @argv = client.request.args
      @raw = raw
      @client = client
      @key = key unless key.nil?
      @key = @argv[0] unless key

      unless @cmd == "AUTH" or client.cluster_bus
        raise IllegalKeyException, "Key: #{@key} is illegal" if XFireDB.illegal_key? @key or XFireDB.private_key? @key
      end

      if client.user and cmd == "CLUSTER"
        raise IllegalCommandException, "Not authorized to execute CLUSTER commands" unless client.user.level >= XFireDB::User::ADMIN
      end
    end

    def forward(key, query = nil)
      node = @cluster.where_is? key
      node = @cluster.nodes[node]
      return node.cluster_query @raw unless @raw.nil?
      return node.cluster_query query
    end

    def exec(add)
      if add
        @cluster.local_node.shard.add_key(@key) unless @key.nil?
      else
        @cluster.local_node.shard.del_key(@key) unless @key.nil?
      end
    end
  end
end

