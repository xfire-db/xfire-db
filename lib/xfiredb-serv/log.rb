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
  # XFireDB logging module.
  class Log
    # Init logging.
    LOG_INIT = "[init]: ".freeze
    # Server logging.
    LOG_SERVER = "[server]: ".freeze
    # Client logging.
    LOG_CLIENT = "[client]: ".freeze
    # XQL logging.
    LOG_XQL = "[xql]: ".freeze

    # Log a connecting client.
    def Log.connecting_client(client,user = nil)
      XFireDB::Log.write(Log::LOG_SERVER + "Client [#{client}] connected")
      XFireDB::Log.write(" (authenticated as #{user})\n") unless user.nil?
    end

    # Log an authentication failure.
    def Log.auth_fail(ip, user)
      XFireDB::Log.write(XFireDB::Log::LOG_SERVER + "Client [#{ip}] failed to authenticate as #{user}\n")
    end
  end
end
