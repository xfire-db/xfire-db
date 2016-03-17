require 'xfiredb'

client = XFireDB.connect('localhost', 7000,
                    XFireDB::SSL | XFireDB::AUTH | XFireDB::STREAM,
                      'root', 'root')
3.times {|x|
  client.set_add('test-set', 'set key1', 'set key2', 'set key3')
}

puts client.set_include?('test-set', 'set key1')
puts client.set_include?('test-set', 'key2')
client.delete('test-set')
client.close
