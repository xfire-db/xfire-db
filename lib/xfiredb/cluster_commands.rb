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

    def initialize(cluster, argv)
      super("CLUSTER", argv)
      @subcmd = @argv.shift
      @cluster = cluster
    end

    def exec
      rv = case @subcmd.upcase
           when "WHEREIS"
             "OK"
           when "YOUHAVE?"
             key = @argv[0]
             local = @cluster.local_node
             local.shard.include?(key)
           else
             "Command not known"
           end
    end
  end
end

