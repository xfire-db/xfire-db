require 'xfiredb'

client = XFireDB.connect('localhost', 7000,
                    XFireDB::SSL | XFireDB::AUTH | XFireDB::STREAM,
                      'root', 'root')
3.times {|x|
  client.list_push('test-list', "List entry #{x+1}")
}

list = client.list_ref('test-list', 0..-1)
list.each do |element|
  puts element
end

client.delete('test-list')
client.close
