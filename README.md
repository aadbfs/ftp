# ftp

Use unix/linux/related miscellany to run the code, as this was the original testing and development environment.

1. Download the files into a directory.

2. If needed, chmod +x the files

3. Compile the c program by using this command: gcc ftserver.c -o ftserver

4. Next, compile the Java program using this command: javac ftclient.java

5. Next, open up a seperate server window and execute the server program using the command: ftserver PORTNO
The PORTNO is the portnumber you define.

6. In the original server window, cd to the Java program. Execute it via: java ftclient [SERVER NAME] [SERVER PORTNO] -COMMAND "filename_if_command_is_minus_g.txt" [DATA PORTNO]

7. All valid file types are transferable.
