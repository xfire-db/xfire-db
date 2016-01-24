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

#
module XFireDB
  # Server workpool. Implemented as a queue, the server will push 'work'
  # onto the workerpool. A configurable amount of threads will be waiting to
  # take work of the pool and handle it.
  class WorkerPool < Queue
    # Create a new WorkerPool.
    #
    # @param [Fixnum] num Number of threads to create.
    # @param [Cluster] cluster Cluster object.
    def initialize(num, cluster)
      super()
      db = XFireDB.db
      wokers = (0...num).map do
        Thread.new do
          begin
            while stream = self.pop(false)
              client = XFireDB::Client.from_stream(stream, cluster)
              family, port, host, ip = stream.peeraddr
              if XFireDB.config.auth
                auth = stream.gets
                if auth
                  auth.chomp!
                else
                  stream.close
                  next
                end

                auth = XFireDB::XQL.parse(auth)
                unless auth.cmd == "AUTH"
                  stream.puts "Access denied"
                  stream.close
                  next
                end

                unless client.auth(auth.args[0], auth.args[1])
                  XFireDB::Log.auth_fail(ip, auth.args[0])
                  stream.puts "Access denied"
                  stream.close
                  next
                else
                  stream.puts "OK"
                end
              end

              XFireDB::Log.connecting_client(ip, auth ? auth.args[0] : nil)
              loop do
                client.read
                break if client.quit_recv

                v = cluster.query(client)
                if v.is_a? Array
                  stream.puts v.map { |s| s.length + 1}.join ' '
                  v.each do |val|
                    stream.puts val
                  end
                else
                  stream.puts v.length + 1
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
          rescue IllegalCommandException => e
            stream.puts e
            stream.close
            next
          rescue Exception => e
            puts e.class
            if e.is_a? BCrypt::Errors::InvalidHash
              stream.puts "Access denied for #{ip}"
              XFireDB::Log.auth_fail(ip, auth.args[0])
              stream.close
              next
            end

            puts e
            puts e.backtrace
            stream.puts "Query failed"
            stream.close
            next
          end
        end
      end
    end
  end
end

