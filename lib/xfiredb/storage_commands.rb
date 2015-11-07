#
#   XFireDB storage commands
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
  class CommandSet < XFireDB::Command
    def initialize(cluster, client)
      super(cluster, "SET", client)
    end

    def exec
      key = @argv[0]
      data = @argv[1]
      db = XFireDB.db

      return "Syntax `GET <key> \"<data>\"'" unless key and data
      if @cluster.local_node.shard.include?(key)
        db[key] = data
      else
        node = @cluster.where_is?(key)
        node = @cluster.nodes[node]
        return node.query(@client, "SET #{key} \"#{data}\"")
      end
      return "OK"
    end
  end

  class CommandGet < XFireDB::Command
    def initialize(cluster, client)
      super(cluster, "GET", client)
    end

    def exec
      db = XFireDB.db
      return unless @argv[0]

      if @cluster.local_node.shard.include?(@argv[0])
        return db[@argv[0]]
      else
        node = @cluster.where_is?(@argv[0])
        node = @cluster.nodes[node]
        return node.query(@client, "GET #{@argv[0]}")
      end
    end
  end

  class CommandAuth < XFireDB::Command
    def initialize(cluster, client)
      super(cluster, "AUTH", client)
    end

    def exec
      user = @argv[0]
      pw = @argv[1]
      map = XFireDB.db['xfiredb']
      local_pw = BCrypt::Password.new(map["user::#{user}"])
      return "Access denied" if local_pw.nil? or local_pw != pw
      return "OK"
    end
  end

  class CommandDelete < XFireDB::Command
    def initialize(cluster, client)
      super(cluster, "DELETE", client)
    end

    def exec
      key = @argv[0]
      db = XFireDB.db

      return unless key
      if @cluster.local_node.shard.include?(key)
        db.delete(key)
        return "OK"
      else
        node = @cluster.where_is?(key)
        node = @cluster.nodes[node]
        return node.query(@client, "DELETE #{key}")
      end
    end
  end
end

