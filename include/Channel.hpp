/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lsampiet <lsampiet@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/05 17:34:02 by caio              #+#    #+#             */
/*   Updated: 2025/08/16 18:34:04 by lsampiet         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "Utils.hpp"
#include "Server.hpp"
#include <vector>
#include <map>
#include <set>
#include <algorithm>

class Server;
class Client;

// Channel modes
#define MODE_INVITE_ONLY 'i'
#define MODE_TOPIC_PROTECT 't'
#define MODE_KEY 'k'
#define MODE_LIMIT 'l'
#define MODE_MODERATED 'm'
#define MODE_NO_EXTERNAL_MSGS 'n'
#define MODE_SECRET 's'
#define MODE_PRIVATE 'p'

class Channel
{
    private:
        std::string _name;
        std::string _topic;
        std::string _welcome_msg;  //MESSAGE OF THE DAY
        std::string _password;
        std::string _key;  //for +k mode
        
        std::set<std::string> _userlist;
        std::set<std::string> _operators;
        std::set<std::string> _banned;
        std::set<std::string> _invited; //invited list mode +i

        std::set<char> _modes; //channel active modes
        size_t _user_limit;    // user limits mode +l

        time_t _creation_time;

        bool _hasMode(char mode) const;
        void _boreadcastToChannel(Server *server, const std::string &message, int exclude_fd = -1);
        std::string _getUserModePrefix(const std::string &nickname) const;
   
    public:
        Channel(std::string const &name, std::string const &password = "");
        ~Channel();

        //Getters
        std::string getName() const;
        std::string getTopic() const;
        std::string getWelcomeMsg() const;
        std::string getPassword() const;
        std::string getKey() const;
        size_t getUserCount() const;
        size_t getUserLimit() const;
        time_t getCreationTime() const;

        //Setters
        void setTopic(std::string const &topic);
        void setPassword(std::string const &password);
        void setUserLimit(size_t limit);
        void setWelcomeMsg(std::string const &msg);

        //User management
        bool addUser(std::string const &user, std::string const &password = "");
        bool removeUser(std::string const &user);
        bool hasUser(std::string const &user) const;
        std::vector<std::string> getUserList() const;
        std::string getUserListString() const;
        bool updateUserNick(const std::string &oldNick, const std::string &newNick);

        //Operators management
        bool addOp(std::string const &user);
        bool removeOp(std::string const &user);
        bool isOp(std::string const &user) const;
        
        //Ban system
        bool banUser(std::string const &user);
        bool unbanUser(std::string const &user);
        bool isBanned(std::string const &user) const;

        //Invite system
        bool inviteUser(std::string const &user);
        bool isInvited (std::string const &user) const;
        
        //Modes management
        bool setMode(char mode, bool enable, std::string const &param = "");
        bool hasMode (char mode) const;
        std::string getModeString() const;

        //Validations
        bool isPasswordRequired() const;
        bool canUserJoin(std::string const &user, std::string const &password = "") const;
        bool canUserSetTopic(std::string const &user) const;

        //Connection and Communication
        bool connect(Server *server, int client_fd);
        void sendMessage(Server *server, const std::string &sender, std::string const msg);
        void sendNotice(Server *server, const std::string &message);

        //System messages
        void announceJoin(Server *server, Client *client);
        void announcePart(Server *server, Client *client, const std::string &reason = "");
        void announceQuit(Server *server, Client *client, const std::string &reason = "");
        void announceNickChange(Server *server, const std::string &oldNick, const std::string &newNick);

        //Empty channel checking
        bool isEmpty() const;

};
