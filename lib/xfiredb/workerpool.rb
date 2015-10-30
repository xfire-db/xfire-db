#
#   XFireDB Worker pool
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
  class WorkerPool < Queue
    def initialize(num, server)
      super()
      wokers = (0...num).map do
        Thread.new do
          begin
            while stream = self.pop(false)
              client = XFireDB::Client.from_stream(stream)
              stream.close
              server.store.db["test"] = "hey there"
            end
          rescue ThreadError
            puts "Thread error"
          end
        end
      end
    end

    def handle(client)
    end
  end
end

