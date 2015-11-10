#
#   XFireDB client
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
  class Client
    attr_accessor :stream, :cluster
    attr_reader :request, :user

    @request = nil
    @stream = nil
    @cluster = nil
    @user = nil

    def initialize(client, xql = nil)
      @request = XFireDB::XQL.parse(xql) unless xql.nil?
      @stream = client
    end

    def Client.from_stream(stream, cluster = nil)
      client = Client.new(stream)
      client.stream = stream
      client.cluster = cluster
      return client
    end

    def auth(user, password)
      @user = XFireDB::User.new(user, password)
      @user.auth
    end

    def auth?
      @user.authenticated
    end

    def read(ip = nil, port = nil)
      data = @stream.gets.chomp
      @request = XFireDB::XQL.parse(data)
      @request.src_ip = ip
      @request.src_port = port
    end

    def process(xql = nil)
      if xql.nil? && @process.nil?
        raise ArgumentError.new("Cannot handle a process without a query")
      end

      @request = XFireDB::Request.new(xql) unless xql.nil?
      @request.handle
    end
  end
end
