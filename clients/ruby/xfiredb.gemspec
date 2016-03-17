# coding: utf-8
lib = File.expand_path('../lib', __FILE__)
$LOAD_PATH.unshift(lib) unless $LOAD_PATH.include?(lib)
require 'xfiredb/version'

Gem::Specification.new do |spec|
  spec.name          = "xfiredb"
  spec.version       = XFireDB::VERSION
  spec.authors       = ["Michel Megens"]
  spec.email         = ["dev@michelmegens.net"]

  spec.summary       = %q{XFireDB connector}
  spec.description   = %q{XFireDB ruby client library}
  spec.homepage      = "http://xfiredb.bietje.net"
  spec.files         = ["lib/xfiredb.rb", "lib/xfiredb/client.rb", "lib/xfiredb/result.rb", "lib/xfiredb/socket.rb", "lib/xfiredb/version.rb"]

  spec.require_paths = ["lib"]

  spec.add_development_dependency "bundler", "~> 1.10"
  spec.add_development_dependency "rake", "~> 10.0"
  spec.add_development_dependency "rspec"
end
