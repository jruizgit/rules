#### Redis install
durable.js relies on Redis version 2.8  
  
1. If you are using mac:
  1. Make sure you have XCode installed, you can get this from App Store
  2. Download command line tools (File->Preferences->Downloads)
2. Download [Redis](http://download.redis.io/releases/redis-2.8.4.tar.gz)   
3. Extract the tar.gz package into a directory (for example, library\redis)  
4. Compile Redis:
  1. Start a terminal
  2. cd documents\redis\src
  3. make
5. Configure Redis:
  1. Open the library\redis\redis.conf file in your favorite editor
  2. Change the following entries:
    * Uncomment `port 0`
    * Uncomment `unixsocket /tmp/redis.sock`
    * Uncomment `unixsocketperm 755`
    * Comment `save 900 1`, `save 300 10`, `save 60 10000`
6. Start Redis: 
  1. Start a terminal
  2. cd library/redis
  3. src/redis-server redis.conf
  
For more information go to: http://redis.io/download

#### Node.js install
durable.js uses Node.js version  0.10.15.   

1. Download [Node.js](http://nodejs.org/dist/v0.10.15/node-v0.10.15.pkg) for Mac  
2. Run the installer, don't fight it, just follow the instructions  
3. The installer will set all the necessary environment variables, so you are ready to go  

For more information go to: http://nodejs.org/download  
#### First program
Now that your cache and web server are ready, let's write our first program:  

1. Start a terminal  
2. Create a directory for your app: `md /firstapp`  
3. In the new directory `npm install durable` (this will download durable.js and its dependencies)  
4. In that same directory create a test.js file using your favorite editor  
5. Type or copy and save the following code:
```javascript
var d = require('durable');
d.run({
    helloworld: {
        r1: {
            when: { content: 'first' },
            run: function(s) { console.log('Hello world'); }
        }
    }
});   
```
6. In the terminal type `node test.js`  
7. Open the web browser and go to: http://localhost:5000/helloworld/1/admin.html   
8. Type the message definition in the textbox at the bottom and click the 'Post Message' button  
```javascript
{ "id":1, "content":"first" }  
```
