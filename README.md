# ft_irc - IRC Server Implementation

A simple IRC (Internet Relay Chat) server implementation written in C++98, designed to handle multiple clients simultaneously.

## 📋 Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Requirements](#requirements)
- [Installation](#installation)
- [Usage](#usage)
- [Supported Commands](#supported-commands)
- [Channel Modes](#channel-modes)
- [Testing](#testing)
- [Project Structure](#project-structure)

## 🌟 Overview

This project implements an IRC server that follows basic IRC protocol standards. The server handles multiple clients, manages channels, and supports essential IRC commands.

### Key Features

- Multiple client connections using `poll()`
- Channel management with operators and regular users
- Password-based authentication
- Channel modes (invite-only, topic protection, etc.)
- Private messaging and channel communication

## ✨ Features

- Multiple client connections
- User authentication with passwords
- Channel creation and management
- Private messaging
- Channel operators system
- Channel modes (invite-only, topic protection, user limits)
- KICK, INVITE, and TOPIC commands
- Real-time message broadcasting

## 🔧 Requirements

- g++ compiler with C++98 support
- Unix-like system (Linux, macOS)
- GNU Make

## 🚀 Installation

1. Clone and compile:
   ```bash
   git clone <repository-url>
   cd ft_irc
   make
   ```

2. Clean build (optional):
   ```bash
   make clean    # Remove object files
   make fclean   # Remove all build files
   make re       # Rebuild everything
   ```

## 📖 Usage

### Start the server:
```bash
./ircserv <port> <password>
```

**Example:**
```bash
./ircserv 6667 mypassword
```

### Connect with IRC clients:
- **Testing with nc**: `nc localhost 6667`
- **IRC clients**: HexChat, WeeChat, irssi, etc.

### Basic usage:
1. Connect to server
2. Authenticate with password
3. Set nickname: `/nick <nickname>`
4. Join channels: `/join #channelname`
5. Start chatting!

## 🎮 Supported Commands

### User Commands
| Command | Syntax | Description |
|---------|--------|-------------|
| `PASS` | `PASS <password>` | Authenticate with server password |
| `NICK` | `NICK <nickname>` | Set or change nickname |
| `USER` | `USER <username> <mode> <unused> :<realname>` | Set user information |
| `QUIT` | `QUIT [:<reason>]` | Disconnect from server |

### Channel Commands
| Command | Syntax | Description |
|---------|--------|-------------|
| `JOIN` | `JOIN #<channel> [<key>]` | Join a channel |
| `PART` | `PART #<channel> [:<reason>]` | Leave a channel |
| `PRIVMSG` | `PRIVMSG <target> :<message>` | Send message to user or channel |
| `NOTICE` | `NOTICE <target> :<message>` | Send notice to user or channel |

### Operator Commands
| Command | Syntax | Description |
|---------|--------|-------------|
| `KICK` | `KICK #<channel> <nick> [:<reason>]` | Remove user from channel |
| `INVITE` | `INVITE <nick> #<channel>` | Invite user to channel |
| `TOPIC` | `TOPIC #<channel> [:<topic>]` | Set or view channel topic |
| `MODE` | `MODE #<channel> <modes> [<parameters>]` | Change channel modes |

## 🔧 Channel Modes

| Mode | Description | Parameter |
|------|-------------|-----------|
| `+i` | Invite-only channel | None |
| `+t` | Only operators can change topic | None |
| `+k` | Channel requires password | `<password>` |
| `+l` | Limit number of users | `<number>` |
| `+o` | Grant/revoke operator status | `<nickname>` |

### Examples:
```bash
/mode #mychannel +i              # Make invite-only
/mode #mychannel +k secretpass   # Set password
/mode #mychannel +l 50           # Limit to 50 users
/mode #mychannel +o alice        # Make alice operator
```

## 🏗️ Architecture

### Core Components

1. **Server Class**: Main server logic, connection handling, and command routing
2. **Client Class**: Individual client connection management and state
3. **Channel Class**: Channel operations, user lists, and mode management
4. **Utils**: Helper functions for validation and logging

### Design Patterns

- **Non-blocking I/O**: Single-threaded event-driven architecture
- **Resource Management**: RAII principles for automatic cleanup
- **Command Pattern**: Modular command handling system
- **Observer Pattern**: Event broadcasting for channel updates

### Memory Management

- Automatic cleanup on client disconnection
- Proper resource deallocation in destructors
- Signal handling for graceful shutdown
- No memory leaks in normal operation

## 🧪 Testing

### Basic Connection Test:
```bash
nc localhost 6667
PASS mypassword
NICK testuser
USER testuser 0 * :Test User
JOIN #test
PRIVMSG #test :Hello everyone!
```

### Multi-client Testing:
Open multiple terminals to test real-time messaging between clients.

### Partial Data Test:
```bash
nc -C 127.0.0.1 6667
com^Dman^Dd  # Use Ctrl+D to send data in chunks
```

## 📁 Project Structure

```
ft_irc/
├── Makefile                 # Build configuration
├── include/
│   ├── Server.hpp          # Server class definition
│   ├── Client.hpp          # Client class definition
│   ├── Channel.hpp         # Channel class definition
│   └── Utils.hpp           # Utility functions
└── src/
    ├── main.cpp            # Entry point and signal handling
    ├── Server.cpp          # Server implementation
    ├── Client.cpp          # Client implementation
    ├── Channel.cpp         # Channel implementation
    └── Utils.cpp           # Utility implementations
```
## 📜 License

This project is part of the 42 School curriculum and follows their academic guidelines.

---

**Note**: This IRC server is designed for educational purposes and follows the 42 School ft_irc project requirements. It implements core IRC functionality while maintaining high code quality and proper error handling.
