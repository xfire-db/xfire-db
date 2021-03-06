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
  # Configuration class
  #
  # This class is responsible for ready the yaml-like configuration file and
  # parsing it into ruby variable's.
  class Config
    attr_reader :port, :config_port, :addr, :cluster, :data_dir,
      :debug, :log_file, :err_log_file, :db_file, :persist_level, :auth, :problems,
      :ssl, :ssl_cert, :ssl_key, :cluster_user, :cluster_auth, :pid_file
    attr_accessor :daemon, :secret

    CONFIG_PORT = "port"
    CONFIG_BIND_ADDR = "bind-addr"
    CONFIG_CLUSTER = "cluster-enabled"
    CONFIG_DEBUG = "debug-mode"
    CONFIG_TEST_OPT = "test-option"
    CONFIG_DB_FILE = "db-file"
    CONFIG_PERSIST_LEVEL = "persist-level"
    CONFIG_AUTH_REQUIRED = "auth-required"
    CONFIG_SSL_SERV = 'ssl-required'
    CONFIG_SSL_CERT = 'ssl-certificate'
    CONFIG_SSL_KEY = 'ssl-key'
    CONFIG_DATA_DIR = 'data-dir'

    @port = nil
    @addr = nil
    @cluster = false
    @cluster_config = nil
    @debug = false
    @daemon = false
    @log_file = nil
    @err_log_file = nil
    @db_file = nil
    @persist_level = 0
    @auth = false
    @ssl = false
    @ssl_cert = nil
    @ssl_key = nil
    @secret = nil
    @cluser_auth = false
    @pid_file = nil
    @data_dir = nil

    # Create a new config.
    #
    # @param [String] file Path to the configuration file.
    def initialize(file = nil)
      return unless file

      @filename = file
      @problems = 0
      @cluster_user = 'cluster'
      fh = File.open(@filename, "r")
      puts "[config]: config file (#{file}) not found!" unless check_config(fh)
      fh.each do |line|
        next if line.length <= 1 or line.lstrip.start_with?('#')
        string = line.split(/(.+?) (.+)/)
        opt = string[1]
        arg = string[2]

        parse opt, arg
      end
      fh.close

      @cluster_auth = @cluster
      check_mandatory
    end

    # Check if all mandatory configuration keys are present and
    # have correct arguments.
    def check_mandatory
      fail_count = 0

      if @ssl
        unless @ssl_key and @ssl_cert
          puts "ssl-key and ssl-certificate are required when using SSL"
          fail_count += 1
        end
      end

      unless @pid_file
        puts "[config]: pid-file is a required configuration option"
        fail_count += 1
      end

      unless @port
        puts "[config]: port is a required config option"
        fail_count += 1
      end

      unless @addr
        puts "[config]: bind-addr is a required config option"
        fail_count += 1
      end

      unless @log_file
        puts "[config]: stdout-file is a required config option" unless @log_file
        fail_count += 1
      end

      unless @err_log_file
        puts "[config]: stderr-file is a required config option" unless @err_log_file
        fail_count += 1
      end

      unless @db_file and @persist_level < 3
        puts "[config]: db-file is a required config option" unless @db_file
        fail_count += 1
      end

      exit unless fail_count == 0
    end

    # Parse the config options.
    #
    # @param [String] opt Option to parse.
    # @param [String] arg Argument to the option.
    def parse(opt, arg)
      case opt
        # Main config options
      when CONFIG_DATA_DIR
        @data_dir = arg
        @pid_file = arg
        @log_file = File.expand_path("#{arg}/xfiredb.stdout")
        @err_log_file = File.expand_path("#{arg}/xfiredb.stderr")
      when CONFIG_SSL_CERT
        @ssl_cert = File.expand_path(arg)
      when CONFIG_SSL_KEY
        @ssl_key = File.expand_path(arg)
      when CONFIG_SSL_SERV
        @ssl = true if arg.upcase == "TRUE"
      when CONFIG_AUTH_REQUIRED
        @auth = true if arg.upcase == "TRUE"
      when CONFIG_DB_FILE
        @db_file = File.expand_path(arg)
      when CONFIG_PERSIST_LEVEL
        @persist_level = arg.to_i if arg.is_i?
        puts "[config]: #{opt} should be numeric" unless arg.is_i?
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
      when CONFIG_DEBUG
        @debug = true if arg.eql? "true"
        # cluster config options
      else
        puts "[config]: unknown option: \'#{opt}\'"
        @problems += 1
      end
    end

    # Check the configuration file.
    #
    # @param [File] file Config file to check.
    def check_config(file)
      !File.world_readable?(file).nil? && File.exist?(file)
    end
  end

end

