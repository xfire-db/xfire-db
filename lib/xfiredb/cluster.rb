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
          data = node.split(':')
          @nodes[data[0]] = ClusterNode.new(data[1], data[2])
        end
      end

      @nodes[local['id']] = LocalNode.new(addr, port, self)
    end

    def shell
      exit
    end

    def local_node
      @nodes[@local]
    end

    def start
      local = @nodes[@local]
      local.start
    end

    def broadcast(cmd)
      rv = Aray.new

      @nodes.each do |node|
        rv.push node.cluster_query(cmd)
      end

      return rv
    end

    def query(request)
      cmd = XFireDB.cmds
      cmd = cmd[request.cmd]
      return "Command not known" unless cmd
      instance = cmd.new(request.args)
      return instance.exec
    end

    def cluster_query(request)
      cmd = XFireDB::ClusterCommand.new(self, request.args)
      return cmd.exec
    end

  end
end

