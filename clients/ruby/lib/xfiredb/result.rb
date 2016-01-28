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

module XFireDB
  class Result
    attr_reader :data, :status

    OK = 0x1
    SUCCESS = 0x2
    NULL = 0x4
    MSG = 0x8

    ACK = XFireDB::Result::OK | XFireDB::Result::SUCCESS

    def initialize(dat)
      @data = dat
      @status = nil
    end

    def process
      case @data[0,1]
      when '+' # String reply
        @data.slice!(0)
        @status = XFireDB::Result::SUCCESS
      when '&' # Boolean result
        @status = XFireDB::Result::SUCCESS
        @data = true if @data == "&true"
        @data = false if @data == "&false"
      when '%' # Numeral result
        @status = XFireDB::Result::SUCCESS
        @data.slice!(0)
        @data = @data.to_i
      when '-'
        process_default
      else
        process_default
      end
    end

    def success?
      @status & XFireDB::Result::SUCCESS == XFireDB::Result::SUCCESS
    end

    def null?
      @status & XFireDB::Result::NULL == XFireDB::Result::NULL
    end

    private
    def process_default
      @data.slice!(0) if @data[0,1] == '-'

      if @data == "OK"
        @status = ACK
      elsif @data == "nil"
        @status = NULL
      else
        @status = MSG
      end
    end
  end
end

