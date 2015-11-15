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

module XFireDB
  class User
    attr_reader :user, :password
    attr_accessor :authenticated

    @user = nil
    @password = nil
    @hash = nil
    @authenticated = false

    def initialize(user, pw)
      @user = user
      @password = pw
    end

    def User.from_hash(user, hash)
      u = User.new(user, nil)
      u.hash = hash
      return u
    end

    def hash=(hash)
      if hash.is_a? BCrypt::Password
        @hash = hash
      else
        @hash = BCrypt::Password.new(hash)
      end
    end

    def hash
      @hash
    end

    def auth
      db = XFireDB.db
      map = db['xfiredb']
      local_pw = map["user::#{@user}"]
      local_pw = BCrypt::Password.new(local_pw)
      @authenticated = local_pw == @password ? true : false
      return @authenticated
    end
  end
end

