#
#   XFireDB string extensions
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

# XFireDB string extensions.
class String
  # Check if a string contains numeral characters only.
  def is_i?
    /\A[-+]?\d+\z/ === self
  end

  # Remove the first character if it starts with a specific seperator.
  #
  # @param [String] sep Seperator to test the start of the string against.
  def rchomp(sep = $/)
    self.start_with?(sep) ? self[sep.size..-1] : self
  end

  # Add quotation marks around the string.
  def quote
    "\"#{self}\""
  end

  # Tokenize a string.
  def tokenize
    self.scan(/(?:"(?:\\.|[^"])*"|[^" ])+/).map {|s| s.strip.rchomp('"').chomp('"')}
  end
end

