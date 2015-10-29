#
#   XFireDB config
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
  class Config
    attr_reader :port, :config_port, :addr, :cluster, :cluster_config,
      :debug, :problems
    CONFIG_PORT = "port"
    CONFIG_BIND_ADDR = "bind-addr"
    CONFIG_CLUSTER = "cluster-enabled"
    CONFIG_CLUSTER_CONF = "cluster-config-file"
    CONFIG_DEBUG = "debug-mode"
    CONFIG_TEST_OPT = "test-option"

    @port = nil
    @addr = nil
    @cluster = false
    @cluster_config = nil
    @debug = false

    def initialize(file)
      @filename = file
      @problems = 0
      fh = File.open(@filename, "r")
      puts "[config]: config file (#{file}) not found!" unless check_config(fh)
      fh.each do |line|
        next if line.length <= 1
        string = line.split(/(.+?) (.+)/)
        opt = string[1]
        arg = string[2]

        parse opt, arg
      end
      fh.close
    end

    def parse(opt, arg)
      case opt
        # Main config options
      when CONFIG_PORT
        if arg.is_i?
          @port = arg.to_i
          @config_port = @port + 10000
        else
          puts "[config]: #{opt} should be numeric"
        end
      when CONFIG_BIND_ADDR
        @addr = arg
      when CONFIG_CLUSTER
        @cluster = true if arg.eql? "true"
      when CONFIG_CLUSTER_CONF
        puts "[config]: (#{opt}) File \'#{arg}\' not found!" unless check_config(arg)
        @cluster_config = arg
      when CONFIG_DEBUG
        @debug = true if arg.eql? "true"
        # cluster config options
      else
        puts "[config]: unknown option: \'#{opt}\'"
        @problems += 1
      end
    end

    def check_config(file)
      !File.world_readable?(file).nil? && File.exist?(file)
    end
  end

end

