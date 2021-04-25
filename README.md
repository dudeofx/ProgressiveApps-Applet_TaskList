## ProgressiveApps-Applet_TaskList
A simple task list applet that can be integrated into web-pages. 

The applet for private personal pages. It has no multi-user support and no effort was made to prevent misuse. The applet is considered prototype code (not production code).

The initiative to write this applet is to explore using the browser's DOM as the GUI for WebAssembly applets. 

At the moment there isn't direct access to the browser's DOM in C. We have to go thru JavaScript to get things done. C and JavaScript handle data very differently hence there is a lot of red-tape when jumping between languages. This is a cumbersome process from a programmer's perspective and also the result is inefficient code. Its discouraging for making small applets. For small apps its best to stick to JavaScript and just drop C. WebAssembly would benefit greatly from a framework that would hide away the ugliness. 

I conclude that if you decide to use the browser's DOM as a GUI, you will be required to spend considerable effort in developing an interface to access the DOM. For large projects, those that need to process a lot of data, C has the advantage that it can keep the system resources low and process the data efficiently. In such cases, for example a social network or an online wordprocessor, it might be worth the extra effort to use webassembly. 

Here, I used PHP as the server back-end but still processed the data in binary. That is not an elegant approach but it worked. Node.js is probably more native to work with a WebAssembly client side. Personally, I recommend CGI-BIN as a back-end because it allows me to work with binary data and still make use of Apache/Nginx servers.

---
## Quick Start

Compile the applet by executing "build.sh". Two files will be generated:

  * TaskList.js 
  * TaskList.wasm
  
The applet requires a SQLite3 database. Install and execute the program "sqlite3". When you run that program it puts you into a database environment. Inside that environment run the commands: 

    .save TaskList.db
    .quit

That created a file "TaskList.db" which represents the database. Next set file ownership and permissions: 

    sudo chown www-data:www-data TaskList.db
    sudo chmod 660 TaskList.db
  
Make sure SQLite3 is also installed/enabled on your PHP setup. Its likely your setup is ready to go but consult the internet if its not.

The following files are required to run the applet example. Just copy them over to the web-server.

  * TaskList.js 
  * TaskList.wasm
  * Index.html
  * TaskList.db
  * TaskList.php

     
---
## Applet deployment

see "index.html"

  * The script "TaskList.js" installs the applet to be used on the webpage. 
  * The applet is contained within a div element. To declare a div as a TaskList set the class attribute to "appTaskList". 
  * You must set the id attribute to uniquely identify your task list. 
  * The title of the task list is set by using the 'data-title' attribute.

The way the code works is that after the page is loaded, the code searches all the elements whose class is set to "appTaskList". The code then deploys the applet within the marked elements. The code uses the id attribute as the table name in the database. The title is not stored. It can be arbitrarily changed or forgotten. In other words the title purely a cosmetic feature. 

---
## Applet structure

The applet is split into client side and server side. (see "applet_layers.jpg")

### Client side

The client side is layed out in three layers: The surface, the interface and the app core. 

  * "index.html" is the surface or top layer. Its job is to determine how the GUI is going to be presented.
  * "TaskList.c" is the app core which is the inner most layer. 
  * "TaskList.Interface.js" is the interface which sits in between the surface layer and the app core. Its job is to hide rudimentary house keeping tasks in order to make things cleaner and easy to manage both at the surface and the app core.

### Server side

"TaskList.php" handles the server side


---
## Applet Communications

The client communicates with the server in a request response fashion. Data exchange is done in raw binary. Compatibility of binary data format is assumed to be true. The C structure "tHeader" is used as the header of the packet that is exchanged back and forth. The definition for "tHeader" is found on "TaskList.h". The packet structure consists a command, a single integer parameter, the name of the task list and an optional body.

The following commands as used as a protocol.

  * 0 = NOP: Do nothing (ignored)

server side:

  * 0x01 = GET_INDEX: Request the index; Server returns INDEX
  * 0x02 = GET_ITEM: Request a single item (param == id); Server returns ITEM
  * 0x03 = INSERT: Insert new item; Server returns NEW_ITEM
  * 0x04 = DELETE: Deletes item; Server returns DELETE
  * 0x05 = UPDATE: Updates item; Server returns UPDATE

client side:

  * 0x04 = DELETE: Informs client which items where deleted
  * 0x05 = UPDATE: Updates a specific item on the index
  * 0x06 = INDEX: Samples of all items 
  * 0x07 = ITEM: A single item (for edit usually)
  * 0x08 = NEW_ITEM: A newly added item
  * 0x09 = OK: Command successful (unused)
  * 0x0A = ERROR: Error (param = code) (unused)
   
