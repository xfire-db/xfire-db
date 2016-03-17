require 'xfiredb'

client = XFireDB.connect('localhost', 7000,
                    XFireDB::SSL | XFireDB::AUTH | XFireDB::STREAM,
                      'root', 'root')
3.times {|x|
  client.map_add('map-key', "key#{x+1}", "Key #{x+1} data")
}

map = client.map_ref('map-key', 'key1', 'key2', 'key3')
puts map['key1']
client.delete('map-key')
client.close

