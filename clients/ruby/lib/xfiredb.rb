#
#   XFireDB client library
#   Copyright (C) 2016  Michel Megens <dev@michelmegens.net>
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

require 'socket'
require 'openssl'
require 'set'

require 'xfiredb/socket'
require 'xfiredb/version'
require 'xfiredb/client'
require 'xfiredb/result'

# XFireDB client module.
module XFireDB
  # SSL connection flag.
  SSL    = 0x1
  # Authentication required flag.
  AUTH   = 0x2
  # Command stream flag.
  STREAM = 0x4

  class << self
    # Create a new client.
    # @return [Client] A new client object.
    def new
      Client.new
    end

    # Connect to an XFireDB server
    #
    # @param [String] host Server host.
    # @param [Fixnum] port Server port.
    # @param [Fixnum] flags Connection flags.
    # @param [String] user Username.
    # @param [String] pass Password.
    # @return [Client] A client object.
    def connect(host, port, flags = nil, user = nil, pass = nil)
      client = Client.new
      client.connect(host, port, flags, user, pass)
      return client
    end
  end
end

