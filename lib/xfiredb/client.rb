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
    attr_accessor :xql
    attr_reader :request

    @xql = nil
    @request = nil

    def initialize(xql = nil)
      @xql = xql
      @request = XFireDB::Request.new(xql) unless xql.nil?
    end

    def process(xql = nil)
      if xql.nil? && @xql.nil?
        raise ArgumentError.new("Cannot handle a process without a query")
      end

      @xql = xql if @xql.nil?
      @request = XFireDB::Request.new(xql) unless xql.nil?
      @request.handle
    end
  end
end