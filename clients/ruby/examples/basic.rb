require 'xfiredb'

client = XFireDB.connect('localhost', 7000,
                    XFireDB::SSL | XFireDB::AUTH | XFireDB::STREAM,
                      'root', 'root')
client.set('test-key', 'Some test data')
data = client.get('test-key')
client.delete('test-key')
puts data
client.close

