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

require 'xfiredb/storage_engine'
require 'xfiredb/engine'
require 'xfiredb/server'
require 'xfiredb/config'
require 'xfiredb/string'
require 'xfiredb/clusterbus'
require 'xfiredb/workerpool'

module XFireDB
  def XFireDB.start(cmdargs)
    @options = OpenStruct.new
    @options.config = nil
    @options.workers = 2

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

      opts.on("-h", "--help", "Display this help message") do
        puts opts
        exit
      end
    end

    begin
      opt_parser.parse!(cmdargs)
      mandatory = [:config]
      missing = mandatory.select{ |param| @options[param].nil? }
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

    server = Server.new(@options[:config])
    server.start
    server.stop
  end

  def XFireDB.worker_num
    @options[:workers]
  end
end

