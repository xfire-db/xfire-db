#
#   XFireDB user
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
  # XFireDB user
  class User
    # ADMIN user level
    ADMIN = 10
    # REGULAR user level
    NORMAL = 1
    attr_reader :user, :password
    attr_accessor :authenticated, :level

    @user = nil
    @password = nil
    @hash = nil
    @authenticated = false
    @level = 1

    # Create a new user.
    #
    # @param [String] user Username.
    # @param [String] pw Password.
    def initialize(user, pw)
      @user = user
      @password = pw
    end

    # Create a new user from a hash.
    #
    # @param [String] user Username.
    # @param [String] hash Password hash.
    # @return [User] New user object.
    def User.from_hash(user, hash)
      u = User.new(user, nil)
      u.hash = hash
      return u
    end

    # Assign a new hash to a {User}. The hash will be converted
    # to a [BCrypt::Password].
    #
    # @param [String] hash Password hash to set.
    def hash=(hash)
      if hash.is_a? BCrypt::Password
        @hash = hash
      else
        @hash = BCrypt::Password.new(hash)
      end
    end

    # Hash getter method.
    #
    # @return [BCrypt::Password] User password hash.
    def hash
      @hash
    end

    # Authenticate a user. The authentication will be done
    # using the so called 'local' users (i.e. this is not a cluster wide
    # authentication). For cluster wide authentication please check {Client#auth}.
    #
    # @return [Boolean] true if the authentication succeeded false otherwise.
    def auth
      db = XFireDB.db
      map = db['xfiredb']
      local_pw = map["user::#{@user}"]
      local_pw = BCrypt::Password.new(local_pw)
      @authenticated = local_pw == @password ? true : false
      return @authenticated
    end

    # Convert a {User} to a string.
    #
    # @return [String] String representation of {User}.
    def to_s
      "#{@user}::#{@hash}::#{@level}"
    end
  end
end

