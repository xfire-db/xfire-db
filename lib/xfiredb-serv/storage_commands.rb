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

# Answer prefixes:
#
# + --> Data answer (straight out of the database)
# - --> Error code answer ('nil' and 'OK' are considered error codes too)
# % --> Numeral answer.

module XFireDB
  class CommandSInclude < XFireDB::Command
    def initialize(cluster, client)
      super(cluster, "SINCLUDE", client)
    end

    def exec
      key = @argv.shift

      return forward(key, "SINCLUDE #{key} #{@argv.map(&:quote).join(' ')}") unless @cluster.local_node.shard.include? key
      return "-Syntax error: SINCLUDE <key> <set-key1> <set-key2> ..." unless key and @argv.length > 0

      set = XFireDB.db[key]
      return "-nil" unless set.is_a? XFireDB::Set

      rv = Array.new
      @argv.each do |hkey|
        rv.push "+" + set.include?(hkey).to_s
      end

      return rv
    end
  end

  class CommandSClear < XFireDB::Command
    def initialize(cluster, client)
      super(cluster, "SCLEAR", client)
    end

    def exec
      key = @argv[0]

      return forward(key, "SCLEAR #{key}") unless @cluster.local_node.shard.include? key
      return "-Syntax error: SCEAR <key>" unless key and @argv.length > 0

      set = XFireDB.db[key]
      return "-nil" unless set.is_a? XFireDB::Set

      XFireDB.db.delete(key)
      super(false)
      return "-OK"
    end
  end

  class CommandSDel < XFireDB::Command
    def initialize(cluster, client)
      super(cluster, "SDEL", client)
    end

    def exec
      key = @argv.shift

      return forward(key, "SDEL #{key} #{@argv.map(&:quote).join(' ')}") unless @cluster.local_node.shard.include? key
      return "-Syntax error: SDEL <key> <set-key1> <set-key2> ..." unless key and @argv.length > 0

      set = XFireDB.db[key]
      return "-nil" unless set.is_a? XFireDB::Set

      rv = 0
      @argv.each do |hkey|
        rv += 1 unless set.remove(hkey).nil?
      end

      unless set.size > 0
        XFireDB.db.delete(key)
        super(false)
      end

      rv = "%" + rv.to_s
      return rv
    end
  end

  class CommandSAdd < XFireDB::Command
    def initialize(cluster, client)
      super(cluster, "SADD", client)
    end

    def exec
      key = @argv.shift

      return forward(key, "SADD #{key} #{@argv.map(&:quote).join(' ')}") unless @cluster.local_node.shard.include? key
      return "-Syntax error: SADD <key> <set-key1> <set-key2> ..." unless key and @argv.length > 0

      XFireDB.db[key] = XFireDB::Set.new unless XFireDB.db[key].is_a? XFireDB::Set
      set = XFireDB.db[key]

      rv = 0
      @argv.each do |hkey|
        rv += 1 unless set.add(hkey).nil?
      end

      super(true)

      rv = "%" + rv.to_s
      return rv
    end
  end

  class CommandMDel < XFireDB::Command
    def initialize(cluster, client)
      super(cluster, "MDEL", client)
    end

    def exec
      key = @argv.shift

      return forward(key, "MDEL #{key} #{@argv.join(' ')}") unless @cluster.local_node.shard.include? key
      return "-Syntax error: MDEL <key> <hkey1> <hkey2> ..." unless key

      map = XFireDB.db[key]
      return "-nil" unless map.is_a? XFireDB::Hashmap

      rv = 0
      @argv.each do |hkey|
        rv += 1 unless map.delete(hkey).nil?
      end

      unless map.size > 0
        XFireDB.db.delete(key)
        super(false)
      end

      rv = "%" + rv.to_s
      return rv
    end
  end

  class CommandMRef < XFireDB::Command
    def initialize(cluster, client)
      super(cluster, "MREF", client)
    end

    # MREF <key> <hkey1> <hkey2> ...
    def exec
      key = @argv.shift

      return forward(key, "MREF #{key} #{@argv.join(' ')}") unless @cluster.local_node.shard.include? key
      return "-Syntax error: MREF <key> <hkey1> <hkey2> ..." unless key

      map = XFireDB.db[key]
      return "-nil" unless map.is_a? XFireDB::Hashmap

      rv = Array.new
      @argv.each do |hkey|
        v = map[hkey]
        rv.push "-" + "nil" if v.nil?
        rv.push "+" + v if v
      end

      return rv
    end
  end

  class CommandMClear < XFireDB::Command
    def initialize(cluster, client)
      super(cluster, "MCLEAR", client)
    end

    def exec
      key = @argv[0]

      return "-Syntax error: MCLEAR <key>" unless key
      return forward(key, "MCLEAR #{key}") unless @cluster.local_node.shard.include? key
      db = XFireDB.db
      map = db[key]

      if map.is_a? XFireDB::Hashmap
        db.delete(key)
        super(false)
        return "-OK"
      else
        return "-nil"
      end
    end
  end

  class CommandMAdd < XFireDB::Command
    def initialize(cluster, client)
      super(cluster, "MADD", client)
    end

    # MADD <key> <mapkey> <data>
    def exec
      key = @argv[0]
      hkey = @argv[1]
      data = @argv[2]

      return "-Syntax error: MADD <key> <map-key> <data>" unless key and hkey and data
      return forward(key, "MADD #{key} #{hkey} \"#{data}\"") unless @cluster.local_node.shard.include? key

      db = XFireDB.db
      db[key] = XFireDB::Hashmap.new unless db[key].is_a? XFireDB::Hashmap
      db[key][hkey] = data
      super(true)
      return "-OK"
    end
  end

  class CommandMSize < XFireDB::Command
    def initialize(cluster, client)
      super(cluster, "MSIZE", client)
    end

    def exec
      key = @argv[0]

      return "-Syntax error: MSIZE <key>" unless key
      return forward(key, "MSIZE #{key}") unless @cluster.local_node.shard.include? key

      map = XFireDB.db[key]
      return "-nil" unless map.is_a? XFireDB::Hashmap

      rv = "%" + map.size.to_s
      return rv
    end
  end

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
      super(true)
      return "-OK"
    end
  end

  class CommandLClear < XFireDB::Command
    def initialize(cluster, client)
      super(cluster, "LCLEAR", client)
    end

    def exec
      key = @argv[0]

      return forward key, "LCLEAR #{key}" unless @cluster.local_node.shard.include? key
      list = XFireDB.db[key]

      return "-nil" unless list.is_a? XFireDB::List
      XFireDB.db.delete(key)
      super(false)
      return "-OK"
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
        return "-LPOP range invalid: #{idx}" unless range[0].is_i? and range[1].is_i?

        list = XFireDB.db[key]
        return "-nil" unless list.is_a? XFireDB::List

        range[0] = range[0].to_i
        range[1] = range[1].to_i
        range[0] += list.length unless range[0] >= 0
        range[1] += list.length unless range[1] >= 0
        return "-LPOP range invalid: #{idx}" unless range[0] <= range[1]

        rv = Array.new
        n = range[0]

        while range[0] <= range[1]
          v = list.pop n
          rv.push "-nil" if v.nil?
          rv.push "+" + v if v
          range[0] += 1
        end

        # delete the entire list unless there are entry's left
        unless list.length > 0
          XFireDB.db.delete(key) unless list.length > 0
          super(false)
        end

        return rv
      else
        idx = idx.to_i
        list = XFireDB.db[key]

        return "-nil" unless list.is_a? XFireDB::List
        rv = list.pop idx
        rv = "+" + rv

        unless list.length > 0
          XFireDB.db.delete key unless list.length > 0
          super false
        end
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
        return "-LREF range invalid: #{idx}" unless range[0].is_i? and range[1].is_i?

        list = XFireDB.db[key]
        return "-nil" unless list.is_a? XFireDB::List

        range[0] = range[0].to_i
        range[1] = range[1].to_i
        range[0] += list.length unless range[0] >= 0
        range[1] += list.length unless range[1] >= 0

        return "-LREF range invalid: #{idx}" unless range[0] <= range[1]

        rv = Array.new
        while range[0] <= range[1]
          entry = list[range[0]]
          rv.push "+" + entry if entry
          rv.push "-nil" if entry.nil?

          range[0] += 1
        end

        return rv
      else
        idx = idx.to_i
        list = XFireDB.db[key]

        return "-nil" unless list.is_a? XFireDB::List
        return "+" + list[idx]
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

      return "-Syntax error: LSIZE <key>" unless key
      return forward key, "LSIZE #{key}" unless @cluster.local_node.shard.include? key

      list = XFireDB.db[key]
      return "-nil" unless list.is_a? XFireDB::List
      return "%" + list.length
    end
  end

  class CommandLSet < XFireDB::Command
    def initialize(cluster, client)
      super(cluster, "LSET", client)
    end

    # LSET <key> <idx> <data>
    def exec
      key = @argv[0]
      idx = @argv[1]
      data = @argv[3]

      return "-Syntax error: LSET <key> <idx> <data>" unless key and idx and data and idx.is_is?
      return forward key, "LSET #{key} #{idx} \"#{data}\"" unless @cluster.local_node.shard.include? key

      list = XFireDB.db[key]
      return "-nil" unless list.is_a? XFireDB::List

      idx = idx.to_i
      list[idx] = data
      super(true)
      "-OK"
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

      return "-Syntax `GET <key> \"<data>\"'" unless key and data
      if @cluster.local_node.shard.include?(key)
        db[key] = data
        super(true)
      else
        node = @cluster.where_is?(key)
        node = @cluster.nodes[node]
        return node.query(@client, "SET #{key} \"#{data}\"")
      end
      return "-OK"
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
        return "+" + db[@argv[0]]
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
      return "-Access denied" if local_pw.nil? or local_pw != pw
      return "-OK"
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
        super(false)
        return "-OK"
      else
        node = @cluster.where_is?(key)
        node = @cluster.nodes[node]
        return node.query(@client, "DELETE #{key}")
      end
    end
  end
end

