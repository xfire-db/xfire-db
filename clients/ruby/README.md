# ruby-xfiredb

XFireDB client library for ruby applications.

## Installation

Add this line to your application's Gemfile:

```ruby
gem 'xfiredb'
```

And then execute:

    $ bundle

Or install it yourself as:

    $ gem install xfiredb

## Usage

Connecting to a server and executing a query is done as following:

```ruby
client = XFireDB.connect('localhost', 7000,
		XFireDB::SSL | XFireDB::AUTH | XFireDB::STREAM,
		  'user', 'pass')

client.query("GET key1") {|r| puts r.data}
client.close
```

## Contributing

Bug reports and pull requests are welcome on [GitLab](http://git.bietje.net/xfiredb/xfiredb).

