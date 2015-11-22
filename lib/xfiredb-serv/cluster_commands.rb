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
  class ClusterCommand < Command
    attr_reader :cluster

    @subcmd = nil
    @cluster = nil

    @ip = nil
    @port = nil

    def initialize(cluster, client)
      super(cluster, "CLUSTER", client)

      raise IllegalCommandException,
        "Cluster commands not allowed in non-cluster mode" unless XFireDB.config.cluster
      config = XFireDB.config
      @subcmd = @argv.shift

      @ip = client.request.src_ip
      @port = client.request.src_port
    end

    def exec
      rv = case @subcmd.upcase
           when "USERPOISON"
             cluster_userpoison
           when "USERMOD"
             cluster_usermod
           when "USERADD"
             cluster_useradd
           when "USERDEL"
             cluster_userdel
           when "FORGET"
             cluster_forget
           when "MIGRATE"
             cluster_migrate
           when "SLOTS"
             cluster_num_slots
           when "RESHARD"
             cluster_reshard
           when "NODES"
             get_nodes
           when "WHEREIS?"
             where_is?
           when "GETID"
             cluster_get_id
           when "MEET"
             cluster_meet
           when "YOUHAVE?"
             key = @argv[0]
             local = @cluster.local_node
             local.shard.include?(key)
           else
             "Command not known"
           end
    end

    private
    def cluster_userpoison
      user = @argv[0]
      users = XFireDB.users

      return "Syntax error: CLUSTER USERPOISON <user>" unless user
      users.delete user
      "OK"
    end

    def cluster_usermod
      uname = @argv[0]
      action = @argv[1]
      query = "CLUSTER USERMOD #{uname} #{action}"
      synerr = "Syntax error: CLUSTER USERMOD <username> <action>=arg"

      return synerr unless uname and action
      return forward('xfiredb-users', query) unless @cluster.local_node.shard.include? 'xfiredb-users'
      db = XFireDB.db

      user = db['xfiredb-users'][uname]
      return "User not known" if user.nil?

      action = action.split('=').map {|s| s.strip}
      arg = action[1]
      action = action[0]

      return synerr unless arg
      rv = case action.upcase
           when "LEVEL"
             return "CLUSTER USERMOD: argument to LEVEL should be numeric" unless arg.is_i?
             level = arg.to_i
             user = user.split ' '
             user[1] = level
             user = user.join ' '
             db['xfiredb-users'][uname] = user
             @cluster.poison_user uname
             "OK"
           when "PASS"
            hash = BCrypt::Password.create arg
            user = user.split ' '
            user[0] = hash
            user = user.join ' '
            db['xfiredb-users'][uname] = user
            @cluster.poison_user uname
            "OK"
           when "USERNAME"
             if db['xfiredb-users'][arg].nil?
               db['xfiredb-users'][arg] = user
               db['xfiredb-users'].delete uname
               @cluster.poison_user uname
               @cluster.poison_user arg
               "OK"
             else
               "User #{arg} already exists"
             end
           else
             synerr
           end
      return rv
    end

    def cluster_useradd
      uname = @argv[0]
      pass = @argv[1]
      query = "CLUSTER USERADD #{uname} #{pass}"

      return "Syntax error: CLUSTER USERADD username pass" unless uname and pass
      return forward('xfiredb-users', query) unless @cluster.local_node.shard.include? 'xfiredb-users'
      hash = BCrypt::Password.create pass

      users = XFireDB.users
      db = XFireDB.db
      user = XFireDB::User.from_hash(uname, hash)
      db['xfiredb-users'][uname] = "#{hash} #{XFireDB::User::NORMAL}"
      user.level = XFireDB::User::NORMAL
      users[uname] = user
      @cluster.poison_user uname
      "OK"
    end

    def cluster_userdel
      uname = @argv[0]
      query = "CLUSTER USERDEL #{uname}"

      return "Syntax error: USERDEL username" unless uname
      return forward('xfiredb-users', query) unless @cluster.local_node.shard.include? 'xfiredb-users'
      users = XFireDB.users
      db = XFireDB.db

      return "Cannot delete last user" unless db['xfiredb-users'].size > 1
      users.delete uname
      db['xfiredb-users'].delete uname
      @cluster.poison_user uname
      "OK"
    end

    def cluster_num_slots
      rv = Hash.new

      if @port.nil?
        @cluster.nodes.each do |id, node|
          rv[id] = node.cluster_query("CLUSTER SLOTS").to_i
        end
        return rv
      else
        return @cluster.local_node.shard.size
      end
    end

    def cluster_forget
      node = @argv[0]

      return "Syntax error: CLUSTER FORGET <id>" unless node
      @cluster.forget(node)
    end

    # CLUSTER MIGRATE <number-of-slots> <dst-id>
    def cluster_migrate
      num = @argv[0]
      dst = @argv[1]

      return "Incorrect syntax: CLUSTER MIGRATE <num> <dst>" unless num and dst and num.is_i?
      num = num.to_i
      @cluster.local_node.migrate(num, dst) ? "OK" : "Migration failed"
    end

    def cluster_reshard
      num = @argv[0]
      src = @argv[1]
      dst = @argv[2]

      unless num and src and dst and num.is_i?
        return "Incorrect syntax: CLUSTER RESHARD <number-of-slots> <source> <destination>"
      end

      num = num.to_i
      return @cluster.reshard(num, src, dst)
    end

    def where_is?
      key = @argv[0]
      return "Incorrect syntax: CLUSTER WHEREIS? <key>" unless key

      query = "CLUSTER YOUHAVE? #{key}"
      @cluster.nodes.each do |id, value|
        rv = value.cluster_query query
        return id unless rv == "false"
      end

      return "Key not known"
    end

    def cluster_meet
      ip = @argv[0]
      port = @argv[1]
      uname = @argv[2]
      pw = @argv[3]
      return "Incorrect syntax: CLUSTER MEET <ip> <port> <username> <password>" unless ip and port and uname and pw
      return "ERROR: The port should be numeral" unless port.is_i?

      port = port.to_i
      port = port + 10000
      local_ip = @cluster.local_node.addr
      local_port = @cluster.local_node.port

      id = cluster_get_id
      secret = XFireDB.db['xfiredb']['secret']
      sock = XFireDB::SocketFactory.create_socket ip, port
      sock.puts "AUTH"
      sock.puts "#{id} #{secret} #{local_ip} #{local_port}"
      sock.puts "#{uname} #{pw}"
      rv = sock.gets.chomp
      
      if rv.start_with? "ID::"
        rv = rv.split('::')
        rv.shift
        rv = rv.join('')

        db = XFireDB.db
        id = rv
        nodes = db['xfiredb-nodes']
        db['xfiredb-nodes'] = nodes = XFireDB::List.new unless nodes
        port = port - 10000
        nodes.push("#{id} #{ip} #{port}")

        # send a to add the new node in the rest of the network
        @cluster.add_node(id, ip, port)
        gossip = ""
        @cluster.nodes.each do |id, node|
          gossip += " #{id} #{node.addr} #{node.port} #{XFireDB::Cluster::GOSSIP_CHECK}"
        end

        @cluster.gossip_send(gossip.lstrip)
        rv = "OK"
      end
      return rv
    end

    def get_nodes
      rv = Array.new
      @cluster.nodes.each do |key, value|
        rv.push("#{key}\t#{value}")
      end

      return rv
    end

    def cluster_far_id(ip, port)
      @cluster.get_far_id(ip, port)
    end

    def cluster_get_id
      XFireDB.db['xfiredb']['id']
    end
  end
end

