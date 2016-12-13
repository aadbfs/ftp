/* 
* Name: Alex Silva
* inspiration from: http://www.rgagnon.com/javadetails/java-0542.html
*/

/****
Library definitions
****/
import java.io.BufferedOutputStream;
import java.io.FileOutputStream;
import java.io.IOException; 
import java.io.InputStream;
import java.io.OutputStream;
import java.net.Socket;
import java.net.ServerSocket;


/****
Class: ftclient
Description: main class block
****/
public class ftclient 
{
    public final static int fileSize = 4194304; //def: #file size
    /****
    function: main
    Description: main class block
    ****/
    public static void main (String [] args) throws IOException 
    {
        //init var decs
        int currentByte = 0;
        int readBytes;
        int serverPort = 0;
        int dataPort = 0;
        FileOutputStream fos = null;
        BufferedOutputStream bos = null;
        OutputStream os = null;
        ServerSocket servSock = null;
        Socket dataSock = null;
        Socket controlSock = null;
        String serverHost = null;
        String command = null;
        String filename= null;
        String pushCommand = null;
        
        //if incorrect number of args
        if (args.length < 4 || args.length > 5)
        {
            System.out.println("Usage: ftclient.java <serverHost> <serverPort> -g|-l <filename> <dataPort>");
            System.exit(0);
        }
        
        //get host, port, and command
        serverHost = args[0];
        serverPort = Integer.parseInt(args[1]);
        command = args[2];
        
        //if -g
        if (command.equals("-g")) 
        {
            filename = args[3];
            dataPort = Integer.parseInt(args[4]);
        }
        
        //elif -l
        else if (command.equals("-l"))
        {
            filename = null;
            dataPort = Integer.parseInt(args[3]);
        }
        
        else
        {
            System.out.println("Usage: command must be -g or -l");
            System.exit(0);
        }
        
        try 
        {
            //listen, connect
            System.out.printf("Data port: %d\n", dataPort);
            servSock = new ServerSocket(dataPort); 
            controlSock = new Socket(serverHost, serverPort);
            System.out.printf("Connected to host %s on port %d\n", serverHost, serverPort);
            
            //push command to server
            pushCommand = command + " " + serverHost + " " + dataPort + " " + filename;
            os = controlSock.getOutputStream();
            os.write(pushCommand.getBytes());
            os.flush();
            
            //accept connection
            dataSock = servSock.accept();
            
            //if -g
            if (command.equals("-g"))
            {
                //recv file
                byte [] arr  = new byte [fileSize];
                InputStream is = dataSock.getInputStream();

                readBytes = is.read(arr, 0, arr.length);
                currentByte = readBytes;
                
                //read bytes
                do 
                {
                   readBytes = is.read(arr, currentByte, (arr.length - currentByte));
                   if (readBytes >= 0) 
                       currentByte += readBytes;
                } while(readBytes > -1);
                
                //write bytes
                if (currentByte > 1)
                {
                    fos = new FileOutputStream("copyof_" + filename);
                    bos = new BufferedOutputStream(fos);
                    bos.write(arr, 0 , currentByte);
                    bos.flush();
                    System.out.println("File " + filename + " downloaded (" + currentByte + " bytes read)");
                } 
            }
            
            //else
            else 
            {
                
                byte [] arr  = new byte [fileSize];
                InputStream is = dataSock.getInputStream();
                readBytes = is.read(arr, 0, arr.length);
                currentByte = readBytes;
                
                do 
                {
                   readBytes = is.read(arr, currentByte, (arr.length-currentByte));
                   if (readBytes >= 0) 
                       currentByte += readBytes;
                } while(readBytes > -1);
                
                System.out.write(arr);
            }
        }
        
        //finally, close all used things
        finally 
        {
          if (fos != null) 
              fos.close();
          if (bos != null) 
              bos.close();
          if (servSock != null) 
              servSock.close();
          if (dataSock != null) 
              dataSock.close();
          if (controlSock != null) 
              controlSock.close();
        }
    }
}