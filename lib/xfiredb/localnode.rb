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

    def to_s
      "myself, #{@addr} #{@port}"
    end

    def start_clusterbus
      serv = TCPServer.new(@config.addr, @config.port + 10000)
      Thread.new do
        loop do
          Thread.start(serv.accept) do |request|
            begin
            family, port, host, ip = request.peeraddr;
            type = request.gets.chomp
            if not @cluster.request_from_node?(ip) and type.upcase != "AUTH"
              request.puts "Access denied"
              request.close
              next
            end
            reply = case type.upcase
            when "AUTH"
              source = request.gets.chomp
              auth = request.gets.chomp
              @cluster.auth_node(ip, source, auth)
            when "QUERY"
              dom, port, host, ip = request.peeraddr
              client = XFireDB::Client.from_stream(request)
              client.read(ip, port)
              @cluster.cluster_query(client)
            when "PING"
              "PONG"
            when "GOSSIP"
              gossip = request.gets.chomp
              gossip = gossip.split(' ')
              # 0 => node the gossip is about
              # 1 => node IP
              # 2 => node port
              # 3 => node status code, or message
              @cluster.gossip(gossip)
              "OK"
            end

            request.puts reply
            request.close
            rescue Exception => e
              puts e
              puts e.backtrace
            end
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

    def migrate(num, dst)
      slots, keys = @shard.reshard(num)
      # TODO: migrate the given keys to dst
    end

    def gossip(gossip)
      # this node is sending the gossip and therefore
      # already knows the contents. No action required.
    end

    def query(query)
    end
  end
end

