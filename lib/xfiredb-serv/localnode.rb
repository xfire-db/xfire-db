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

    def auth_cluster_node(secret)
      map = XFireDB.db['xfiredb']
      known = map['secret']

      return known == secret ? true : false
    end

    def start_clusterbus
      server = TCPServer.new(@config.addr, @config.port + 10000)
      if @config.ssl
        context = OpenSSL::SSL::SSLContext.new
        context.cert = OpenSSL::X509::Certificate.new(File.open(@config.ssl_cert))
        context.key = OpenSSL::PKey::RSA.new(File.open(@config.ssl_key))
        serv = OpenSSL::SSL::SSLServer.new(server, context)
      else
        serv = server
      end

      Thread.new do
        loop do
          Thread.start(serv.accept) do |request|
            begin
            family, port, host, ip = request.peeraddr;
            type = request.gets.chomp
            if not type.upcase == "AUTH" and @config.cluster_auth
              auth_cmd = type.split(' ')
              if not auth_cmd[0].upcase == "AUTH"
                request.puts "Access denied"
                request.close
                next
              else
                db = XFireDB.db
                # does the user exist
                unless auth_cluster_node(auth_cmd[1])
                  request.puts "Access denied"
                  request.close
                  next
                end
              end
              request.puts "OK"
              type = request.gets.chomp
            end

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
            when "ADDSLOTS"
              slots = request.gets.chomp
              @cluster.add_slots(slots.split(' '))
              "OK"
            when "QUERY"
              dom, port, host, ip = request.peeraddr
              client = XFireDB::Client.from_stream(request, @cluster, true)
              client.read(ip, port)
              @cluster.cluster_query(client)
            when "PING"
              "PONG"
            when "UGOSSIP"
              gossip = request.gets.chomp
              gossip = gossip.split(' ')
              @cluster.user_gossip(gossip)
              "OK"
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

            if reply.is_a? Array
              reply.each do |v|
                request.puts v
              end
            else
              request.puts reply
            end
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
      ontop = @config.debug
      opts = {:ARGV => [@options.action], :ontop => ontop, :dir_mode => :normal,
              :dir => @config.pid_file, :log_output => true}

      # Exit the engine before we fork
      XFireDB.exit
      Daemons.run_proc('xfiredb', opts) do
        XFireDB.create
        XFireDB.engine.start

        self.start_clusterbus
        @engine = XFireDB.engine
        @pool = XFireDB::WorkerPool.new(XFireDB.worker_num, @cluster)
        XFireDB::Log.write(XFireDB::Log::LOG_INIT + "Configuration file loaded " \
                           "(#{@config.problems} problems)\n", false, false)
        XFireDB::Log.write(XFireDB::Log::LOG_INIT + "Initialisation complete, " \
                          "database started!\n", false, false)
        server = TCPServer.new(@config.addr, @config.port)
        if @config.ssl
          context = OpenSSL::SSL::SSLContext.new
          context.cert = OpenSSL::X509::Certificate.new(File.open(@config.ssl_cert))
          context.key = OpenSSL::PKey::RSA.new(File.open(@config.ssl_key))
          serv = OpenSSL::SSL::SSLServer.new(server, context)
        else
          serv = server
        end

        Signal.trap('TERM') {
          puts "[signal]: TERMINATE received, stopping"
          XFireDB.shutdown
          serv.close
        }

        while XFireDB.running
          begin
            @pool.push(serv.accept)
          rescue IO::WaitReadable, Errno::EINTR
            IO.select([serv])
            retry
          rescue Errno::EBADF
            break
          end
        end

        XFireDB.save
        exit
      end
    end

    def migrate(num, dst)
      db = XFireDB.db
      slots, keys = @shard.reshard(num)

      if @shard.slots.size > 0
        db['xfiredb']['shards'] = @shard.slots.to_a.join(':')
      else
        db['xfiredb'].delete('shards')
      end

      dst = @cluster.nodes[dst]
      dst.add_slots(slots.to_a.join(' '))

      keys.each do |key|
        val = db[key]
        case val
        when String
          dst.cluster_query("SET #{key} \"#{val}\"")
        when XFireDB::List
          val.each do |entry|
            dst.cluster_query("LPUSH #{key} \"#{entry}\"")
          end
        when XFireDB::Hashmap
          val.each do |k, v|
            dst.cluster_query("MADD #{key} #{k} \"#{v}\"")
          end
        when XFireDB::Set
          val.each do |k|
            dst.cluster_query("SADD #{key} \"#{k}\"")
          end
        end
        db.delete(key)
      end

      GC.start
      return true
    end

    def gossip(gossip)
      # this node is sending the gossip and therefore
      # already knows the contents. No action required.
    end

    def query(query)
    end
  end
end

