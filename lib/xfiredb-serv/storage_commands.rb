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

#
module XFireDB
  # SET INCLUDE handler
  class CommandSInclude < XFireDB::Command

    # Create a new instance of CommandSInclude.
    #
    # @param [Cluster] cluster Cluster object.
    # @param [Client] client Client object.
    # @return [CommandSInclude] A new CommandSInclude object.
    def initialize(cluster, client)
      super(cluster, "SINCLUDE", client)
    end

    # Excute the command.
    #
    # @return [String] Reply to client.
    def exec
      key = @argv.shift

      return "-Syntax error: SINCLUDE <key> <set-key1> <set-key2> ..." unless key and @argv.length > 0
      return forward(key, "SINCLUDE #{key} #{@argv.map(&:quote).join(' ')}") unless @cluster.local_node.shard.include? key

      set = XFireDB.db[key]
      return "-nil" unless set.is_a? XFireDB::Set

      rv = Array.new
      @argv.each do |hkey|
        rv.push "&" + set.include?(hkey).to_s
      end

      return rv
    end
  end

  # Hanlder for SCLEAR. Clear an entire set.
  class CommandSClear < XFireDB::Command
    # Create a new SClear handler.
    #
    # @param [Cluster] cluster Cluster object.
    # @param [Client] client Client object.
    def initialize(cluster, client)
      super(cluster, "SCLEAR", client)
    end

    # Excute the command.
    #
    # @return [String] Reply to client.
    def exec
      key = @argv[0]

      return "-Syntax error: SCEAR <key>" unless key and @argv.length > 0
      return forward(key, "SCLEAR #{key}") unless @cluster.local_node.shard.include? key

      set = XFireDB.db[key]
      return "-nil" unless set.is_a? XFireDB::Set

      XFireDB.db.delete(key)
      super(false)
      return "-OK"
    end
  end

  # SDEL handler.
  class CommandSDel < XFireDB::Command
    # Create a new SDel handler.
    #
    # @param [Cluster] cluster Cluster object.
    # @param [Client] client Client object.
    def initialize(cluster, client)
      super(cluster, "SDEL", client)
    end

    # Excute the command.
    #
    # @return [String] Reply to client.
    def exec
      key = @argv.shift

      return "-Syntax error: SDEL <key> <set-key1> <set-key2> ..." unless key and @argv.length > 0
      return forward(key, "SDEL #{key} #{@argv.map(&:quote).join(' ')}") unless @cluster.local_node.shard.include? key

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

  # SADD handler
  class CommandSAdd < XFireDB::Command
    # Create a new SADD handler.
    #
    # @param [Cluster] cluster Cluster object.
    # @param [Client] client Client object.
    def initialize(cluster, client)
      super(cluster, "SADD", client)
    end

    # Excute the command.
    #
    # @return [String] Reply to client.
    def exec
      key = @argv.shift

      return "-Syntax error: SADD <key> <set-key1> <set-key2> ..." unless key and @argv.length > 0
      return forward(key, "SADD #{key} #{@argv.map(&:quote).join(' ')}") unless @cluster.local_node.shard.include? key

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

  # MDEL handler
  class CommandMDel < XFireDB::Command
    # Create a new MDEL handler.
    #
    # @param [Cluster] cluster Cluster object.
    # @param [Client] client Client object.
    def initialize(cluster, client)
      super(cluster, "MDEL", client)
    end

    # Excute the command.
    #
    # @return [String] Reply to client.
    def exec
      key = @argv.shift

      return "-Syntax error: MDEL <key> <hkey1> <hkey2> ..." unless key
      return forward(key, "MDEL #{key} #{@argv.join(' ')}") unless @cluster.local_node.shard.include? key

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

  # MREF hanlder
  class CommandMRef < XFireDB::Command
    # Create a new MREF handler.
    #
    # @param [Cluster] cluster Cluster object.
    # @param [Client] client Client object.
    def initialize(cluster, client)
      super(cluster, "MREF", client)
    end

    # Excute the command.
    #
    # @return [String] Reply to client.
    def exec
      key = @argv.shift

      return "-Syntax error: MREF <key> <hkey1> <hkey2> ..." unless key
      return forward(key, "MREF #{key} #{@argv.join(' ')}") unless @cluster.local_node.shard.include? key

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

  # MCLEAR handler
  class CommandMClear < XFireDB::Command
    # Create a new MCLEAR handler.
    #
    # @param [Cluster] cluster Cluster object.
    # @param [Client] client Client object.
    def initialize(cluster, client)
      super(cluster, "MCLEAR", client)
    end

    # Excute the command.
    #
    # @return [String] Reply to client.
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

  # MADD handler
  class CommandMAdd < XFireDB::Command
    # Create a new MADD handler.
    #
    # @param [Cluster] cluster Cluster object.
    # @param [Client] client Client object.
    def initialize(cluster, client)
      super(cluster, "MADD", client)
    end

    # Excute the command.
    #
    # @return [String] Reply to client.
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

  # MSIZE handler
  class CommandMSize < XFireDB::Command
    # Create a new MSIZE handler.
    #
    # @param [Cluster] cluster Cluster object.
    # @param [Client] client Client object.
    def initialize(cluster, client)
      super(cluster, "MSIZE", client)
    end

    # Excute the command.
    #
    # @return [String] Reply to client.
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

  # LPUSH handler
  class CommandLPush < XFireDB::Command
    # Create a new LPUSH handler.
    #
    # @param [Cluster] cluster Cluster object.
    # @param [Client] client Client object.
    def initialize(cluster, client)
      super(cluster, "LPUSH", client)
    end

    # Excute the command.
    #
    # @return [String] Reply to client.
    def exec
      key = @argv[0]
      data = @argv[1]

      return "-Syntax error: LPUSH <key> <data>" unless key and data
      return forward(key, "LPUSH #{key} \"#{data}\"") unless @cluster.local_node.shard.include? key

      db = XFireDB.db
      db[key] = XFireDB::List.new unless db[key].is_a? XFireDB::List
      db[key].push data
      super(true)
      return "-OK"
    end
  end

  # LCLEAR handler
  class CommandLClear < XFireDB::Command
    # Create a new LCLEAR handler.
    #
    # @param [Cluster] cluster Cluster object.
    # @param [Client] client Client object.
    def initialize(cluster, client)
      super(cluster, "LCLEAR", client)
    end

    # Excute the command.
    #
    # @return [String] Reply to client.
    def exec
      key = @argv[0]

      return "-Syntax error: LCLEAR <key>" unless key
      return forward key, "LCLEAR #{key}" unless @cluster.local_node.shard.include? key
      list = XFireDB.db[key]

      return "-nil" unless list.is_a? XFireDB::List
      XFireDB.db.delete(key)
      super(false)
      return "-OK"
    end
  end

  # LPOP handler
  class CommandLPop < XFireDB::Command
    # Create a new LPOP handler.
    #
    # @param [Cluster] cluster Cluster object.
    # @param [Client] client Client object.
    def initialize(cluster, client)
      super(cluster, "LPOP", client)
    end

    # Excute the command.
    #
    # @return [String] Reply to client.
    def exec
      key = @argv[0]
      idx = @argv[1]

      return "-Syntax error: LPOP <key> <idx | range>" unless key and idx
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

  # LREF handler
  class CommandLRef < XFireDB::Command
    # Create a new LREF handler.
    #
    # @param [Cluster] cluster Cluster object.
    # @param [Client] client Client object.
    def initialize(cluster, client)
      super(cluster, "LREF", client)
    end

    # Excute the command.
    #
    # @return [String] Reply to client.
    def exec
      key = @argv[0]
      idx = @argv[1]

      return "-Syntax error: LREF <key> <idx | range>" unless key and idx
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

  # LSIZE handler
  class CommandLSize < XFireDB::Command
    # Create a new LSIZE handler.
    #
    # @param [Cluster] cluster Cluster object.
    # @param [Client] client Client object.
    def initialize(cluster, client)
      super(cluster, "LSIZE", client)
    end

    # Excute the command.
    #
    # @return [String] Reply to client.
    def exec
      key = @argv[0]

      return "-Syntax error: LSIZE <key>" unless key
      return forward key, "LSIZE #{key}" unless @cluster.local_node.shard.include? key

      list = XFireDB.db[key]
      return "-nil" unless list.is_a? XFireDB::List
      return "%" + list.length
    end
  end

  # LSET handler
  class CommandLSet < XFireDB::Command
    # Create a new LSET handler.
    #
    # @param [Cluster] cluster Cluster object.
    # @param [Client] client Client object.
    def initialize(cluster, client)
      super(cluster, "LSET", client)
    end

    # Excute the command.
    #
    # @return [String] Reply to client.
    def exec
      key = @argv[0]
      idx = @argv[1]
      data = @argv[2]

      return "-Syntax error: LSET <key> <idx> <data>" unless key and idx and data and idx.is_i?
      return forward key, "LSET #{key} #{idx} \"#{data}\"" unless @cluster.local_node.shard.include? key

      list = XFireDB.db[key]
      return "-nil" unless list.is_a? XFireDB::List

      idx = idx.to_i
      list[idx] = data
      super(true)
      "-OK"
    end
  end

  # SET handler
  class CommandSet < XFireDB::Command
    # Create a new SET handler.
    #
    # @param [Cluster] cluster Cluster object.
    # @param [Client] client Client object.
    def initialize(cluster, client)
      super(cluster, "SET", client)
    end

    # Excute the command.
    #
    # @return [String] Reply to client.
    def exec
      key = @argv[0]
      data = @argv[1]
      db = XFireDB.db

      return "-Syntax `SET <key> \"<data>\"'" unless key and data
      return forward(key, "SET #{key} \"#{data}\"") unless @cluster.local_node.shard.include? key
      db[key] = data
      super(true)
      return "-OK"
    end
  end

  # GET handler
  class CommandGet < XFireDB::Command
    # Create a new GET handler.
    #
    # @param [Cluster] cluster Cluster object.
    # @param [Client] client Client object.
    def initialize(cluster, client)
      super(cluster, "GET", client)
    end

    # Excute the command.
    #
    # @return [String] Reply to client.
    def exec
      db = XFireDB.db
      key = @argv[0]
      return "-Incorrect syntax: GET <key>" unless @argv[0]

      return forward(key, "GET #{key}") unless @cluster.local_node.shard.include?(@argv[0])
      val = db[@argv[0]]
      return "-nil" if val.nil?
      return "+" + db[@argv[0]]
    end
  end

  # DELETE handler
  class CommandDelete < XFireDB::Command
    # Create a new DELETE handler.
    #
    # @param [Cluster] cluster Cluster object.
    # @param [Client] client Client object.
    def initialize(cluster, client)
      super(cluster, "DELETE", client)
    end

    # Excute the command.
    #
    # @return [String] Reply to client.
    def exec
      key = @argv[0]
      db = XFireDB.db

      return "-Incorrect syntax: DELETE <key>" unless key
      forward(key, "DELETE #{key}") unless @cluster.local_node.shard.include?(key)
      db.delete(key)
      super(false)
      return "-OK"
    end
  end
end

