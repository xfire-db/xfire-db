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
  class Cluster
    attr_reader :nodes, :local

    GOSSIP_ADD = "100"
    GOSSIP_DEL = "101"
    GOSSIP_MOVE = "102"
    GOSSIP_CHECK = "103"

    @nodes = nil
    @cluster_config = nil
    @engine = nil
    @local = nil

    def initialize(addr, port)
      @nodes = Hash.new

      db = XFireDB.db
      nodes = db['xfiredb-nodes']
      local = db['xfiredb']

      if local.nil?
        puts "Please setup XFireDB using the Shell (--shell) first" 
        exit
      end
      @local = local['id']

      if nodes
        nodes.each do |node|
          data = node.split(' ')
          @nodes[data[0]] = ClusterNode.new(data[1], data[2].to_i)
        end
      end

      @nodes[local['id']] = LocalNode.new(addr, port, self)
    end

    def add_slots(slots)
      local_node.shard.add_slots(slots)
      db = XFireDB.db
      dbslots = db['xfiredb']['shards']

      unless dbslots.nil?
        dbslots = "#{slots.join(':')}:#{dbslots}"
      else
        dbslots = slots.join(':')
      end

      db['xfiredb']['shards'] = dbslots
      "OK"
    end

    def add_node(id, ip, port)
      @nodes[id] = ClusterNode.new(ip, port)
    end

    def remove_node(id)
      node = @nodes.delete(id)
      return unless node;

      dbnodes = XFireDB.db['xfiredb-nodes']
      idx = 0
      dbnodes.each do |info|
        __id, ip, port = info.split(' ')
        break if id == __id
        idx += 1
      end

      dbnodes.pop(idx)
    end

    def local_node
      @nodes[@local]
    end

    def start
      local = @nodes[@local]
      local.start
    end

    def broadcast(cmd)
      rv = Array.new

      @nodes.each do |node|
        rv.push node.cluster_query(cmd)
      end

      return rv
    end

    def auth_node(ip, source, node)
      source = source.split(' ')
      node = node.split(' ')

      username = node[0]
      password = node[1]
      return "Incorrect syntax: CLUSTER AUTH <username> <password>" unless username and password

      db = XFireDB.db
      map = db['xfiredb']
      local_pw = map["user::#{username}"]
      return "INCORRECT" if local_pw.nil?
      local_pw = BCrypt::Password.new(local_pw)

      return "INCORRECT" if local_pw.nil? or local_pw != password

      db['xfiredb-nodes'] ||= XFireDB::List.new
      db['xfiredb-nodes'].push("#{source[0]} #{source[1]} #{source[2]}")
      add_node(source[0], source[1], source[2].to_i)
      return "OK"
    end

    def where_is?(key)
      client = XFireDB::Client.new(nil, "CLUSTER WHEREIS? #{key}")
      self.query(client)
    end

    def reshard(num, src, dst)
      if src == "all"
        dstnode = @nodes[dst]
        return "Destination node not known" if dstnode.nil?

        shard_dist = Hash.new

        @nodes.each do |id, node|
          shard_dist[id] = node.shards.to_i
        end

        total = 16384 - shard_dist[dst]

        return "Reshard operation not possible" if num > total

        shard_dist.delete(dst)

        while num > 0
          shard_dist.delete_if {|id, shards| shards == 0}

          per_node = num / shard_dist.size
          per_node = num if per_node == 0
          # sort the shard distribution in descending order
          shard_dist = shard_dist.sort_by { |id, num| num }.reverse

          # take shards from the most populated nodes first
          x = 0
          shard_dist.each_with_object({}) { |(id, shards), hash|
            if per_node <= shards
              hash[id] = shards - per_node
              x = per_node
            else
              hash[id] = 0
              x = shards
            end

            @nodes[id].migrate_query(x, dst)
            num -= x
            break if num <= 0
          }
        end
      else
        srcnode = @nodes[src]
        dstnode = @nodes[dst]

        return "Source or destination not known" unless srcnode and dstnode
        srcnode.migrate_query(num, dst)
      end

      "OK"
    end

    def forget(node_id)
      node = @nodes[node_id]
      return "Node not known" unless node

      num = node.shards.to_i
      while num > 0
        per_node = num / (@nodes.size - 1)
        per_node = num if per_node == 0

        @nodes.each do |id, cnode|
          next if node_id == id
          node.migrate_query(per_node, id)
          num -= per_node
          break unless num > 0
        end
      end

      gsp = "#{node_id} #{node.addr} #{node.port} #{XFireDB::Cluster::GOSSIP_DEL}"
      self.gossip_send(gsp)
      self.remove_node(node_id)
      "OK"
    end

    def gossip_send(gossip)
      @nodes.each do |id, node|
        node.gossip(gossip)
      end
    end

    # Parse gossip packet
    # latest[n+0] = node id
    # latest[n+0] = node ip
    # latest[n+0] = node port
    # latest[n+0] = node state/msg
    def gossip(latest)
      len = latest.length
      len = len / 4

      len.times do |idx|
        i = idx * 4
        id = latest[i]
        ip = latest[i+1]
        port = latest[i+2]
        msg = latest[i+3]
        node = @nodes[id]
        dbnodes = XFireDB.db['xfiredb-nodes']

        case msg
        when GOSSIP_ADD
          if node.nil?
            dbnodes.push("#{id} #{ip} #{port}")
            self.add_node(id, ip, port.to_i)
          end
        when GOSSIP_DEL
          self.remove_node(id) unless node.nil?
        when GOSSIP_MOVE
          self.remove(id) unless node.nil?
          self.add_node(id, ip, port)
        when GOSSIP_CHECK
          if node.nil?
            dbnodes.push("#{id} #{ip} #{port}")
            self.add_node(id, ip, port.to_i)
          end
        end
      end
    end

    def request_from_node?(ip)
      @nodes.each do |id, node|
        return true if node.addr == ip
      end

      return false
    end

    def get_far_id(ip, port)
      sock = XFireDB::SocketFactory.create_cluster_socket ip, port
      sock.puts "QUERY"
      sock.puts "CLUSTER GETID"
      rv = sock.gets.chomp
      sock.close
      return rv
    end

    def query(client)
      cmd = XFireDB.cmds
      cmd = cmd[client.request.cmd]
      return "Command not known" unless cmd
      instance = cmd.new(self, client)
      instance.exec
    end

    def cluster_query(client)
      cmd = XFireDB::ClusterCommand.new(self, client)
      return cmd.exec
    end

  end
end

