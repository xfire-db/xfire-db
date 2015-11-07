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

    def shell
      exit
    end

    def add_node(id, ip, port)
      @nodes[id] = ClusterNode.new(ip, port)
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
      "OK"
    end

    def request_from_node?(ip)
      @nodes.each do |id, node|
        return true if node.addr == ip
      end

      return false
    end

    def get_far_id(ip, port)
      sock = TCPSocket.new(ip, port)
      sock.puts "QUERY"
      sock.puts "CLUSTER GETID"
      sock.gets.chomp
    end

    def query(client)
      cmd = XFireDB.cmds
      cmd = cmd[client.request.cmd]
      return "Command not known" unless cmd
      instance = cmd.new(self, client)
      return instance.exec
    end

    def cluster_query(client)
      cmd = XFireDB::ClusterCommand.new(self, client)
      return cmd.exec
    end

  end
end

