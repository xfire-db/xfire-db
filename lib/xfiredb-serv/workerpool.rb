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
    def initialize(num, cluster)
      super()
      db = XFireDB.db
      wokers = (0...num).map do
        Thread.new do
          begin
            while stream = self.pop(false)
              client = XFireDB::Client.from_stream(stream, cluster)
              if XFireDB.config.auth
                auth = stream.gets.chomp
                auth = XFireDB::XQL.parse(auth)
                unless auth.cmd == "AUTH"
                  stream.puts "Access denied"
                  stream.close
                  next
                end

                unless client.auth(auth.args[0], auth.args[1])
                  stream.puts "Access denied"
                  stream.close
                  next
                end
              end
              loop do
                client.read
                break if client.quit_recv

                v = cluster.query(client)
                if v.is_a? Array
                  v.each do |val|
                    stream.puts val
                  end
                else
                  stream.puts v
                end
                break unless client.keep
              end
              stream.close
            end
          rescue IllegalKeyException => e
            stream.puts e
            stream.close
            next
          rescue Exception => e
            puts e
            puts e.backtrace
            stream.puts "Query failed"
            stream.close
            next
          end
        end
      end
    end

    def handle(client)
    end
  end
end

