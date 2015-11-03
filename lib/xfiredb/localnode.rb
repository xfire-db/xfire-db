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

    def initialize(addr, port)
      super(addr, port)
      @engine = XFireDB.engine
      @shard = XFireDB::KeyShard.new
      @config = XFireDB.config
      @options = XFireDB.options
    end

    def start
      puts "[init]: XFireDB started in debugging mode" if @config.debug
      Daemons.run_proc('xfiredb', :ARGV => [@options.action]) do
      begin
        XFireDB.create
        @pool = XFireDB::WorkerPool.new(XFireDB.worker_num, self)
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

