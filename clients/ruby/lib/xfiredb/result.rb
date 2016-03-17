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
  # Query result class
  class Result
    attr_reader :data, :status

    # Status OK
    OK = 0x1
    # Status SUCCESS
    SUCCESS = 0x2
    # Status NULL
    NULL = 0x4
    # Status MESSAGE
    MSG = 0x8

    # Query success status codes
    ACK = XFireDB::Result::OK | XFireDB::Result::SUCCESS

    # Create a new result object
    #
    # @param [String] dat Raw query data.
    def initialize(dat)
      @data = dat
      @status = nil
    end

    # Process the data.
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

    # Check whether the query was succesfull or not.
    def success?
      @status & XFireDB::Result::SUCCESS == XFireDB::Result::SUCCESS
    end

    # Check if the result is NULL.
    def null?
      @status & XFireDB::Result::NULL == XFireDB::Result::NULL
    end

    private
    # Process status codes.
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

