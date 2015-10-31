#
#   XFireDB module
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

require 'pp'
require 'ostruct'
require 'optparse'
require 'thread'
require 'socket'
require 'daemons'
require 'securerandom'
require 'bcrypt'
require 'set'

require 'io/console'

require 'xfiredb/storage_engine'
require 'xfiredb/clusternode'
require 'xfiredb/localnode'
require 'xfiredb/digest'
require 'xfiredb/engine'
require 'xfiredb/server'
require 'xfiredb/config'
require 'xfiredb/string'
require 'xfiredb/clusterbus'
require 'xfiredb/workerpool'
require 'xfiredb/client'
require 'xfiredb/log'
require 'xfiredb/shell'
require 'xfiredb/keyshard'

module XFireDB
  def XFireDB.start(cmdargs)
    @options = OpenStruct.new
    @options.config = nil
    @options.workers = 2
    @options.action = nil
    @options.shell = false

    opt_parser = OptionParser.new do |opts|
      opts.banner = "Usage: xfiredb [options] -c FILE"
      opts.on("-c", "--config FILE",
              "Path to the XFireDB server configuration file") do |conf|
        @options.config = conf
      end

      opts.on("-w", "--workers NUM",
              "Number of threads used to accept incoming connections") do |num|
        @options.workers = num || 2
      end

      opts.on("-s", "--shell",
              "Drop into the XFireDB shell before starting the server") do |shell|
        @options.shell = true
      end

      opts.on("-a", "--action [start|stop|status]",
              "Server action") do |action|
        @options.action = action
      end

      opts.on("-h", "--help", "Display this help message") do
        puts opts
        exit
      end
    end

    begin
      opt_parser.parse!(cmdargs)
      mandatory = [:config, :action]
      missing = mandatory.select { |param|
        if (param == :config and @options[:action] == "start") or param == :action
          @options[param].nil?
        end
      }
      unless missing.empty?

        puts "Mandatory arguments: #{missing.join(', ')}"
        puts opt_parser
        exit
      end
      rescue OptionParser::InvalidOption, OptionParser::MissingArgument
        puts $!.to_s
        puts opt_parser
        exit
      end

    server = Server.new(@options, @options[:config])
    server.shell if @options[:shell]
    server.start
  end

  def XFireDB.print(str)
    print str
    $stdout.flush
  end

  def XFireDB.worker_num
    @options[:workers]
  end
end

