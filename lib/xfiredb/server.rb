#
#   XFireDB server
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
  class Server
    attr_reader :config
    attr_reader :store

    def initialize(opts, conf)
      @config = XFireDB::Config.new(conf)
      @options = opts
    end

    def stop
      @store.exit
    end

    def shell
      @store = XFireDB::Engine.new
      XFireDB::Shell.start(@store)
      @store.exit
    end

    def start
      puts "[init]: XFireDB started in debugging mode" if @config.debug

      Daemons.run_proc('xfiredb', :ARGV => [@options.action]) do
      begin
        @store = XFireDB::Engine.new
          @bus = XFireDB::ClusterBus.new if @config.cluster
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
          self.stop
        end
      end
    end
  end
end

