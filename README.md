# IM
A simple client/server Instant Messaging application

# Build & Run

This software consists of two parts: the server and the client. The server only runs at Linux, and the client can run at Windows, Linux or Mac OS.

## Server

Building the server requires CMake at compile-time.

Type the following command to build:

```
cd server
cmake .
make
```

The executable is generated to `server/bin/im`. Run it to start a server daemon. It will listen at TCP Port 19623. All the data is hold in memory and will not be stored to disk.

## Client

The client is written based on Electron, which is a Node.js UI framework. You can refer to [the official document](https://electronjs.org/docs/tutorial/installation) on how to install it to your platform. The following is an example for Linux:

```
apt-get install npm # Install the Node.js package manager npm
cd client
npm install electron # Install Electron through npm to client/ directory
```

Now run the client with Electron:

```
# In client/ directory
node_modules/.bin/electron .
```
