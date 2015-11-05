#
#   XFireDB LocalNode class
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
  class LocalNode < ClusterNode
    attr_reader :engine, :shard

    @engine = nil
    @shard = nil
    @config = nil
    @options = nil
    @cluster = nil

    def initialize(addr, port, cluster)
      super(addr, port)
      @engine = XFireDB.engine
      @shard = XFireDB::KeyShard.new
      @config = XFireDB.config
      @options = XFireDB.options
      @cluster = cluster
      
      db = XFireDB.db
      slots = db['xfiredb']['shards']
      return unless slots

      slot_ary = slots.split(':')
      slot_ary.each do |single|
        @shard.add_slot(single)
      end
    end

    def start_clusterbus
      serv = TCPServer.new(@config.addr, @config.port + 10000)
      Thread.new do
        loop do
          Thread.start(serv.accept) do |request|
            type = request.gets.chop
            reply = case type.upcase
            when "QUERY"
              query = XFireDB::XQL.parse(request.gets.chop)
              dom, port, host, ip = request.peeraddr
              query.src_ip = ip
              query.src_port = port
              @cluster.cluster_query(query)
            when "PING"
              "PONG"
            when "GOSSIP"
              gossip = request.gets.chop
              gossip = gossip.split(':')
              # 0 => node the gossip is about
              # 1 => node IP
              # 2 => node port
              # 3 => node status code, or message
              @cluster.gossip(gossip)
              "OK"
            end

            request.puts reply
            request.close
          end
        end
      end
    end

    def start
      puts "[init]: XFireDB started in debugging mode" if @config.debug
      Daemons.run_proc('xfiredb', {:ARGV => [@options.action], :ontop => true, :log_output => true}) do
      self.start_clusterbus
      begin
        XFireDB.create
        @engine = XFireDB.engine
        @pool = XFireDB::WorkerPool.new(XFireDB.worker_num, @cluster)
        XFireDB::Log.write(XFireDB::Log::LOG_INIT + "Configuration file loaded " \
                           "(#{@config.problems} problems)\n", false, false)
        XFireDB::Log.write(XFireDB::Log::LOG_INIT + "Initialisation complete, " \
                          "database started!\n", false, false)
        serv = TCPServer.new(@config.addr, @config.port)
        loop do
          @pool.push(serv.accept)
        end
        rescue SystemExit => e
          XFireDB.stop if @options.action == "stop"
        end
      end
    end

    def cluster_query(query)
    end

    def query(query)
    end
  end
end

