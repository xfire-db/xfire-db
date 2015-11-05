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

    def initialize(cluster, argv, ip = nil, port = nil)
      super("CLUSTER", argv)
      @subcmd = @argv.shift
      @cluster = cluster

      @ip = ip
      @port = port
    end

    def exec
      rv = case @subcmd.upcase
           when "AUTH"
             cluster_auth
           when "WHEREIS"
             "OK"
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
    def cluster_meet
      ip = @argv[0]
      port = @argv[1]
      uname = @argv[2]
      pw = @argv[3]
      return "Incorrect syntax: CLUSTER MEET <ip> <port> <username> <password>" unless ip and port and uname and pw
      return "ERROR: The port should be numeral" unless port.is_i?
      port = port.to_i
      port = port + 10000
      sock = TCPSocket.new(ip, port)
      sock.puts "QUERY"
      sock.puts "CLUSTER AUTH #{uname} #{pw}"
      rv = sock.gets.chop

      if rv == "OK"
        db = XFireDB.db
        id = cluster_far_id(ip, port)
        nodes = db['xfiredb-nodes']
        db['xfiredb-nodes'] = nodes = XFireDB::List.new unless nodes
        port = port - 10000
        nodes.push("#{id}:#{ip}:#{port}")
      end
      return rv
    end

    def cluster_far_id(ip, port)
      sock = TCPSocket.new(ip, port)
      sock.puts "QUERY"
      sock.puts "CLUSTER GETID"
      sock.gets.chop
    end

    def cluster_get_id
      XFireDB.db['xfiredb']['id']
    end

    def cluster_auth
      username = @argv[0]
      password = @argv[1]
      return "Incorrect syntax: CLUSTER AUTH <username> <password>" unless username and password

      db = XFireDB.db
      map = db['xfiredb']
      local_pw = map["user::#{username}"]
      return "INCORRECT" if local_pw.nil?
      local_pw = BCrypt::Password.new(local_pw)

      return "INCORRECT" if local_pw.nil? or local_pw != password
      return "OK"
    end
  end
end

