**************************************************************
*******                                                 *****
*******      SORABH HAMIRWASIA                          *****
*******                                                 *****
**************************************************************

Files:
=====
1) project2.cpp
2) project2.h
3) message.h
4) Makefile
5) hostfile.txt
6) CS505_Project2.pdf  -- Report for the project.

Steps to generate the executable of the program and run it:-
====================================================
1) Go to the directory containing all the above mentioned files and run the
command "make". It will create an executable "proj2".

2) Create a hostfile with all the VM's hostname (e.g. ubuntu-clone49) in the hostfile.
I am submitting my hostfile.txt with this project.

3) Execute the executable generated in step 1 using the command below:

	Command:
	========
   ./proj2 -p <portNumber> -h <hostFileName> -c <numberOfDataMessageToSend>

5) To clean the executable. Run the command:

    make clean;
