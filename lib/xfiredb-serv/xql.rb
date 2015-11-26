#
#   XFireDB XQL parser
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

#
module XFireDB
  # XQL query wrapper.
  class XQLCommand
    attr_reader :cmd, :args, :raw
    attr_accessor :src_ip, :src_port

    @cmd = nil
    @args = nil
    @raw = nil
    @src_ip = nil
    @src_port = nil

    # Create a new XQL query query.
    #
    # @param [String] cmd Command identifier.
    # @param [String] args Arguments to cmd.
    # @param [String] query The full XQL query.
    def initialize(cmd, args, query)
      @cmd = cmd
      @args = args
      @raw = query
    end
  end

  # XQL factory
  class XQL
    # Factory method to produce an XQLCommand.
    #
    # @param [String] query An XQL query.
    # @return [XQLCommand]
    def XQL.parse(query)
      cmdary = query.tokenize
      cmd = cmdary.shift
      cmd.upcase!
      return XQLCommand.new(cmd, cmdary, query)
    end
  end
end

