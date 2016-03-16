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
require 'xfiredb-serv/workerpool'
require 'xfiredb-serv/client'
require 'xfiredb-serv/log'
require 'xfiredb-serv/shell'
require 'xfiredb-serv/keyshard'
require 'xfiredb-serv/xql'

# XFireDB server module. All XFireDB related classes are found within this module.
#
# @author Michel Megens
# @since 0.0.1
module XFireDB
  VERSION = '1.0.0'

  @@config = nil
  @@users = nil
  @@options = nil
  @@engine = nil
  @@running = true

  # List of available command handles
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

  # Keys which cannot be acced by clients and are not moved by
  # a reshard operation.
  @@illegals = ["xfiredb", "xfiredb-nodes"]

  # List of keys that are not accesible by clients, but are moved
  # by a reshard operation.
  @@private_keys = ["xfiredb-users"]

  # Get a list of keys that are needed during the preinit stage. These
  # keys are loaded first from the harddisk.
  #
  # @return [Set] A list of keys needed during initalisation.
  def XFireDB.preinit_keys
    tmp = @@illegals + @@private_keys
    tmp.to_set
  end

  # Start the XFireDB server / cluster node.
  #
  # @param cmdargs [String] Command line arguments
  # @return nil
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

      opts.on("-v", "--version",
              "Print the current version of the XFireDB server") do
        puts "XFireDB #{VERSION}"
        return
      end

      opts.on("-w", "--workers NUM",
              "Number of threads used to accept incoming connections") do |num|
        num = num.to_i unless num.nil?
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
        return
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
        return
      end
      rescue OptionParser::InvalidOption, OptionParser::MissingArgument
        puts $!.to_s
        puts opt_parser
        return
      end

    @@config = XFireDB::Config.new(@options[:config])
    case @options.action
    when "stop"
      opts = {:ARGV => [@options.action], :dir_mode => :normal, :dir => @@config.pid_file, :log_output => true }
      Daemons.run_proc('xfiredb', opts) do
      end
    when "status"
      opts = {:ARGV => [@options.action], :dir_mode => :normal, :dir => @@config.pid_file, :log_output => true }
      Daemons.run_proc('xfiredb', opts) do
      end
    else
      @@options = @options
      XFireDB.create
      XFireDB.engine.pre_init
      XFireDB::Shell.start(XFireDB.engine) if @options[:shell]
      unless missing.empty?
        XFireDB.save
        return
      end
      XFireDB.create_users
      server = XFireDB::Cluster.new(@@config.addr, @@config.port)
      server.start
    end
  end

  # Prepare the XFireDB server for shutdown.
  def XFireDB.shutdown
    puts("[exit]: Shutting down XFireDB server process")
    @@running = false
  end

  # Check if the XFireDB server is running.
  #
  # @return [Boolean] true if the server is running, false otherwise.
  def XFireDB.running
    @@running
  end

  # Get the list of available users.
  #
  # @return [Hash] The known users.
  def XFireDB.users
    @@users
  end

  # Load the users from the database.
  def XFireDB.create_users
    map = XFireDB.db['xfiredb-users']
    @@users = Hash.new
    return if map.nil?

    map.each do |username, u|
      hash, level = u.split(' ')
      user = XFireDB::User.from_hash(username, hash)
      user.level = level.to_i
      @@users[username] = user
    end
  end

  # Check if a key is illegal.
  #
  # @return [Boolean] true if the key is illegal, false otherwise.
  def XFireDB.illegal_key?(key)
    @@illegals.include? key
  end

  # Check if a key is private.
  #
  # @return [Boolean] true if the key is private, false otherwise.
  def XFireDB.private_key?(key)
    @@private_keys.include? key
  end

  # Config option getter.
  #
  # @return [Config] The configuration options.
  def XFireDB.config
    @@config
  end

  # Command line option getter.
  #
  # @return [OpenStruct] The command line options.
  def XFireDB.options
    @@options
  end

  # Engine getter.
  #
  # @return [Engine] The XFireDB engine.
  def XFireDB.db
    @@engine.db
  end

  # Commands getter.
  #
  # @return [Hash] Hashmap of the known commands.
  def XFireDB.cmds
    @@commands
  end

  # Getter for the XFireDB engine object.
  #
  # @return [XFireDB::Engine]
  def XFireDB.engine
    @@engine
  end

  # Create a new XFireDB engine instance.
  #
  # @return [XFireDB::Engine]
  def XFireDB.create
    @@engine = XFireDB::Engine.new
  end

  # Stop the database engine.
  def XFireDB.exit
    @@engine.exit if @@engine
  end

  # Save the current state of the engine to the disk.
  def XFireDB.save
    @@engine.save if @@engine
  end

  # Print a string to the standard output and flush the buffer.
  def XFireDB.print(str)
    print str
    $stdout.flush
  end

  # Get the number of worker threads.
  #
  # @return [Fixnum] The number of worker threads.
  def XFireDB.worker_num
    @options[:workers]
  end
end

