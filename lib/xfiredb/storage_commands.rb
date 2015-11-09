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
  class CommandLPush < XFireDB::Command
    def initialize(cluster, client)
      super(cluster, "LPUSH", client)
    end

    # LPUSH <key> <data>
    def exec
      key = @argv[0]
      data = @argv[1]

      return forward(key, "LPUSH #{key} \"#{data}\"") unless @cluster.local_node.shard.include? key

      db = XFireDB.db
      db[key] = XFireDB::List.new unless db[key].is_a? XFireDB::List
      db[key].push data
      return "OK"
    end
  end

  class CommandLPop < XFireDB::Command
    def initialize(cluster, client)
      super(cluster, "LPOP", client)
    end

    # LPOP <key> idx
    # LPOP <key> n..m
    def exec
      key = @argv[0]
      idx = @argv[1]

      return forward key, "LPOP #{key} #{idx}" unless @cluster.local_node.shard.include? key
      unless idx.is_i?
        range = idx.split('..')
        return "LPOP range invalid: #{idx}" unless range[0].is_i? and range[1].is_i?

        list = XFireDB.db[key]
        return "nil" unless list.is_a? XFireDB::List

        range[0] = range[0].to_i
        range[1] = range[1].to_i
        range[0] += list.length unless range[0] >= 0
        range[1] += list.length unless range[1] >= 0
        return "LPOP range invalid: #{idx}" unless range[0] <= range[1]

        rv = Array.new
        n = range[0]

        while range[0] <= range[1]
          rv.push list.pop n
          range[0] += 1
        end

        # delete the entire list unless there are entry's left
        XFireDB.db.delete(key) unless list.length > 0

        return rv
      else
        idx = idx.to_i
        list = XFireDB.db[key]

        return "nil" unless list.is_a? XFireDB::List
        rv = list.pop idx
        XFireDB.db.delete key unless list.length > 0
        return rv
      end
    end
  end

  class CommandLRef < XFireDB::Command
    def initialize(cluster, client)
      super(cluster, "LREF", client)
    end

    # LREF <key> idx
    # LREF <key> n..m
    def exec
      key = @argv[0]
      idx = @argv[1]

      return forward key, "LREF #{key} #{idx}" unless @cluster.local_node.shard.include? key

      unless idx.is_i?
        range = idx.split('..')
        return "LREF range invalid: #{idx}" unless range[0].is_i? and range[1].is_i?

        list = XFireDB.db[key]
        return "nil" unless list.is_a? XFireDB::List

        range[0] = range[0].to_i
        range[1] = range[1].to_i
        range[0] += list.length unless range[0] >= 0
        range[1] += list.length unless range[1] >= 0

        return "LREF range invalid: #{idx}" unless range[0] <= range[1]

        rv = Array.new
        while range[0] <= range[1]
          entry = list[range[0]]
          rv.push unless entry.nil?
          rv.push "nil" if entry.nil?

          range[0] += 1
        end

        return rv
      else
        idx = idx.to_i
        list = XFireDB.db[key]

        return "nil" unless list.is_a? XFireDB::List
        return list[idx]
      end
    end
  end

  class CommandLSize < XFireDB::Command
    def initialize(cluster, client)
      super(cluster, "LSIZE", client)
    end

    # LSIZE <key>
    def exec
      key = @argv[0]

      return "Syntax error: LSIZE <key>" unless key
      return forward key, "LSIZE #{key}" unless @cluster.local_node.shard.include? key

      list = XFireDB.db[key]
      return "nil" unless list.is_a? XFireDB::List
      return list.length
    end
  end

  class CommandLSet < XFireDB::Command
    def initialize(cluster, client)
      super(cluster, "LSET", client)
    end

    # LSET <key> <idx> <data>
    def exec
    end
  end

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

