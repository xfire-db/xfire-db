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
require 'openssl'
require 'bcrypt'
require 'set'

require 'io/console'

require 'xfiredb-serv/storage_engine'
require 'xfiredb-serv/digest'
require 'xfiredb-serv/illegalkeyexception'
require 'xfiredb-serv/illegalcommandexception'
require 'xfiredb-serv/socket'
require 'xfiredb-serv/user'
require 'xfiredb-serv/cluster'
require 'xfiredb-serv/clusternode'
require 'xfiredb-serv/localnode'
require 'xfiredb-serv/command'
require 'xfiredb-serv/storage_commands'
require 'xfiredb-serv/cluster_commands'
require 'xfiredb-serv/engine'
require 'xfiredb-serv/config'
require 'xfiredb-serv/string'
require 'xfiredb-serv/clusterbus'
require 'xfiredb-serv/workerpool'
require 'xfiredb-serv/client'
require 'xfiredb-serv/log'
require 'xfiredb-serv/shell'
require 'xfiredb-serv/keyshard'
require 'xfiredb-serv/xql'

module XFireDB
  @@config = nil
  @@users = nil
  @@options = nil
  @@engine = nil
  @@commands = {
    "GET" => XFireDB::CommandGet,
    "SET" => XFireDB::CommandSet,
    "DELETE" => XFireDB::CommandDelete,

    "MADD" => XFireDB::CommandMAdd,
    "MREF" => XFireDB::CommandMRef,
    "MCLEAR" => XFireDB::CommandMClear,
    "MDEL" => XFireDB::CommandMDel,
    "MSIZE" => XFireDB::CommandMSize,

    "SADD" => XFireDB::CommandSAdd,
    "SDEL" => XFireDB::CommandSDel,
    "SCLEAR" => XFireDB::CommandSClear,
    "SINCLUDE" => XFireDB::CommandSInclude,

    "LCLEAR" => XFireDB::CommandLClear,
    "LPUSH" => XFireDB::CommandLPush,
    "LPOP" => XFireDB::CommandLPop,
    "LSET" => XFireDB::CommandLSet,
    "LREF" => XFireDB::CommandLRef,
    "LSIZE" => XFireDB::CommandLSize,
    "CLUSTER" => XFireDB::ClusterCommand
  }

  @@illegals = ["xfiredb", "xfiredb-nodes"]
  @@private_keys = ["xfiredb-users"]

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

      if not missing.empty? and @options[:shell] == false
        puts "Mandatory arguments: #{missing.join(', ')}"
        puts opt_parser
        exit
      end
      rescue OptionParser::InvalidOption, OptionParser::MissingArgument
        puts $!.to_s
        puts opt_parser
        exit
      end

    @@config = XFireDB::Config.new(@options[:config])
    case @options.action
    when "stop"
      opts = {:ARGV => [@options.action], :dir_mode => :script, :dir => @@config.pid_file }
      Daemons.run_proc('xfiredb', opts) do
      end
    when "status"
    else
      @@options = @options
      XFireDB.create
      XFireDB::Shell.start(XFireDB.engine) if @options[:shell]
      unless missing.empty?
        XFireDB.exit
        exit
      end
      XFireDB.create_users
      server = XFireDB::Cluster.new(@@config.addr, @@config.port)
      XFireDB.exit
      server.start
    end

  end

  def XFireDB.users
    @@users
  end

  def XFireDB.create_users
    map = XFireDB.db['xfiredb-users']
    @@users = Hash.new
    return if map.nil?

    map.each do |username, hash|
      user = XFireDB::User.from_hash(username, hash)
      @@users[username] = user
    end
  end

  def XFireDB.illegal_key?(key)
    @@illegals.include? key
  end

  def XFireDB.private_key?(key)
    @@private_keys.include? key
  end

  def XFireDB.config
    @@config
  end

  def XFireDB.options
    @@options
  end

  def XFireDB.db
    @@engine.db
  end

  def XFireDB.cmds
    @@commands
  end

  def XFireDB.engine
    @@engine
  end

  def XFireDB.create
    @@engine = XFireDB::Engine.new
  end

  def XFireDB.exit
    @@engine.exit if @@engine
  end

  def XFireDB.print(str)
    print str
    $stdout.flush
  end

  def XFireDB.worker_num
    @options[:workers]
  end
end

