#include "../include/Client.hpp"

Client::Client(int client_socket, sockaddr_in client_addr) 
    : _client_fd(client_socket), _client_addr(client_addr),  
      _isRegistered(false), _hasPassword(false), _hasNick(false), _hasUser(false)
{
    //Converts IP address to string in a secure way
    char ip_str[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, INET_ADDRSTRLEN) != NULL)
        this->_hostname = std::string(ip_str);
    else
        this->_hostname = "unknown";
    
    logMessage("New client connected! FD= ", BLUE, itoa(client_socket), GREEN);
    this->_lastActivity = time(NULL);
    
}

Client::~Client()
{
    if(this->_client_fd > 0)
        close(this->_client_fd);
}

int Client::getFd() const
{
    return this->_client_fd;
}

std::string Client::getHostname() const
{
    return this->_hostname;
}

sockaddr_in Client::getClientAddr() const
{
    return this->_client_addr;
}

std::string Client::getNickname() const
{
    return this->_nickname;
}

std::string Client::getRealname() const
{
    return this->_realname;
}

std::string Client::getUsername() const
{
    return this->_username;
}

std::string Client::getPassword() const
{
    return this->_password;
}

std::string Client::getBuffer() const
{
    return this->_buffer;
}

bool Client::isRegistered() const
{
    return (this->_isRegistered);
}

void Client::setNickname(const std::string &nickname)
{
    std::string formattedNick = nickname; 
    
    // Remove control chars
    size_t pos = formattedNick.find_first_of("\r\n");
    if (pos != std::string::npos)
        formattedNick.erase(pos);
    
    // Remove extra spaces
    pos = formattedNick.find(' ');
    if (pos != std::string::npos)
        formattedNick.erase(pos);
        
    this->_nickname = formattedNick;
    this->_hasNick = !formattedNick.empty();
    this->checkRegistrationComplete();
}

void Client::setUsername(const std::string &username)
{
    this->_username = username;
    this->_hasUser = !username.empty();
    this->checkRegistrationComplete();
}

void Client::setRealname(const std::string &realname)
{
    this->_realname = realname;
}

void Client::appendBuffer(const std::string &data)
{
    // Prevent buffer overflow
    if (this->_buffer.length() + data.length() > MAX_BUFFER_SIZE)
    {
        logMessage("WARNING: ", YELLOW, "Buffer overflow prevented for client FD=" + itoa(_client_fd), WHITE);
        this->_buffer.clear(); // Limpa buffer para evitar problemas
        return;
    }
    
    this->_buffer += data;
}

bool Client::isDataComplete() const
{
    return (this->_buffer.find("\r\n") != std::string::npos || this->_buffer.find("\n") != std::string::npos);
}

void Client::cleanBuffer()
{
    this->_buffer.clear();
}

std::string Client::getNextCompleteMessage()
{
    std::string message;
    size_t pos = this->_buffer.find("\r\n");
    
    if (pos == std::string::npos)
        pos = this->_buffer.find("\n");
    
    if (pos != std::string::npos)
    {
        message = this->_buffer.substr(0, pos);
        this->_buffer.erase(0, pos + (this->_buffer[pos] == '\r' ? 2 : 1));
    }
    
    return message;
}

void Client::setNamesAndPass(std::string const &data)
{
       std::istringstream iss(data);
    std::string line;

    while(std::getline(iss, line))
    {
        // Remove \r if its present
        if(!line.empty() && line[line.length() - 1] == '\r')
            line.erase(line.length() - 1);
        
        if (line.empty())
            continue;
        
        // Parse different register commands
        if (line.length() >= 5)
        {
            std::string cmd = line.substr(0, 5);
            
            if(cmd == "PASS ")
                this->parsePassCommand(line);
            else if(cmd == "NICK ")
                this->parseNickCommand(line);
            else if(cmd == "USER ")
                this->parseUserCommand(line);
        }
    }
}

void Client::parsePassCommand(const std::string &line)
{
   if (line.length() > 5)
    {
        std::string password = line.substr(5);
        
        // Remove espaços no início
        size_t start = password.find_first_not_of(" \t");
        if (start != std::string::npos)
            password = password.substr(start);
        
        // Remove caracteres de controle no final (mas mantém o conteúdo)
        size_t end = password.find_last_not_of(" \t\r\n");
        if (end != std::string::npos)
            password = password.substr(0, end + 1);
        
        this->_password = password;
        this->_hasPassword = !password.empty();
        
        logMessage("Password set for client FD=" + itoa(_client_fd) + ": ", CYAN, 
                  (password.empty() ? "[EMPTY]" : this->_password), WHITE);
        
        this->checkRegistrationComplete();
    }
}

void Client::parseNickCommand(const std::string &line)
{
    if (line.length() > 5)
    {
        std::string nickname = line.substr(5);
        
        // Remove espaços no início
        size_t start = nickname.find_first_not_of(" \t");
        if (start != std::string::npos)
            nickname = nickname.substr(start);
        
        // Remove espaços e caracteres de controle no final
        size_t end = nickname.find_last_not_of(" \t\r\n");
        if (end != std::string::npos)
            nickname = nickname.substr(0, end + 1);
        
        // Remove espaços no meio (pega só a primeira palavra)
        size_t space_pos = nickname.find(' ');
        if (space_pos != std::string::npos)
            nickname = nickname.substr(0, space_pos);
        
        this->setNickname(nickname);
        logMessage("Nickname parsed for client FD=" + itoa(_client_fd) + ": ", CYAN, nickname, WHITE);
    }
}

void Client::parseUserCommand(const std::string &line)
{
   if (line.length() > 5)
    {
        std::string user_data = line.substr(5);
        std::istringstream iss(user_data);
        std::string username, mode, unused;
        
        // Format: USER <username> <mode> <unused> :<realname>
        if (iss >> username >> mode >> unused)
        {
            this->setUsername(username);
            
            // Gets the rest of the line as realname (after ":")
            std::string remaining;
            std::getline(iss, remaining);
            
            size_t colon_pos = remaining.find(':');
            if (colon_pos != std::string::npos)
            {
                std::string realname = remaining.substr(colon_pos + 1);
                
                // Remove espaços no início e fim do realname
                size_t start = realname.find_first_not_of(" \t");
                if (start != std::string::npos)
                    realname = realname.substr(start);
                
                size_t end = realname.find_last_not_of(" \t\r\n");
                if (end != std::string::npos)
                    realname = realname.substr(0, end + 1);
                
                this->setRealname(realname);
            }
            
            logMessage("User info parsed for client FD=" + itoa(_client_fd) + ": ", CYAN, 
                      username, WHITE);
        }
    }
}

void Client::checkRegistrationComplete()
{
    bool was_registered = this->_isRegistered;
    
    // CORREÇÃO: Se temos PASS e NICK, mas não USER, cria USER padrão
    if (this->_hasPassword && this->_hasNick && !this->_hasUser)
    {
        // Auto-gera dados de usuário baseados no nickname
        this->_username = this->_nickname;
        this->_realname = this->_nickname;
        this->_hasUser = true;
        logMessage("Auto-generated USER data for: ", CYAN, this->_nickname, WHITE);
    }
    
    // Cliente está registrado quando tem nick, user e password
    this->_isRegistered = (this->_hasNick && this->_hasUser && this->_hasPassword);
    
    if (!was_registered && this->_isRegistered)
    {
        logMessage("Client registration data complete! Nick: ", GREEN, 
                  this->_nickname + " User: " + this->_username + " Pass: " + this->_password, BLUE);
    }
    else if (!this->_isRegistered)
    {
        logMessage("Client registration incomplete - Nick: " + 
                  std::string(this->_hasNick ? "OK" : "MISSING") + 
                  " User: " + (this->_hasUser ? "OK" : "MISSING") + 
                  " Pass: " + (this->_hasPassword ? "OK" : "MISSING"), YELLOW, "", WHITE);
    }
}

void Client::joinChannel(const std::string &channelName)
{
    this->_channels.insert(channelName);
}

void Client::leaveChannel(const std::string &channelName)
{
    this->_channels.erase(channelName);
}

bool Client::isInChannel(const std::string &channelName) const
{
    return (this->_channels.find(channelName) != this->_channels.end());
}

std::set<std::string> Client::getChannels() const
{
    return this->_channels;
}


bool Client::isValidInput(const std::string &input) const
{
    // Verifies if data inserted has harmful control caracters
    for (size_t i = 0; i < input.length(); i++)
    {
        unsigned char c = static_cast<unsigned char>(input[i]);
        
        // Allow only printable chars and some control chars
        if (c < 32 && c != '\r' && c != '\n' && c != '\t')
            return false;
            
        // NULL control char is not allowed
        if (c == 0)
            return false;
    }
    return true;
}

void Client::setLastActivity (time_t now)
{
    this->_lastActivity = now;
}
time_t Client::getLastActivity (void)
{
    return (this->_lastActivity);
}
