#
#   XFireDB shell
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
  # XFireDB server shell. This shell can be used to configure and setup
  # an XFireDB instance.
  class Shell
    @@engine = nil

    # Start the shell.
    #
    # @param [Engine] engine XFireDB engine instance.
    def Shell.start(engine)
      $stdout.sync = true
      @@engine = engine
      map = @@engine.db['xfiredb']
      print "\n"
      puts "XFireDB is not setup properly, use " \
        "`setup' to enter the setup" if map.nil? or map['id'].nil?

      loop {
        print "> "
        rawcmd = gets.chop
        cmd = Shell.parse(rawcmd)
        rv = Shell.exec(cmd)
        puts rv if rv.class == String
        break if rv == "Daemonizing XFireDB"
      }
    end

    # Execute a shell command.
    #
    # @param [String] cmd Shell command to execute.
    def Shell.exec(cmd)
      res = case cmd[0]
            when "daemonize"
              "Daemonizing XFireDB"
            when "reset"
              Shell.reset
              Shell.reset_root_node if cmd[1] == "root-node"
            when "setup"
              cluster = cmd[1] == "root-node"
              Shell.setup(cluster)
              Shell.setup_root_node if cmd[1] == "root-node"
            when "useradd"
              Shell.useradd
            when "quit"
              exit
            when "help"
              "Available commands:\n\n" \
                "* help\n" \
                "* quit\n" \
                "* daemonize\n" \
                "* setup\n"
            else
              "Invalid command"
            end
    end

    # Add a new user.
    def Shell.useradd
      print "Username: "
      user = gets.chop
      print "Password: "
      passw = STDIN.noecho(&:gets).chomp

      db = @@engine.db
      map = db['xfiredb']
      map["user::#{user}"] = BCrypt::Password.create passw
      puts "\nUser created!"
    end

    # Reset a XFireDB node.
    def Shell.reset
      db = @@engine.db
      db['xfiredb'].delete('shards') unless db['xfiredb'].nil?
      db.delete('xfiredb-nodes')
    end

    # Reset the node to a root node.
    def Shell.reset_root_node
      Shell.setup_root_node
    end

    # Setup a XFireDB server/node.
    #
    # @param [Cluster] cluster Cluster instance.
    def Shell.setup(cluster)
      db = @@engine.db
      db['xfiredb'] ||= XFireDB::Hashmap.new
      map = db['xfiredb']
      map['id'] ||= SecureRandom.hex(64)
      print "Sys admin username: "
      user = gets.chop
      print "Password: "
      passw = STDIN.noecho(&:gets).chomp
      puts "\nSetup complete!"
      map["user::#{user}"] = BCrypt::Password.create passw
      if cluster
        db['xfiredb-users'] = XFireDB::Hashmap.new
        db['xfiredb-users']["#{user}"] = "#{BCrypt::Password.create passw} #{XFireDB::User::ADMIN}"
        secret = SecureRandom.base64 32
        db['xfiredb']['secret'] = secret
        XFireDB.config.secret = secret
      end
      return
    end

    # Setup a XFireDB root node.
    def Shell.setup_root_node
      db = @@engine.db
      rv = (0..16383).to_a
      rv = rv.join(':')
      db['xfiredb']['shards'] = rv
      db.delete('xfiredb-nodes')
      return
    end

    # Parse a shell command.
    #
    # @param [String] rawcmd The command to parse.
    # @return [Array] The parsed command and its arguments.
    def Shell.parse(rawcmd)
      rawcmd = rawcmd.split(/(?:^|(?:[.!?]\s))(\w+) (.+)/)
      cmd = rawcmd[1] || rawcmd[0]
      args = rawcmd[2]
      [cmd, args]
    end
  end
end

